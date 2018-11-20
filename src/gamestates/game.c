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

#define BLUR_DIVIDER 8

#define COLS 8
#define ROWS 8

static char* ANIMALS[] = {"bee", "bird", "cat", "fish", "frog", "ladybug"};
static char* SPECIALS[] = {"egg"};

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
	{.actions = 0, .names = {}}, // egg
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

enum FIELD_TYPE {
	FIELD_TYPE_ANIMAL,
	FIELD_TYPE_FREEFALL,
	FIELD_TYPE_EMPTY,
	FIELD_TYPE_DISABLED,

	FIELD_TYPES
};

struct FieldID {
	int i;
	int j;
};

struct Field {
	struct Character* animal;
	enum ANIMAL_TYPE animal_type;
	enum FIELD_TYPE type;
	struct FieldID id;
	bool matched;
	bool sleeping;

	struct Tween hiding, falling, swapping;
	struct FieldID swapee;
	int fall_levels, level_no;

	int time_to_action, action_time;
	int time_to_blink, blink_time;
};

struct GamestateResources {
	// This struct is for every resource allocated and used by your gamestate.
	// It gets created on load and then gets passed around to all other function calls.
	ALLEGRO_FONT* font;
	ALLEGRO_BITMAP* bg;

	ALLEGRO_BITMAP *scene, *lowres_scene, *lowres_scene_blur, *board;

	struct Character* archetypes[sizeof(ANIMALS) / sizeof(ANIMALS[0]) + sizeof(SPECIALS) / sizeof(SPECIALS[0])];
	struct FieldID current, hovered;
	struct Field fields[COLS][ROWS];

	struct Timeline* timeline;

	ALLEGRO_BITMAP* field_bgs[4];

	ALLEGRO_SHADER *combine_shader, *kawese_shader, *desaturate_shader;

	bool locked, clicked;
};

int Gamestate_ProgressCount = 47; // number of loading steps as reported by Gamestate_Load

static void ProcessFields(struct Game* game, struct GamestateResources* data);

static inline bool IsDrawable(enum FIELD_TYPE type) {
	return (type == FIELD_TYPE_ANIMAL) || (type == FIELD_TYPE_FREEFALL);
}

static inline bool IsSameID(struct FieldID one, struct FieldID two) {
	return one.i == two.i && one.j == two.j;
}

static inline bool IsValidID(struct FieldID id) {
	return id.i != -1 && id.j != -1;
}

