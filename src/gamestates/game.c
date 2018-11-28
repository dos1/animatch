/*! \file empty.c
 *  \brief Empty gamestate.
 */
/*
 * Copyright (c) Sebastian Krzyszkowiak <dos@dosowisko.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../common.h"
#include <allegro5/allegro_color.h>
#include <libsuperderpy.h>

#define MAX_ACTIONS 16
#define MATCHING_TIME 0.4
#define MATCHING_DELAY_TIME 0.2
#define FALLING_TIME 0.5
#define SWAPPING_TIME 0.15
#define SHAKING_TIME 0.5
#define HINT_TIME 1.0
#define LAUNCHING_TIME 1.5
#define COLLECTING_TIME 0.6

#define BLUR_DIVIDER 8

#define COLS 8
#define ROWS 8

static char* ANIMALS[] = {"bee", "bird", "cat", "fish", "frog", "ladybug"};
static char* SPECIALS[] = {"egg", "berry", "apple", "chestnut", "special", "eyes", "dandelion"};

static struct {
	int actions;
	char* names[MAX_ACTIONS];
} ACTIONS[] = {
	{.actions = 3, .names = {"eyeroll", "fly1", "fly2"}}, // bee
	{.actions = 3, .names = {"eyeroll", "sing1", "sing2"}}, // bird
	{.actions = 3, .names = {"action1", "action2", "action3"}}, // cat
	{.actions = 3, .names = {"eyeroll", "swim1", "swim2"}}, // fish
	{.actions = 5, .names = {"bump1", "bump2", "eyeroll", "tonque1", "tonque2"}}, // frog
	{.actions = 6, .names = {"bump1", "bump2", "bump3", "eyeroll1", "eyeroll2", "kiss"}}, // ladybug
	//
	{.actions = 2, .names = {"stand", "stand2"}}, // egg
	{.actions = 1, .names = {"stand"}}, // berry
	{.actions = 2, .names = {"stand", "stand2"}}, // apple
	{.actions = 3, .names = {"stand", "stand2", "stand3"}}, // chestnut
	{.actions = 6, .names = {"bee", "bird", "cat", "fish", "frog", "ladybug"}}, // special
	{.actions = 1, .names = {"eyes"}}, // eyes
	{.actions = 1, .names = {"stand"}} // dandelion
};

enum ANIMAL_TYPE {
	ANIMAL_TYPE_BEE,
	ANIMAL_TYPE_BIRD,
	ANIMAL_TYPE_CAT,
	ANIMAL_TYPE_FISH,
	ANIMAL_TYPE_FROG,
	ANIMAL_TYPE_LADYBUG,

	ANIMAL_TYPES
};

enum COLLECTIBLE_TYPE {
	COLLECTIBLE_TYPE_BERRY,
	COLLECTIBLE_TYPE_APPLE,
	COLLECTIBLE_TYPE_CHESTNUT,
	COLLECTIBLE_TYPES
};

enum FIELD_TYPE {
	FIELD_TYPE_ANIMAL,
	FIELD_TYPE_FREEFALL,
	FIELD_TYPE_COLLECTIBLE,
	FIELD_TYPE_EMPTY,
	FIELD_TYPE_DISABLED,

	FIELD_TYPES
};

struct FieldID {
	int i;
	int j;
};

struct Field {
	struct FieldID id;
	enum FIELD_TYPE type;
	union {
		struct {
			enum COLLECTIBLE_TYPE type;
			int variant;
		} collectible;
		struct {
			enum ANIMAL_TYPE type;
			bool sleeping;
			bool special;
		} animal;
		struct {
			int variant;
		} freefall;
	} data;
	int matched;
	bool to_remove;
	bool handled;

	struct Character *drawable, *overlay;
	bool overlay_visible;

	struct {
		struct Tween hiding, falling, swapping, shaking, hinting, launching, collecting;
		struct FieldID swapee;
		int fall_levels, level_no;
		int time_to_action, action_time;
		int time_to_blink, blink_time;
	} animation;
};

struct GamestateResources {
	// This struct is for every resource allocated and used by your gamestate.
	// It gets created on load and then gets passed around to all other function calls.
	ALLEGRO_BITMAP* bg;

	ALLEGRO_BITMAP *scene, *lowres_scene, *lowres_scene_blur, *board;

	struct Character* archetypes[sizeof(ANIMALS) / sizeof(ANIMALS[0]) + sizeof(SPECIALS) / sizeof(SPECIALS[0])];
	struct FieldID current, hovered, swap1, swap2;
	struct Field fields[COLS][ROWS];

	struct Timeline* timeline;

	ALLEGRO_BITMAP *field_bgs[4], *field_bgs_bmp;

	ALLEGRO_SHADER *combine_shader, *desaturate_shader;

	bool locked, clicked;

	struct ParticleBucket* particles;
};

int Gamestate_ProgressCount = 60; // number of loading steps as reported by Gamestate_Load

static void ProcessFields(struct Game* game, struct GamestateResources* data);

static bool IsSleeping(struct Field* field) {
	return (field->type == FIELD_TYPE_ANIMAL && field->data.animal.sleeping);
}

static inline bool IsDrawable(enum FIELD_TYPE type) {
	return (type == FIELD_TYPE_ANIMAL) || (type == FIELD_TYPE_FREEFALL) || (type == FIELD_TYPE_COLLECTIBLE);
}

static inline bool IsSameID(struct FieldID one, struct FieldID two) {
	return one.i == two.i && one.j == two.j;
}

static inline bool IsValidID(struct FieldID id) {
	return id.i != -1 && id.j != -1;
}

struct DandelionParticleData {
	double angle, dangle;
	double scale, dscale;
	struct GravityParticleData* data;
};

static struct DandelionParticleData* DandelionParticleData() {
	struct DandelionParticleData* data = calloc(1, sizeof(struct DandelionParticleData));
	data->data = GravityParticleData((rand() / (double)RAND_MAX - 0.5) / 64.0, (rand() / (double)RAND_MAX - 0.5) / 64.0, 0.000075, 0.000075);
	data->angle = rand() / (double)RAND_MAX * 2 * ALLEGRO_PI;
	data->dangle = (rand() / (double)RAND_MAX - 0.5) * ALLEGRO_PI;
	data->scale = 0.6 + 0.1 * rand() / (double)RAND_MAX;
	data->dscale = (rand() / (double)RAND_MAX - 0.5) * 0.002;
	return data;
}

static bool DandelionParticle(struct Game* game, struct ParticleState* particle, double delta, void* d) {
	struct DandelionParticleData* data = d;
	data->angle += data->dangle * delta;
	data->scale += data->dscale * delta;

	particle->tint = al_map_rgba(222, 222, 222, 222);
	particle->angle = data->angle;
	particle->scaleX = data->scale;
	particle->scaleY = data->scale;
	bool res = GravityParticle(game, particle, delta, data->data);
	if (!res) {
		free(data->data);
		free(data);
	}
	return res;
}

void Gamestate_Logic(struct Game* game, struct GamestateResources* data, double delta) {
	// Called 60 times per second (by default). Here you should do all your game logic.
	TM_Process(data->timeline, delta);
	UpdateParticles(game, data->particles, delta);

	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			AnimateCharacter(game, data->fields[i][j].drawable, delta, 1.0);
			if (data->fields[i][j].overlay_visible) {
				AnimateCharacter(game, data->fields[i][j].overlay, delta, 1.0);
			}
			UpdateTween(&data->fields[i][j].animation.falling, delta);
			UpdateTween(&data->fields[i][j].animation.hiding, delta);
			UpdateTween(&data->fields[i][j].animation.swapping, delta);
			UpdateTween(&data->fields[i][j].animation.shaking, delta);
			UpdateTween(&data->fields[i][j].animation.hinting, delta);
			UpdateTween(&data->fields[i][j].animation.launching, delta);
			UpdateTween(&data->fields[i][j].animation.collecting, delta);

			if (IsSleeping(&data->fields[i][j])) {
				continue;
			}

			if (data->fields[i][j].animation.blink_time) {
				data->fields[i][j].animation.blink_time -= (int)(delta * 1000);
				if (data->fields[i][j].animation.blink_time <= 0) {
					data->fields[i][j].animation.blink_time = 0;
					if (data->fields[i][j].type == FIELD_TYPE_ANIMAL) {
						SelectSpritesheet(game, data->fields[i][j].drawable, "stand");
					}
				}
			} else if (data->fields[i][j].animation.action_time) {
				data->fields[i][j].animation.action_time -= (int)(delta * 1000);
				if (data->fields[i][j].animation.action_time <= 0) {
					data->fields[i][j].animation.action_time = 0;
					if (data->fields[i][j].type == FIELD_TYPE_ANIMAL) {
						SelectSpritesheet(game, data->fields[i][j].drawable, "stand");
					}
				}
			} else {
				data->fields[i][j].animation.time_to_action -= (int)(delta * 1000);
				data->fields[i][j].animation.time_to_blink -= (int)(delta * 1000);

				if (data->fields[i][j].animation.time_to_action <= 0) {
					data->fields[i][j].animation.time_to_action = rand() % 250000 + 500000;
					data->fields[i][j].animation.action_time = rand() % 2000 + 1000;
					if (data->fields[i][j].type == FIELD_TYPE_ANIMAL) {
						SelectSpritesheet(game, data->fields[i][j].drawable, ACTIONS[data->fields[i][j].data.animal.type].names[rand() % ACTIONS[data->fields[i][j].data.animal.type].actions]);
					}
				}

				if (data->fields[i][j].animation.time_to_blink <= 0) {
					if (strcmp(data->fields[i][j].drawable->spritesheet->name, "stand") == 0) {
						if (data->fields[i][j].type == FIELD_TYPE_ANIMAL) {
							SelectSpritesheet(game, data->fields[i][j].drawable, "blink");
						}
						data->fields[i][j].animation.time_to_blink = rand() % 100000 + 200000;
						data->fields[i][j].animation.blink_time = rand() % 400 + 100;
					}
				}
			}
		}
	}
}

static void DrawScene(struct Game* game, struct GamestateResources* data) {
	al_set_target_bitmap(data->scene);
	ClearToColor(game, al_map_rgb(0, 0, 0));
	al_draw_bitmap(data->bg, 0, 0, 0);
}

static void UpdateBlur(struct Game* game, struct GamestateResources* data) {
	DrawScene(game, data);

	float size[2] = {al_get_bitmap_width(data->lowres_scene), al_get_bitmap_height(data->lowres_scene)};

	al_set_target_bitmap(data->lowres_scene_blur);
	ClearToColor(game, al_map_rgb(0, 0, 0));
	al_draw_scaled_bitmap(data->scene, 0, 0, al_get_bitmap_width(data->scene), al_get_bitmap_height(data->scene),
		0, 0, al_get_bitmap_width(data->lowres_scene_blur), al_get_bitmap_height(data->lowres_scene_blur), 0);

	al_set_target_bitmap(data->lowres_scene);
	ClearToColor(game, al_map_rgb(0, 0, 0));
	al_use_shader(game->data->kawese_shader);
	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_set_shader_float_vector("size", 2, size, 1);
	al_set_shader_float("kernel", 0);
	al_draw_bitmap(data->lowres_scene_blur, 0, 0, 0);
	al_use_shader(NULL);

	al_set_target_bitmap(data->lowres_scene_blur);
	ClearToColor(game, al_map_rgb(0, 0, 0));
	al_use_shader(game->data->kawese_shader);
	al_set_shader_float_vector("size", 2, size, 1);
	al_set_shader_float("kernel", 0);
	al_draw_bitmap(data->lowres_scene, 0, 0, 0);
	al_use_shader(NULL);
}

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	// Called as soon as possible, but no sooner than next Gamestate_Logic call.
	// Draw everything to the screen here.
	DrawScene(game, data);

	int offsetY = (int)((game->viewport.height - (ROWS * 90)) / 2.0);

	al_set_target_bitmap(data->board);
	ClearToColor(game, al_map_rgba(0, 0, 0, 0));
	al_set_clipping_rectangle(0, offsetY, game->viewport.width, game->viewport.height - offsetY * 2);
	al_hold_bitmap_drawing(true);
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			bool hovered = IsSameID(data->hovered, (struct FieldID){i, j});
			ALLEGRO_COLOR color = al_map_rgba(222, 222, 222, 222);
			if (data->locked || game->data->touch || !hovered) {
				color = al_map_rgba(180, 180, 180, 180);
			}
			if (data->fields[i][j].type != FIELD_TYPE_DISABLED) {
				ALLEGRO_BITMAP* bmp = data->field_bgs[(j * ROWS + i + j % 2) % 4];
				al_draw_tinted_scaled_bitmap(bmp, color, 0, 0, al_get_bitmap_width(bmp), al_get_bitmap_height(bmp),
					i * 90 + 1, j * 90 + offsetY + 1, 90 - 2, 90 - 2, 0);
			}
		}
	}
	al_hold_bitmap_drawing(false);

	al_use_shader(data->desaturate_shader);
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			float tint = 1.0 - GetTweenValue(&data->fields[i][j].animation.hiding);
			data->fields[i][j].drawable->tint = al_map_rgba_f(tint, tint, tint, tint);

			int levels = data->fields[i][j].animation.fall_levels;
			int level_no = data->fields[i][j].animation.level_no;
			float tween = Interpolate(GetTweenPosition(&data->fields[i][j].animation.falling), TWEEN_STYLE_EXPONENTIAL_OUT) * (0.5 - level_no * 0.1) +
				sqrt(Interpolate(GetTweenPosition(&data->fields[i][j].animation.falling), TWEEN_STYLE_BOUNCE_OUT)) * (0.5 + level_no * 0.1);

			//			tween = Interpolate(GetTweenPosition(&data->fields[i][j].falling), TWEEN_STYLE_ELASTIC_OUT);
			int levelDiff = (int)(levels * 90 * (1.0 - tween));

			int x = i * 90 + 45, y = j * 90 + 45 + offsetY - levelDiff;
			int swapeeX = data->fields[i][j].animation.swapee.i * 90 + 45, swapeeY = data->fields[i][j].animation.swapee.j * 90 + 45 + offsetY;

			y -= (int)(sin(GetTweenValue(&data->fields[i][j].animation.collecting) * ALLEGRO_PI) * 10);

			SetCharacterPosition(game, data->fields[i][j].drawable, Lerp(x, swapeeX, GetTweenValue(&data->fields[i][j].animation.swapping)), Lerp(y, swapeeY, GetTweenValue(&data->fields[i][j].animation.swapping)), 0);

			if (IsDrawable(data->fields[i][j].type)) {
				al_set_shader_bool("enabled", IsSleeping(&data->fields[i][j]));
				data->fields[i][j].drawable->angle = sin(GetTweenValue(&data->fields[i][j].animation.shaking) * 3 * ALLEGRO_PI) / 6.0 + sin(GetTweenValue(&data->fields[i][j].animation.hinting) * 5 * ALLEGRO_PI) / 6.0 + sin(GetTweenPosition(&data->fields[i][j].animation.collecting) * 2 * ALLEGRO_PI) / 12.0 + sin(GetTweenValue(&data->fields[i][j].animation.launching) * 5 * ALLEGRO_PI) / 6.0;
				data->fields[i][j].drawable->scaleX = 1.0 + sin(GetTweenValue(&data->fields[i][j].animation.hinting) * ALLEGRO_PI) / 3.0 + sin(GetTweenValue(&data->fields[i][j].animation.launching) * ALLEGRO_PI) / 3.0;
				data->fields[i][j].drawable->scaleY = data->fields[i][j].drawable->scaleX;
				DrawCharacter(game, data->fields[i][j].drawable);
				if (data->fields[i][j].overlay_visible) {
					DrawCharacter(game, data->fields[i][j].overlay);
				}
			}
		}
	}
	al_use_shader(NULL);
	al_reset_clipping_rectangle();

	SetFramebufferAsTarget(game);
	ClearToColor(game, al_map_rgb(0, 0, 0));
	al_draw_bitmap(data->scene, 0, 0, 0);

	float size[2] = {al_get_bitmap_width(data->lowres_scene_blur), al_get_bitmap_height(data->lowres_scene_blur)};

	al_use_shader(data->combine_shader);
	al_set_shader_sampler("tex_bg", data->lowres_scene_blur, 1);
	al_set_shader_float_vector("size", 2, size, 1);
	al_draw_bitmap(data->board, 0, 0, 0);
	al_use_shader(NULL);

	DrawParticles(game, data->particles);

	al_use_shader(data->desaturate_shader);
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			if (IsDrawable(data->fields[i][j].type) && GetTweenPosition(&data->fields[i][j].animation.launching) < 1.0) {
				al_set_shader_bool("enabled", IsSleeping(&data->fields[i][j]));
				DrawCharacter(game, data->fields[i][j].drawable);
				if (data->fields[i][j].overlay_visible) {
					DrawCharacter(game, data->fields[i][j].overlay);
				}
			}
		}
	}
	al_use_shader(NULL);
}

static struct FieldID ToLeft(struct FieldID id) {
	id.i--;
	if (id.i < 0) {
		return (struct FieldID){-1, -1};
	}
	return id;
}

static struct FieldID ToRight(struct FieldID id) {
	id.i++;
	if (id.i >= COLS) {
		return (struct FieldID){-1, -1};
	}
	return id;
}

static struct FieldID ToTop(struct FieldID id) {
	id.j--;
	if (id.j < 0) {
		return (struct FieldID){-1, -1};
	}
	return id;
}

static struct FieldID ToBottom(struct FieldID id) {
	id.j++;
	if (id.j >= ROWS) {
		return (struct FieldID){-1, -1};
	}
	return id;
}

static bool IsValidMove(struct FieldID one, struct FieldID two) {
	if (one.i == two.i && abs(one.j - two.j) == 1) {
		return true;
	}
	if (one.j == two.j && abs(one.i - two.i) == 1) {
		return true;
	}
	return false;
}

static struct Field* GetField(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	if (!IsValidID(id)) {
		return NULL;
	}
	return &data->fields[id.i][id.j];
}

static void UpdateOverlay(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	struct Field* field = GetField(game, data, id);
	if (!IsDrawable(field->type)) {
		return;
	}
	char* name = NULL;
	int index = -1;
	if (field->type == FIELD_TYPE_ANIMAL) {
		if (field->data.animal.special) {
			name = "eyes";
			index = ANIMAL_TYPES + 5; // FIXME
		}
	}

	if (name) {
		struct Character* archetype = data->archetypes[index];
		if (field->overlay->name) {
			free(field->overlay->name);
		}
		field->overlay->name = strdup(archetype->name);
		field->overlay->spritesheets = archetype->spritesheets;

		SelectSpritesheet(game, field->overlay, name);
		field->overlay_visible = true;
	} else {
		field->overlay_visible = false;
	}
}

static void UpdateDrawable(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	struct Field* field = GetField(game, data, id);
	if (!IsDrawable(field->type)) {
		return;
	}

	char* name = "stand";
	if (IsSleeping(field)) {
		name = "blink";
	}

	int index = 0;
	if (field->type == FIELD_TYPE_FREEFALL) {
		index = ANIMAL_TYPES;
		name = ACTIONS[index].names[field->data.freefall.variant];
	} else if (field->type == FIELD_TYPE_COLLECTIBLE) {
		index = ANIMAL_TYPES + 1 + field->data.collectible.type;
		int variant = field->data.collectible.variant;
		if (variant == ACTIONS[index].actions) {
			variant--;
		}
		name = ACTIONS[index].names[variant];
	} else if (field->type == FIELD_TYPE_ANIMAL) {
		index = field->data.animal.type;
		if (field->data.animal.special) {
			index = ANIMAL_TYPES + 4; // FIXME
			name = ANIMALS[field->data.animal.type];
		}
	}

	struct Character* archetype = data->archetypes[index];

	if (field->drawable->name) {
		free(field->drawable->name);
	}
	field->drawable->name = strdup(archetype->name);
	field->drawable->spritesheets = archetype->spritesheets;

	SelectSpritesheet(game, field->drawable, name);

	UpdateOverlay(game, data, id);
}

static int IsMatching(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	int lchain = 0, tchain = 0;
	if (!IsValidID(id)) {
		return 0;
	}
	struct Field* orig = GetField(game, data, id);
	if (orig->type == FIELD_TYPE_FREEFALL) {
		if (id.j == ROWS - 1) {
			return 1;
		}
	}
	if (orig->type == FIELD_TYPE_COLLECTIBLE) {
		if (orig->data.collectible.variant == ACTIONS[ANIMAL_TYPES + 1 + orig->data.collectible.type].actions) {
			return 1;
		}
	}
	if (orig->type != FIELD_TYPE_ANIMAL) {
		return 0;
	}
	if (IsSleeping(orig)) {
		return 0;
	}

	struct FieldID (*callbacks[])(struct FieldID) = {ToLeft, ToRight, ToTop, ToBottom};
	int* accumulators[] = {&lchain, &lchain, &tchain, &tchain};

	for (int i = 0; i < 4; i++) {
		struct FieldID pos = (callbacks[i])(id);
		while (IsValidID(pos)) {
			struct Field* field = GetField(game, data, pos);
			if (field->type != FIELD_TYPE_ANIMAL) {
				break;
			}
			if (field->data.animal.type != orig->data.animal.type) {
				break;
			}
			if (IsSleeping(field)) {
				break;
			}
			(*accumulators[i])++;
			pos = (callbacks[i])(pos);
		}
	}

	int chain = 0;
	if (lchain >= 2) {
		chain += lchain;
	}
	if (tchain >= 2) {
		chain += tchain;
	}
	if (chain) {
		chain++;
	}
	//PrintConsole(game, "field %dx%d %s lchain %d tchain %d chain %d", id.i, id.j, ANIMALS[orig->type], lchain, tchain, chain);
	return chain;
}

static int MarkMatching(struct Game* game, struct GamestateResources* data) {
	int matching = 0;
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->fields[i][j].matched = IsMatching(game, data, (struct FieldID){i, j});
			if (data->fields[i][j].matched) {
				data->fields[i][j].to_remove = true;
				matching++;
			}
		}
	}
	PrintConsole(game, "matched %d", matching);
	return matching;
}

/*
static bool AreAdjacentMatching(struct Game* game, struct GamestateResources* data, struct FieldID id, struct FieldID (*func)(struct FieldID)) {
	for (int i = 0; i < 3; i++) {
		id = func(id);
		if (!IsValidID(id) || !GetField(game, data, id)->matched) {
			return false;
		}
	}
	return true;
}

static int IsMatchExtension(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	return AreAdjacentMatching(game, data, id, ToTop) || AreAdjacentMatching(game, data, id, ToBottom) || AreAdjacentMatching(game, data, id, ToLeft) || AreAdjacentMatching(game, data, id, ToRight);
}
*/