void Gamestate_Logic(struct Game* game, struct GamestateResources* data, double delta) {
	// Called 60 times per second (by default). Here you should do all your game logic.
	TM_Process(data->timeline, delta);
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			AnimateCharacter(game, data->fields[i][j].animal, delta, 1.0);
			UpdateTween(&data->fields[i][j].falling, delta);
			UpdateTween(&data->fields[i][j].hiding, delta);
			UpdateTween(&data->fields[i][j].swapping, delta);

			if (data->fields[i][j].blink_time) {
				data->fields[i][j].blink_time -= (int)(delta * 1000);
				if (data->fields[i][j].blink_time <= 0) {
					data->fields[i][j].blink_time = 0;
					if (data->fields[i][j].type == FIELD_TYPE_ANIMAL) {
						SelectSpritesheet(game, data->fields[i][j].animal, "stand");
					}
				}
			} else if (data->fields[i][j].action_time) {
				data->fields[i][j].action_time -= (int)(delta * 1000);
				if (data->fields[i][j].action_time <= 0) {
					data->fields[i][j].action_time = 0;
					if (data->fields[i][j].type == FIELD_TYPE_ANIMAL) {
						SelectSpritesheet(game, data->fields[i][j].animal, "stand");
					}
				}
			} else {
				data->fields[i][j].time_to_action -= (int)(delta * 1000);
				data->fields[i][j].time_to_blink -= (int)(delta * 1000);

				if (data->fields[i][j].time_to_action <= 0) {
					data->fields[i][j].time_to_action = rand() % 250000 + 500000;
					data->fields[i][j].action_time = rand() % 2000 + 1000;
					if (data->fields[i][j].type == FIELD_TYPE_ANIMAL) {
						SelectSpritesheet(game, data->fields[i][j].animal, ACTIONS[data->fields[i][j].animal_type].names[rand() % ACTIONS[data->fields[i][j].animal_type].actions]);
					}
				}

				if (data->fields[i][j].time_to_blink <= 0) {
					if (strcmp(data->fields[i][j].animal->spritesheet->name, "stand") == 0) {
						if (data->fields[i][j].type == FIELD_TYPE_ANIMAL) {
							SelectSpritesheet(game, data->fields[i][j].animal, "blink");
						}
						data->fields[i][j].time_to_blink = rand() % 100000 + 200000;
						data->fields[i][j].blink_time = rand() % 400 + 100;
					}
				}
			}
		}
	}
}

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	// Called as soon as possible, but no sooner than next Gamestate_Logic call.
	// Draw everything to the screen here.
	al_set_target_bitmap(data->scene);
	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_draw_bitmap(data->bg, 0, 0, 0);

	al_set_target_bitmap(data->lowres_scene);
	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_draw_scaled_bitmap(data->scene, 0, 0, al_get_bitmap_width(data->scene), al_get_bitmap_height(data->scene),
		0, 0, al_get_bitmap_width(data->lowres_scene), al_get_bitmap_height(data->lowres_scene), 0);

	float size[2] = {al_get_bitmap_width(data->lowres_scene), al_get_bitmap_height(data->lowres_scene)};
	int offsetY = (int)((game->viewport.height - (ROWS * 90)) / 2.0);

	al_set_target_bitmap(data->lowres_scene_blur);
	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_set_clipping_rectangle(0, offsetY / BLUR_DIVIDER - 10, game->viewport.width / BLUR_DIVIDER, (game->viewport.height - offsetY * 2) / BLUR_DIVIDER + 20);
	al_use_shader(data->kawese_shader);
	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_set_shader_float_vector("size", 2, size, 1);
	al_set_shader_float("kernel", 0);
	al_draw_bitmap(data->lowres_scene, 0, 0, 0);

	al_set_target_bitmap(data->lowres_scene);
	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_set_clipping_rectangle(0, offsetY / BLUR_DIVIDER - 10, game->viewport.width / BLUR_DIVIDER, (game->viewport.height - offsetY * 2) / BLUR_DIVIDER + 20);
	al_use_shader(data->kawese_shader);
	al_set_shader_float_vector("size", 2, size, 1);
	al_set_shader_float("kernel", 0);
	al_draw_bitmap(data->lowres_scene_blur, 0, 0, 0);

	al_set_target_bitmap(data->board);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	al_set_clipping_rectangle(0, offsetY, game->viewport.width, game->viewport.height - offsetY * 2);
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

	al_use_shader(data->desaturate_shader);
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			float tint = 1.0 - GetTweenValue(&data->fields[i][j].hiding);
			data->fields[i][j].animal->tint = al_map_rgba_f(tint, tint, tint, tint);

			int levels = data->fields[i][j].fall_levels;
			int level_no = data->fields[i][j].level_no;
			float tween = Interpolate(GetTweenPosition(&data->fields[i][j].falling), TWEEN_STYLE_EXPONENTIAL_OUT) * (0.5 - level_no * 0.1) +
				sqrt(Interpolate(GetTweenPosition(&data->fields[i][j].falling), TWEEN_STYLE_BOUNCE_OUT)) * (0.5 + level_no * 0.1);

			//			tween = Interpolate(GetTweenPosition(&data->fields[i][j].falling), TWEEN_STYLE_ELASTIC_OUT);
			int levelDiff = (int)(levels * 90 * (1.0 - tween));

			int x = i * 90 + 45, y = j * 90 + 45 + offsetY - levelDiff;
			int swapeeX = data->fields[i][j].swapee.i * 90 + 45, swapeeY = data->fields[i][j].swapee.j * 90 + 45 + offsetY;

			SetCharacterPosition(game, data->fields[i][j].animal, Lerp(x, swapeeX, GetTweenValue(&data->fields[i][j].swapping)), Lerp(y, swapeeY, GetTweenValue(&data->fields[i][j].swapping)), 0);

			if (IsDrawable(data->fields[i][j].type)) {
				al_set_shader_bool("enabled", data->fields[i][j].sleeping);
				DrawCharacter(game, data->fields[i][j].animal);
			}
		}
	}
	al_use_shader(NULL);

	SetFramebufferAsTarget(game);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	al_draw_bitmap(data->scene, 0, 0, 0);

	al_use_shader(data->combine_shader);
	al_set_shader_sampler("tex_bg", data->lowres_scene, 1);
	al_set_shader_float_vector("size", 2, size, 1);
	al_draw_bitmap(data->board, 0, 0, 0);
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