static int ShouldBeCollected(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	return IsMatching(game, data, ToTop(id)) || IsMatching(game, data, ToBottom(id)) || IsMatching(game, data, ToLeft(id)) || IsMatching(game, data, ToRight(id));
}

static int Collect(struct Game* game, struct GamestateResources* data) {
	int collected = 0;
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			if (ShouldBeCollected(game, data, data->fields[i][j].id) || data->fields[i][j].to_remove) {
				if (IsSleeping(&data->fields[i][j])) {
					data->fields[i][j].data.animal.sleeping = false;
					data->fields[i][j].to_remove = false;
					UpdateDrawable(game, data, data->fields[i][j].id);
					data->fields[i][j].animation.collecting = Tween(game, 0.0, 1.0, TWEEN_STYLE_BOUNCE_OUT, COLLECTING_TIME);
					collected++;
				} else if (data->fields[i][j].type == FIELD_TYPE_COLLECTIBLE) {
					data->fields[i][j].data.collectible.variant++;

					if (data->fields[i][j].data.collectible.variant == ACTIONS[ANIMAL_TYPES + 1 + data->fields[i][j].data.collectible.type].actions) {
						data->fields[i][j].to_remove = true;
					} else {
						data->fields[i][j].to_remove = false;
					}
					UpdateDrawable(game, data, data->fields[i][j].id);
					data->fields[i][j].animation.collecting = Tween(game, 0.0, 1.0, TWEEN_STYLE_BOUNCE_OUT, COLLECTING_TIME);
					collected++;
				}
			}
		}
	}
	PrintConsole(game, "collected %d", collected);
	return collected;
}

static void SpawnParticles(struct Game* game, struct GamestateResources* data, struct FieldID id, int num) {
	struct Field* field = GetField(game, data, id);
	for (int p = 0; p < num; p++) {
		data->archetypes[12]->pos = rand() % data->archetypes[12]->spritesheet->frameCount;
		if (rand() % 2) {
			data->archetypes[12]->pos = 0;
		}
		float x = GetCharacterX(game, field->drawable) / (double)game->viewport.width, y = GetCharacterY(game, field->drawable) / (double)game->viewport.height;
		EmitParticle(game, data->particles, data->archetypes[12], FaderParticle, SpawnParticleBetween(x - 0.01, y - 0.01, x + 0.01, y + 0.01), FaderParticleData(1.0, 0.025, DandelionParticle, DandelionParticleData(), free));
	}
}

static void AnimateMatching(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			if (data->fields[i][j].matched) {
				if (data->fields[i][j].type == FIELD_TYPE_ANIMAL) {
					SelectSpritesheet(game, data->fields[i][j].drawable, ACTIONS[data->fields[i][j].data.animal.type].names[rand() % ACTIONS[data->fields[i][j].type].actions]);

					if (data->fields[i][j].matched >= 4 && ((IsSameID(data->swap1, data->fields[i][j].id)) || (IsSameID(data->swap2, data->fields[i][j].id)))) {
						data->fields[i][j].data.animal.special = true;
						data->fields[i][j].to_remove = false;
						UpdateDrawable(game, data, data->fields[i][j].id);
						SpawnParticles(game, data, data->fields[i][j].id, 64);
						continue;
					}
				}
				data->locked = true;

				SpawnParticles(game, data, data->fields[i][j].id, 16);
			}
		}
	}
}