static inline void UpdateDrawable(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	struct Field* field = GetField(game, data, id);
	if (!IsDrawable(field->type)) {
		return;
	}
	int index = field->animal_type;
	if (field->type != FIELD_TYPE_ANIMAL) {
		index = ANIMAL_TYPES + field->type - 1;
	}
	free(field->animal->name);
	field->animal->name = strdup(data->archetypes[index]->name);
	field->animal->spritesheets = data->archetypes[index]->spritesheets;
	SelectSpritesheet(game, field->animal, "stand");
}

static int IsMatching(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	int lchain = 0, tchain = 0;
	struct Field* orig = GetField(game, data, id);
	if (orig->type == FIELD_TYPE_FREEFALL) {
		if (id.j == ROWS - 1) {
			return 1;
		}
	}
	if (orig->type != FIELD_TYPE_ANIMAL) {
		return 0;
	}
	if (orig->sleeping) {
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
			if (field->animal_type != orig->animal_type) {
				break;
			}
			if (field->sleeping) {
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
				matching++;
			}
		}
	}
	PrintConsole(game, "matching %d", matching);
	return matching;
}

static bool AreAdjacentMatching(struct Game* game, struct GamestateResources* data, struct FieldID id, struct FieldID (*func)(struct FieldID)) {
	for (int i = 0; i < 3; i++) {
		id = func(id);
		if (!IsValidID(id) || !GetField(game, data, id)->matched) {
			return false;
		}
	}
	return true;
}

static int ShouldWakeUp(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	return AreAdjacentMatching(game, data, id, ToTop) || AreAdjacentMatching(game, data, id, ToBottom) || AreAdjacentMatching(game, data, id, ToLeft) || AreAdjacentMatching(game, data, id, ToRight);
}

static void AnimateMatching(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			if (data->fields[i][j].matched) {
				SelectSpritesheet(game, data->fields[i][j].animal, ACTIONS[data->fields[i][j].animal_type].names[rand() % ACTIONS[data->fields[i][j].type].actions]);
				data->fields[i][j].hiding = Tween(game, 0.0, 1.0, TWEEN_STYLE_LINEAR, MATCHING_TIME);
				data->fields[i][j].hiding.predelay = MATCHING_DELAY_TIME;
				data->locked = true;
			}

			if (data->fields[i][j].sleeping && ShouldWakeUp(game, data, data->fields[i][j].id)) {
				data->fields[i][j].sleeping = false;
			}
		}
	}
}

static void EmptyMatching(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->fields[i][j].fall_levels = 0;
			data->fields[i][j].level_no = 0;
			if (data->fields[i][j].matched) {
				data->fields[i][j].type = FIELD_TYPE_EMPTY;
				data->fields[i][j].matched = false;
				data->fields[i][j].hiding = StaticTween(game, 0.0);
				data->fields[i][j].falling = StaticTween(game, 1.0);
			}
		}
	}
}

static void StopAnimations(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->fields[i][j].fall_levels = 0;
			data->fields[i][j].level_no = 0;
			data->fields[i][j].hiding = StaticTween(game, 0.0);
			data->fields[i][j].falling = StaticTween(game, 1.0);
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
	one->swapping = StaticTween(game, 0.0);
	two->swapping = StaticTween(game, 0.0);
	data->locked = false;
	ProcessFields(game, data);
	return true;
}