static void AnimateRemoval(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			if (data->fields[i][j].to_remove) {
				data->fields[i][j].animation.hiding = Tween(game, 0.0, 1.0, TWEEN_STYLE_LINEAR, MATCHING_TIME);
				data->fields[i][j].animation.hiding.predelay = MATCHING_DELAY_TIME;
			}
		}
	}
}

static void DoRemoval(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->fields[i][j].animation.fall_levels = 0;
			data->fields[i][j].animation.level_no = 0;
			if (data->fields[i][j].to_remove) {
				data->fields[i][j].type = FIELD_TYPE_EMPTY;
				data->fields[i][j].matched = false;
				data->fields[i][j].to_remove = false;
				data->fields[i][j].handled = false;
				data->fields[i][j].animation.hiding = StaticTween(game, 0.0);
				data->fields[i][j].animation.falling = StaticTween(game, 1.0);
				data->fields[i][j].animation.shaking = StaticTween(game, 0.0);
				data->fields[i][j].animation.hinting = StaticTween(game, 0.0);
				data->fields[i][j].animation.launching = StaticTween(game, 0.0);
				data->fields[i][j].animation.collecting = StaticTween(game, 0.0);
			}
		}
	}
}

static void StopAnimations(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->fields[i][j].animation.fall_levels = 0;
			data->fields[i][j].animation.level_no = 0;
			data->fields[i][j].animation.hiding = StaticTween(game, 0.0);
			data->fields[i][j].animation.falling = StaticTween(game, 1.0);
			data->fields[i][j].animation.shaking = StaticTween(game, 0.0);
			data->fields[i][j].animation.hinting = StaticTween(game, 0.0);
			data->fields[i][j].animation.launching = StaticTween(game, 0.0);
			data->fields[i][j].animation.collecting = StaticTween(game, 0.0);
		}
	}
}

static void Swap(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two) {
	struct Field tmp = data->fields[one.i][one.j];
	data->fields[one.i][one.j] = data->fields[two.i][two.j];
	data->fields[two.i][two.j] = tmp;
	data->fields[one.i][one.j].id = (struct FieldID){.i = one.i, .j = one.j};
	data->fields[two.i][two.j].id = (struct FieldID){.i = two.i, .j = two.j};
}

static TM_ACTION(AfterSwapping) {
	TM_RunningOnly;
	struct Field* one = TM_GetArg(action->arguments, 0);
	struct Field* two = TM_GetArg(action->arguments, 1);
	Swap(game, data, one->id, two->id);
	one->animation.swapping = StaticTween(game, 0.0);
	two->animation.swapping = StaticTween(game, 0.0);
	data->locked = false;
	ProcessFields(game, data);
	return true;
}

static TM_ACTION(StartSwapping) {
	TM_RunningOnly;
	struct Field* one = TM_GetArg(action->arguments, 0);
	struct Field* two = TM_GetArg(action->arguments, 1);
	data->locked = true;
	one->animation.swapping = Tween(game, 0.0, 1.0, TWEEN_STYLE_SINE_IN_OUT, SWAPPING_TIME);
	one->animation.swapee = two->id;
	two->animation.swapping = Tween(game, 0.0, 1.0, TWEEN_STYLE_SINE_IN_OUT, SWAPPING_TIME);
	two->animation.swapee = one->id;
	return true;
}

static bool IsSwappable(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	if (!IsValidID(id)) {
		return false;
	}
	struct Field* field = GetField(game, data, id);
	if (((field->type == FIELD_TYPE_ANIMAL) && (!IsSleeping(field))) || (field->type == FIELD_TYPE_COLLECTIBLE)) {
		return true;
	}
	return false;
}

static bool AreSwappable(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two) {
	return IsSwappable(game, data, one) && IsSwappable(game, data, two);
}

static void AnimateSwapping(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two) {
	data->swap1 = one;
	data->swap2 = two;
	TM_AddAction(data->timeline, StartSwapping, TM_Args(GetField(game, data, one), GetField(game, data, two)));
	TM_AddDelay(data->timeline, SWAPPING_TIME * 1000);
	TM_AddAction(data->timeline, AfterSwapping, TM_Args(GetField(game, data, one), GetField(game, data, two)));
}

static void GenerateField(struct Game* game, struct GamestateResources* data, struct Field* field) {
	if (rand() / (float)RAND_MAX < 0.001) {
		field->type = FIELD_TYPE_FREEFALL;
		field->data.freefall.variant = rand() % ACTIONS[ANIMAL_TYPES].actions;
	} else if (rand() / (float)RAND_MAX < 0.01) {
		field->type = FIELD_TYPE_COLLECTIBLE;
		field->data.collectible.type = rand() % COLLECTIBLE_TYPES;
		field->data.collectible.variant = 0;
	} else {
		field->type = FIELD_TYPE_ANIMAL;
		field->data.animal.type = rand() % ANIMAL_TYPES;
		if (rand() / (float)RAND_MAX < 0.005) {
			field->data.animal.sleeping = true;
		}
		field->data.animal.special = false;
	}
	field->overlay_visible = false;
	UpdateDrawable(game, data, field->id);
}