static TM_ACTION(StartSwapping) {
	TM_RunningOnly;
	struct Field* one = TM_GetArg(action->arguments, 0);
	struct Field* two = TM_GetArg(action->arguments, 1);
	data->locked = true;
	one->swapping = Tween(game, 0.0, 1.0, TWEEN_STYLE_SINE_IN_OUT, SWAPPING_TIME);
	one->swapee = two->id;
	two->swapping = Tween(game, 0.0, 1.0, TWEEN_STYLE_SINE_IN_OUT, SWAPPING_TIME);
	two->swapee = one->id;
	return true;
}

static bool IsSwappable(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	struct Field* field = GetField(game, data, id);
	if ((field->type == FIELD_TYPE_ANIMAL) && (!field->sleeping)) {
		return true;
	}
	return false;
}

static bool AreSwappable(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two) {
	return IsSwappable(game, data, one) && IsSwappable(game, data, two);
}

static void AnimateSwapping(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two) {
	TM_AddAction(data->timeline, StartSwapping, TM_AddToArgs(NULL, 2, GetField(game, data, one), GetField(game, data, two)));
	TM_AddDelay(data->timeline, SWAPPING_TIME * 1000);
	TM_AddAction(data->timeline, AfterSwapping, TM_AddToArgs(NULL, 2, GetField(game, data, one), GetField(game, data, two)));
}

static void GenerateField(struct Game* game, struct GamestateResources* data, struct Field* field) {
	if (rand() / (float)RAND_MAX < 0.01) {
		field->type = FIELD_TYPE_FREEFALL;
	} else {
		field->type = FIELD_TYPE_ANIMAL;
		field->animal_type = rand() % ANIMAL_TYPES;
	}
	UpdateDrawable(game, data, field->id);
}

static void CreateNewField(struct Game* game, struct GamestateResources* data, struct Field* field) {
	GenerateField(game, data, field);
	field->fall_levels++;
	field->falling = Tween(game, 0.0, 1.0, TWEEN_STYLE_BOUNCE_OUT, FALLING_TIME * (1.0 + field->level_no * 0.025));
	field->hiding = Tween(game, 1.0, 0.0, TWEEN_STYLE_LINEAR, 0.25);
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
						upfield->level_no = field->level_no++;
						upfield->fall_levels++;
						upfield->falling = Tween(game, 0.0, 1.0, TWEEN_STYLE_BOUNCE_OUT, FALLING_TIME * (1.0 + upfield->level_no * 0.025));
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
	EmptyMatching(game, data);
	Gravity(game, data);
	ProcessFields(game, data);
	return true;
}

static void ProcessFields(struct Game* game, struct GamestateResources* data) {
	if (MarkMatching(game, data)) {
		AnimateMatching(game, data);
		TM_AddDelay(data->timeline, (int)((MATCHING_TIME + MATCHING_DELAY_TIME) * 1000));
		TM_AddAction(data->timeline, AfterMatching, NULL);
	}
}