static void CreateNewField(struct Game* game, struct GamestateResources* data, struct Field* field) {
	GenerateField(game, data, field);
	field->animation.fall_levels++;
	field->animation.falling = Tween(game, 0.0, 1.0, TWEEN_STYLE_BOUNCE_OUT, FALLING_TIME * (1.0 + field->animation.level_no * 0.025));
	field->animation.hiding = Tween(game, 1.0, 0.0, TWEEN_STYLE_LINEAR, 0.25);
}

static void Gravity(struct Game* game, struct GamestateResources* data) {
	bool repeat;
	do {
		repeat = false;
		for (int i = 0; i < COLS; i++) {
			for (int j = ROWS - 1; j >= 0; j--) {
				struct FieldID id = (struct FieldID){i, j};
				struct Field* field = GetField(game, data, id);
				if (field->type != FIELD_TYPE_EMPTY) {
					continue;
				}
				struct FieldID up = ToTop(id);
				while (IsValidID(up) && (GetField(game, data, up)->type == FIELD_TYPE_DISABLED)) {
					up = ToTop(up);
				}
				if (IsValidID(up)) {
					struct Field* upfield = GetField(game, data, up);
					if (upfield->type == FIELD_TYPE_EMPTY) {
						repeat = true;
					} else {
						upfield->animation.level_no = field->animation.level_no++;
						upfield->animation.fall_levels++;
						upfield->animation.falling = Tween(game, 0.0, 1.0, TWEEN_STYLE_BOUNCE_OUT, FALLING_TIME * (1.0 + upfield->animation.level_no * 0.025));
						Swap(game, data, id, up);
					}
				} else {
					CreateNewField(game, data, field);
				}
			}
		}
	} while (repeat);

	data->locked = false;
}

static TM_ACTION(AfterMatching) {
	TM_RunningOnly;
	DoRemoval(game, data);
	Gravity(game, data);
	ProcessFields(game, data);
	return true;
}

static TM_ACTION(DispatchAnimations) {
	TM_RunningOnly;
	AnimateMatching(game, data);
	AnimateRemoval(game, data);
	TM_AddDelay(data->timeline, (int)((MATCHING_TIME + MATCHING_DELAY_TIME) * 1000));
	TM_AddAction(data->timeline, AfterMatching, NULL);
	return true;
}

static TM_ACTION(DoSpawnParticles) {
	TM_RunningOnly;
	struct Field* field = TM_Arg(0);
	int* count = TM_Arg(1);
	SpawnParticles(game, data, field->id, *count);
	free(count);
	return true;
}

static TM_ACTION(AnimateSpecial) {
	TM_RunningOnly;
	struct Field* field = TM_Arg(0);
	field->animation.launching = Tween(game, 0.0, 1.0, TWEEN_STYLE_SINE_IN_OUT, LAUNCHING_TIME);
	return true;
}

static void HandleSpecialed(struct Game* game, struct GamestateResources* data, struct Field* field) {
	TM_WrapArg(int, count, 64);
	TM_AddAction(data->timeline, DoSpawnParticles, TM_Args(field, count));
	TM_AddDelay(data->timeline, 10);
	if (field->type != FIELD_TYPE_FREEFALL) {
		field->to_remove = true;
	}
}

static void LaunchSpecial(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	TM_AddAction(data->timeline, AnimateSpecial, TM_Args(GetField(game, data, id)));

	struct FieldID left = ToLeft(id), right = ToRight(id), top = ToTop(id), bottom = ToBottom(id);
	while (IsValidID(left) || IsValidID(right) || IsValidID(top) || IsValidID(bottom)) {
		struct Field* field = GetField(game, data, left);
		if (field) {
			HandleSpecialed(game, data, field);
		}
		field = GetField(game, data, right);
		if (field) {
			HandleSpecialed(game, data, field);
		}
		field = GetField(game, data, top);
		if (field) {
			HandleSpecialed(game, data, field);
		}
		field = GetField(game, data, bottom);
		if (field) {
			HandleSpecialed(game, data, field);
		}
		left = ToLeft(left);
		right = ToRight(right);
		top = ToTop(top);
		bottom = ToBottom(bottom);
	}
	TM_WrapArg(int, count, 64);
	TM_AddAction(data->timeline, DoSpawnParticles, TM_Args(GetField(game, data, id), count));
}

static bool AnimateSpecials(struct Game* game, struct GamestateResources* data) {
	bool found = false;
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			if (data->fields[i][j].to_remove && !data->fields[i][j].handled) {
				if (data->fields[i][j].type == FIELD_TYPE_ANIMAL && data->fields[i][j].data.animal.special) {
					data->fields[i][j].handled = true;
					LaunchSpecial(game, data, data->fields[i][j].id);
					found = true;
				}
			}
		}
	}
	return found;
}

static void ProcessFields(struct Game* game, struct GamestateResources* data) {
	if (MarkMatching(game, data)) {
		AnimateSpecials(game, data);
		Collect(game, data);
		TM_AddAction(data->timeline, DispatchAnimations, NULL);
	}
}

static bool WillMatch(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two) {
	if (!AreSwappable(game, data, one, two)) {
		return false;
	}
	bool res = false;
	Swap(game, data, one, two);
	if (IsMatching(game, data, two)) {
		res = true;
	}
	Swap(game, data, one, two);
	return res;
}

static bool ShowHint(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			struct FieldID id = {.i = i, .j = j};
			struct FieldID (*callbacks[])(struct FieldID) = {ToLeft, ToRight, ToTop, ToBottom};

			for (int q = 0; q < 4; q++) {
				if (IsValidMove(id, callbacks[q](id))) {
					if (WillMatch(game, data, id, callbacks[q](id))) {
						GetField(game, data, id)->animation.hinting = Tween(game, 0.0, 1.0, TWEEN_STYLE_SINE_IN_OUT, HINT_TIME);
						return true;
					}
				}
			}
		}
	}
	return false;
}

static bool AutoMove(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			struct FieldID id = {.i = i, .j = j};
			struct FieldID (*callbacks[])(struct FieldID) = {ToLeft, ToRight, ToTop, ToBottom};

			for (int q = 0; q < 4; q++) {
				if (IsValidMove(id, callbacks[q](id))) {
					if (WillMatch(game, data, id, callbacks[q](id))) {
						AnimateSwapping(game, data, id, callbacks[q](id));
						return true;
					}
				}
			}
		}
	}
	return false;
}

static void Turn(struct Game* game, struct GamestateResources* data) {
	if (!IsValidMove(data->current, data->hovered)) {
		return;
	}

	PrintConsole(game, "swap %dx%d with %dx%d", data->current.i, data->current.j, data->hovered.i, data->hovered.j);
	data->clicked = false;

	if (!AreSwappable(game, data, data->current, data->hovered)) {
		GetField(game, data, data->current)->animation.shaking = Tween(game, 0.0, 1.0, TWEEN_STYLE_SINE_OUT, SHAKING_TIME);
		return;
	}
	data->locked = true;

	AnimateSwapping(game, data, data->current, data->hovered);

	Swap(game, data, data->current, data->hovered);
	if (!IsMatching(game, data, data->current) && !IsMatching(game, data, data->hovered)) {
		AnimateSwapping(game, data, data->current, data->hovered);
	}
	Swap(game, data, data->current, data->hovered);
}

void Gamestate_ProcessEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev) {
	// Called for each event in Allegro event queue.
	// Here you can handle user input, expiring timers etc.
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		UnloadCurrentGamestate(game); // mark this gamestate to be stopped and unloaded
		// When there are no active gamestates, the engine will quit.
	}

	int offsetY = (int)((game->viewport.height - (ROWS * 90)) / 2.0);
	data->hovered.i = (int)(game->data->mouseX * game->viewport.width / 90);
	data->hovered.j = (int)((game->data->mouseY * game->viewport.height - offsetY) / 90);
	if ((data->hovered.i < 0) || (data->hovered.j < 0) || (data->hovered.i >= COLS) || (data->hovered.j >= ROWS) || (game->data->mouseY * game->viewport.height <= offsetY) || (game->data->mouseX == 0.0)) {
		data->hovered.i = -1;
		data->hovered.j = -1;
	}

	if (data->locked) {
		return;
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) || (ev->type == ALLEGRO_EVENT_TOUCH_BEGIN)) {
		data->current = data->hovered;
		data->clicked = true;
	}

	if (((ev->type == ALLEGRO_EVENT_MOUSE_AXES) || (ev->type == ALLEGRO_EVENT_TOUCH_MOVE)) && (data->clicked)) {
		if (IsValidID(data->current) && IsValidID(data->hovered)) {
			Turn(game, data);
		}
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) || (ev->type == ALLEGRO_EVENT_TOUCH_END)) {
		data->clicked = false;
	}

	if (ev->type == ALLEGRO_EVENT_KEY_DOWN) {
		if (ev->keyboard.keycode == ALLEGRO_KEY_H) {
			ShowHint(game, data);
			return;
		}
		if (ev->keyboard.keycode == ALLEGRO_KEY_A) {
			AutoMove(game, data);
			return;
		}

		int type = ev->keyboard.keycode - ALLEGRO_KEY_1;

		struct Field* field = GetField(game, data, data->hovered);

		if (!field) {
			return;
		}

		if (ev->keyboard.keycode == ALLEGRO_KEY_S) {
			if (field->type != FIELD_TYPE_ANIMAL) {
				return;
			}
			field->data.animal.sleeping = !field->data.animal.sleeping;
			UpdateDrawable(game, data, field->id);
			PrintConsole(game, "Field %dx%d, sleeping = %d", field->id.i, field->id.j, field->data.animal.sleeping);
			return;
		}

		if (ev->keyboard.keycode == ALLEGRO_KEY_D) {
			if (field->type != FIELD_TYPE_ANIMAL) {
				return;
			}
			field->data.animal.special = !field->data.animal.special;
			UpdateDrawable(game, data, field->id);
			PrintConsole(game, "Field %dx%d, special = %d", field->id.i, field->id.j, field->data.animal.special);
			return;
		}

		if (ev->keyboard.keycode == ALLEGRO_KEY_MINUS) {
			Gravity(game, data);
			ProcessFields(game, data);
		}

		if (type == -1) {
			type = 9;
		}
		if (type < 0) {
			return;
		}
		if (type >= ANIMAL_TYPES + FIELD_TYPES) {
			return;
		}
		if (type >= ANIMAL_TYPES) {
			type -= ANIMAL_TYPES - 1;
			if (field->type == (enum FIELD_TYPE)type) {
				field->data.collectible.type++;
				if (field->data.collectible.type == COLLECTIBLE_TYPES) {
					field->data.collectible.type = 0;
				}
			} else {
				field->data.collectible.type = 0;
			}
			field->type = type;
			if (field->type == FIELD_TYPE_FREEFALL) {
				field->data.freefall.variant = rand() % ACTIONS[ANIMAL_TYPES].actions;
			} else {
				field->data.collectible.variant = 0;
			}
			PrintConsole(game, "Setting field type to %d", type);
		} else {
			field->type = FIELD_TYPE_ANIMAL;
			field->data.animal.type = type;
			PrintConsole(game, "Setting animal type to %d", type);
		}
		if (IsDrawable(field->type)) {
			UpdateDrawable(game, data, field->id);
		}
	}
}