static void Turn(struct Game* game, struct GamestateResources* data) {
	if (!IsValidMove(data->current, data->hovered)) {
		return;
	}

	PrintConsole(game, "swap %dx%d with %dx%d", data->current.i, data->current.j, data->hovered.i, data->hovered.j);

	if (!AreSwappable(game, data, data->current, data->hovered)) {
		return;
	}
	data->clicked = false;
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

	if (((ev->type == ALLEGRO_EVENT_MOUSE_AXES) && (data->clicked)) || (ev->type == ALLEGRO_EVENT_TOUCH_MOVE)) {
		if (IsValidID(data->current) && IsValidID(data->hovered)) {
			Turn(game, data);
		}
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) || (ev->type == ALLEGRO_EVENT_TOUCH_END)) {
		data->clicked = false;
	}

	if (ev->type == ALLEGRO_EVENT_KEY_DOWN) {
		int type = ev->keyboard.keycode - ALLEGRO_KEY_1;

		struct Field* field = GetField(game, data, data->hovered);

		if (!field) {
			return;
		}

		if (ev->keyboard.keycode == ALLEGRO_KEY_S) {
			field->sleeping = !field->sleeping;
			PrintConsole(game, "Field %dx%d, sleeping = %d", field->id.i, field->id.j, field->sleeping);
			return;
		}

		if (type == -1) {
			ProcessFields(game, data);
		}
		if (type < 0) {
			return;
		}
		if (type >= ANIMAL_TYPES + FIELD_TYPES) {
			return;
		}
		if (type >= ANIMAL_TYPES) {
			type -= ANIMAL_TYPES - 1;
			field->type = type;
			PrintConsole(game, "Setting field type to %d", type);
		} else {
			field->type = FIELD_TYPE_ANIMAL;
			field->animal_type = type;
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
	data->font = al_load_ttf_font(GetDataFilePath(game, "fonts/DejaVuSansMono.ttf"), 42, 0);
	progress(game); // report that we progressed with the loading, so the engine can move a progress bar

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
		RegisterSpritesheet(game, data->archetypes[ii], "stand");
		//RegisterSpritesheet(game, data->archetypes[ii], "blink");
		for (int j = 0; j < ACTIONS[ii].actions; j++) {
			RegisterSpritesheet(game, data->archetypes[ii], ACTIONS[ii].names[j]);
		}
		LoadSpritesheets(game, data->archetypes[ii], progress);
	}

	data->bg = al_load_bitmap(GetDataFilePath(game, "bg.webp"));
	progress(game);

	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->fields[i][j].animal = CreateCharacter(game, ANIMALS[rand() % 6]);
			data->fields[i][j].animal->shared = true;
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
	data->lowres_scene_blur = CreateNotPreservedBitmap(game->viewport.width / BLUR_DIVIDER, game->viewport.height / BLUR_DIVIDER);
	data->board = CreateNotPreservedBitmap(game->viewport.width, game->viewport.height);
	progress(game);

	data->combine_shader = CreateShader(game, GetDataFilePath(game, "shaders/vertex.glsl"), GetDataFilePath(game, "shaders/combine.glsl"));
	progress(game);
	data->kawese_shader = CreateShader(game, GetDataFilePath(game, "shaders/vertex.glsl"), GetDataFilePath(game, "shaders/kawese.glsl"));
	progress(game);
	data->desaturate_shader = CreateShader(game, GetDataFilePath(game, "shaders/vertex.glsl"), GetDataFilePath(game, "shaders/desaturate.glsl"));
	progress(game);

	data->timeline = TM_Init(game, data, "timeline");

	return data;
}

void Gamestate_Unload(struct Game* game, struct GamestateResources* data) {
	// Called when the gamestate library is being unloaded.
	// Good place for freeing all allocated memory and resources.
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			DestroyCharacter(game, data->fields[i][j].animal);
		}
	}
	for (unsigned int i = 0; i < sizeof(ANIMALS) / sizeof(ANIMALS[0]); i++) {
		DestroyCharacter(game, data->archetypes[i]);
	}
	for (unsigned int i = 0; i < sizeof(SPECIALS) / sizeof(SPECIALS[0]); i++) {
		int ii = sizeof(ANIMALS) / sizeof(ANIMALS[0]) + i;
		DestroyCharacter(game, data->archetypes[ii]);
	}
	al_destroy_bitmap(data->bg);
	al_destroy_font(data->font);
	al_destroy_bitmap(data->scene);
	al_destroy_bitmap(data->lowres_scene);
	al_destroy_bitmap(data->lowres_scene_blur);
	al_destroy_bitmap(data->board);
	DestroyShader(game, data->combine_shader);
	DestroyShader(game, data->kawese_shader);
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
			data->fields[i][j].hiding = StaticTween(game, 0.0);
			data->fields[i][j].falling = StaticTween(game, 1.0);

			data->fields[i][j].time_to_action = (int)((rand() % 250000 + 500000) * (rand() / (double)RAND_MAX));
			data->fields[i][j].time_to_blink = (int)((rand() % 100000 + 200000) * (rand() / (double)RAND_MAX));
		}
	}
	data->current = (struct FieldID){-1, -1};
	while (MarkMatching(game, data)) {
		EmptyMatching(game, data);
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
	data->lowres_scene_blur = CreateNotPreservedBitmap(game->viewport.width / BLUR_DIVIDER, game->viewport.height / BLUR_DIVIDER);
	data->board = CreateNotPreservedBitmap(game->viewport.width, game->viewport.height);
}