void* Gamestate_Load(struct Game* game, void (*progress)(struct Game*)) {
	// Called once, when the gamestate library is being loaded.
	// Good place for allocating memory, loading bitmaps etc.
	//
	// NOTE: Depending on engine configuration, this may be called from a separate thread.
	// Unless you're sure what you're doing, avoid using drawing calls and other things that
	// require main OpenGL context.

	struct GamestateResources* data = calloc(1, sizeof(struct GamestateResources));
	for (unsigned int i = 0; i < sizeof(ANIMALS) / sizeof(ANIMALS[0]); i++) {
		data->archetypes[i] = CreateCharacter(game, ANIMALS[i]);
		RegisterSpritesheet(game, data->archetypes[i], "stand");
		RegisterSpritesheet(game, data->archetypes[i], "blink");
		for (int j = 0; j < ACTIONS[i].actions; j++) {
			RegisterSpritesheet(game, data->archetypes[i], ACTIONS[i].names[j]);
		}
		LoadSpritesheets(game, data->archetypes[i], progress);
	}
	for (unsigned int i = 0; i < sizeof(SPECIALS) / sizeof(SPECIALS[0]); i++) {
		int ii = sizeof(ANIMALS) / sizeof(ANIMALS[0]) + i;
		data->archetypes[ii] = CreateCharacter(game, SPECIALS[i]);
		for (int j = 0; j < ACTIONS[ii].actions; j++) {
			RegisterSpritesheet(game, data->archetypes[ii], ACTIONS[ii].names[j]);
		}
		LoadSpritesheets(game, data->archetypes[ii], progress);
	}

	data->bg = al_load_bitmap(GetDataFilePath(game, "bg.webp"));
	progress(game);

	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->fields[i][j].drawable = CreateCharacter(game, NULL);
			data->fields[i][j].drawable->shared = true;
			data->fields[i][j].overlay = CreateCharacter(game, NULL);
			data->fields[i][j].overlay->shared = true;
			SetParentCharacter(game, data->fields[i][j].overlay, data->fields[i][j].drawable);
			SetCharacterPosition(game, data->fields[i][j].overlay, 108 / 2.0, 108 / 2.0, 0); // FIXME: subcharacters should be positioned by parent pivot
			data->fields[i][j].id.i = i;
			data->fields[i][j].id.j = j;
			data->fields[i][j].matched = false;
		}
	}
	progress(game);

	data->field_bgs[0] = al_load_bitmap(GetDataFilePath(game, "kwadrat1.png"));
	progress(game);
	data->field_bgs[1] = al_load_bitmap(GetDataFilePath(game, "kwadrat2.png"));
	progress(game);
	data->field_bgs[2] = al_load_bitmap(GetDataFilePath(game, "kwadrat3.png"));
	progress(game);
	data->field_bgs[3] = al_load_bitmap(GetDataFilePath(game, "kwadrat4.png"));
	progress(game);

	data->scene = CreateNotPreservedBitmap(game->viewport.width, game->viewport.height);
	data->lowres_scene = CreateNotPreservedBitmap(game->viewport.width / BLUR_DIVIDER, game->viewport.height / BLUR_DIVIDER);
	data->lowres_scene_blur = al_create_bitmap(game->viewport.width / BLUR_DIVIDER, game->viewport.height / BLUR_DIVIDER);
	data->board = CreateNotPreservedBitmap(game->viewport.width, game->viewport.height);
	progress(game);

	data->combine_shader = CreateShader(game, GetDataFilePath(game, "shaders/vertex.glsl"), GetDataFilePath(game, "shaders/combine.glsl"));
	progress(game);
	data->desaturate_shader = CreateShader(game, GetDataFilePath(game, "shaders/vertex.glsl"), GetDataFilePath(game, "shaders/desaturate.glsl"));
	progress(game);

	data->timeline = TM_Init(game, data, "timeline");

	data->particles = CreateParticleBucket(game, 8192, true);

	return data;
}

void Gamestate_PostLoad(struct Game* game, struct GamestateResources* data) {
	data->field_bgs_bmp = al_create_bitmap(88 * 4, 88);
	al_set_target_bitmap(data->field_bgs_bmp);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	for (int i = 0; i < 4; i++) {
		al_draw_bitmap(data->field_bgs[i], i * 88, 0, 0);
		al_destroy_bitmap(data->field_bgs[i]);
		data->field_bgs[i] = al_create_sub_bitmap(data->field_bgs_bmp, i * 88, 0, 88, 88);
	}

	UpdateBlur(game, data);
}

void Gamestate_Unload(struct Game* game, struct GamestateResources* data) {
	// Called when the gamestate library is being unloaded.
	// Good place for freeing all allocated memory and resources.
	DestroyParticleBucket(game, data->particles);
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			DestroyCharacter(game, data->fields[i][j].drawable);
			DestroyCharacter(game, data->fields[i][j].overlay);
		}
	}
	for (unsigned int i = 0; i < sizeof(ANIMALS) / sizeof(ANIMALS[0]); i++) {
		DestroyCharacter(game, data->archetypes[i]);
	}
	for (unsigned int i = 0; i < sizeof(SPECIALS) / sizeof(SPECIALS[0]); i++) {
		int ii = sizeof(ANIMALS) / sizeof(ANIMALS[0]) + i;
		DestroyCharacter(game, data->archetypes[ii]);
	}
	for (int i = 0; i < 4; i++) {
		al_destroy_bitmap(data->field_bgs[i]);
	}
	al_destroy_bitmap(data->field_bgs_bmp);
	al_destroy_bitmap(data->bg);
	al_destroy_bitmap(data->scene);
	al_destroy_bitmap(data->lowres_scene);
	al_destroy_bitmap(data->lowres_scene_blur);
	al_destroy_bitmap(data->board);
	DestroyShader(game, data->combine_shader);
	DestroyShader(game, data->desaturate_shader);
	TM_Destroy(data->timeline);
	free(data);
}

void Gamestate_Start(struct Game* game, struct GamestateResources* data) {
	// Called when this gamestate gets control. Good place for initializing state,
	// playing music etc.
	data->locked = false;
	data->clicked = false;
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			GenerateField(game, data, &data->fields[i][j]);
			data->fields[i][j].animation.hiding = StaticTween(game, 0.0);
			data->fields[i][j].animation.falling = StaticTween(game, 1.0);

			data->fields[i][j].animation.time_to_action = (int)((rand() % 250000 + 500000) * (rand() / (double)RAND_MAX));
			data->fields[i][j].animation.time_to_blink = (int)((rand() % 100000 + 200000) * (rand() / (double)RAND_MAX));
		}
	}
	data->current = (struct FieldID){-1, -1};
	while (MarkMatching(game, data)) {
		DoRemoval(game, data);
		Gravity(game, data);
	}
	StopAnimations(game, data);
}

void Gamestate_Stop(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets stopped. Stop timers, music etc. here.
}

void Gamestate_Pause(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets paused (so only Draw is being called, no Logic nor ProcessEvent)
	// Pause your timers and/or sounds here.
}

void Gamestate_Resume(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets resumed. Resume your timers and/or sounds here.
}

void Gamestate_Reload(struct Game* game, struct GamestateResources* data) {
	// Called when the display gets lost and not preserved bitmaps need to be recreated.
	// Unless you want to support mobile platforms, you should be able to ignore it.
	data->scene = CreateNotPreservedBitmap(game->viewport.width, game->viewport.height);
	data->lowres_scene = CreateNotPreservedBitmap(game->viewport.width / BLUR_DIVIDER, game->viewport.height / BLUR_DIVIDER);
	data->board = CreateNotPreservedBitmap(game->viewport.width, game->viewport.height);
}
