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

static char* ANIMALS[] = {"bee", "bird", "cat", "fish", "frog", "ladybug"};

#define MATCHING_TIME 0.6
#define FALLING_TIME 0.5

#define COLS 8
#define ROWS 8

enum FIELD_TYPE {
	FIELD_TYPE_BEE,
	FIELD_TYPE_BIRD,
	FIELD_TYPE_CAT,
	FIELD_TYPE_FISH,
	FIELD_TYPE_FROG,
	FIELD_TYPE_LADYBUG,

	FIELD_TYPE_ANIMALS,

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
	enum FIELD_TYPE type;
	struct FieldID id;
	bool matched;

	struct Tween hiding, falling;
	int fallLevels;
};

struct GamestateResources {
	// This struct is for every resource allocated and used by your gamestate.
	// It gets created on load and then gets passed around to all other function calls.
	ALLEGRO_FONT* font;
	ALLEGRO_BITMAP* bg;
	struct Character* archetypes[sizeof(ANIMALS) / sizeof(ANIMALS[0])];
	struct FieldID current, hovered;
	struct Field fields[COLS][ROWS];

	struct Timeline* timeline;

	bool locked;
};

int Gamestate_ProgressCount = 8; // number of loading steps as reported by Gamestate_Load

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
		}
	}
}

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	// Called as soon as possible, but no sooner than next Gamestate_Logic call.
	// Draw everything to the screen here.
	al_clear_to_color(al_color_hsl(game->time * 64, 1.0, 0.5));
	al_draw_bitmap(data->bg, 0, 0, 0);

	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			int offsetY = (int)((game->viewport.height - (ROWS * 90)) / 2.0);

			float tint = 1.0 - GetTweenValue(&data->fields[i][j].hiding);
			data->fields[i][j].animal->tint = al_map_rgba_f(tint, tint, tint, tint);

			int levelDiff = data->fields[i][j].fallLevels * 90 * (1.0 - GetTweenValue(&data->fields[i][j].falling));

			SetCharacterPosition(game, data->fields[i][j].animal, i * 90 + 45, j * 90 + 45 + offsetY - levelDiff, 0);

			bool hovered = IsSameID(data->hovered, (struct FieldID){i, j});
			ALLEGRO_COLOR color = hovered ? al_map_rgba(160, 160, 160, 160) : al_map_rgba(92, 92, 92, 92);
			if (IsSameID(data->current, (struct FieldID){i, j})) {
				color = al_map_rgba(222, 222, 222, 222);
			}
			if (data->locked) {
				color = al_map_rgba(92, 92, 92, 92);
			}
			if (data->fields[i][j].type != FIELD_TYPE_DISABLED) {
				al_draw_filled_rectangle(i * 90 + 2, j * 90 + offsetY + 2, (i + 1) * 90 - 2, (j + 1) * 90 + offsetY - 2, color);
			}
			if (data->fields[i][j].type < FIELD_TYPE_ANIMALS) {
				DrawCharacter(game, data->fields[i][j].animal);
			}
		}
	}
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

static int IsMatching(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	int lchain = 0, tchain = 0;
	struct Field* orig = GetField(game, data, id);
	if (orig->type >= FIELD_TYPE_ANIMALS) {
		return 0;
	}
	struct FieldID pos = ToLeft(id);
	while (IsValidID(pos)) {
		struct Field* field = GetField(game, data, pos);
		if (field->type != orig->type) {
			break;
		}
		lchain++;
		pos = ToLeft(pos);
	}
	pos = ToRight(id);
	while (IsValidID(pos)) {
		struct Field* field = GetField(game, data, pos);
		if (field->type != orig->type) {
			break;
		}
		lchain++;
		pos = ToRight(pos);
	}

	pos = ToTop(id);
	while (IsValidID(pos)) {
		struct Field* field = GetField(game, data, pos);
		if (field->type != orig->type) {
			break;
		}
		tchain++;
		pos = ToTop(pos);
	}
	pos = ToBottom(id);
	while (IsValidID(pos)) {
		struct Field* field = GetField(game, data, pos);
		if (field->type != orig->type) {
			break;
		}
		tchain++;
		pos = ToBottom(pos);
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

static void AnimateMatching(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			if (data->fields[i][j].matched) {
				data->fields[i][j].hiding = Tween(game, 0.0, 1.0, TWEEN_STYLE_LINEAR, MATCHING_TIME);
				data->locked = true;
			}
		}
	}
}

static void EmptyMatching(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->fields[i][j].fallLevels = 0;
			if (data->fields[i][j].matched) {
				data->fields[i][j].type = FIELD_TYPE_EMPTY;
				data->fields[i][j].matched = false;
				data->fields[i][j].hiding = Tween(game, 0.0, 0.0, TWEEN_STYLE_LINEAR, 0.0);
				data->fields[i][j].falling = Tween(game, 1.0, 1.0, TWEEN_STYLE_LINEAR, 0.0);
			}
		}
	}
}

static void Swap(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two) {
	struct Field tmp = data->fields[one.i][one.j];
	data->fields[one.i][one.j] = data->fields[two.i][two.j];
	data->fields[two.i][two.j] = tmp;
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
				if (IsValidID(up)) {
					struct Field* upfield = GetField(game, data, up);
					if (upfield->type == FIELD_TYPE_EMPTY) {
						repeat = true;
					} else {
						upfield->fallLevels++;
						upfield->falling = Tween(game, 0.0, 1.0, TWEEN_STYLE_BOUNCE_OUT, FALLING_TIME);
						Swap(game, data, id, up);
					}
				} else {
					field->type = rand() % FIELD_TYPE_ANIMALS;
					field->animal->spritesheets = data->archetypes[field->type]->spritesheets;
					SelectSpritesheet(game, field->animal, "stand");
					field->fallLevels++;
					field->falling = Tween(game, 0.0, 1.0, TWEEN_STYLE_BOUNCE_OUT, FALLING_TIME);
					field->hiding = Tween(game, 1.0, 0.0, TWEEN_STYLE_LINEAR, 0.25);
				}
			}
		}
	} while (repeat);

	data->locked = false;
}

static void ProcessFields(struct Game* game, struct GamestateResources* data);

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
		TM_AddDelay(data->timeline, MATCHING_TIME * 1000);
		TM_AddAction(data->timeline, AfterMatching, NULL);
	}
}

static void Turn(struct Game* game, struct GamestateResources* data) {
	if (!IsValidMove(data->current, data->hovered)) {
		return;
	}

	PrintConsole(game, "swap %dx%d with %dx%d", data->current.i, data->current.j, data->hovered.i, data->hovered.j);
	Swap(game, data, data->current, data->hovered);

	if (!IsMatching(game, data, data->current) && !IsMatching(game, data, data->hovered)) {
		Swap(game, data, data->current, data->hovered);
	}
	ProcessFields(game, data);
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
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) || (ev->type == ALLEGRO_EVENT_TOUCH_END)) {
		if (IsValidID(data->current) && IsValidID(data->hovered)) {
			Turn(game, data);
		}
	}

	if (ev->type == ALLEGRO_EVENT_KEY_DOWN) {
		int type = ev->keyboard.keycode - ALLEGRO_KEY_1;

		if (type < 0) {
			return;
		}
		if (type >= FIELD_TYPE_ANIMALS) {
			type++;
		}
		if (type >= FIELD_TYPES) {
			return;
		}

		struct Field* field = GetField(game, data, data->hovered);

		if (!field) {
			return;
		}

		field->type = type;

		if (type < FIELD_TYPE_ANIMALS) {
			field->animal->spritesheets = data->archetypes[type]->spritesheets;
			SelectSpritesheet(game, field->animal, "stand");
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
		LoadSpritesheets(game, data->archetypes[i], progress);
	}

	data->bg = al_load_bitmap(GetDataFilePath(game, "bg.webp"));
	progress(game);

	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->fields[i][j].animal = CreateCharacter(game, ANIMALS[rand() % 6]);
			data->fields[i][j].animal->shared = true;
			data->fields[i][j].animal->scaleX = 0.18;
			data->fields[i][j].animal->scaleY = 0.18;
			data->fields[i][j].id.i = i;
			data->fields[i][j].id.j = j;
			data->fields[i][j].matched = false;
		}
	}

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
	}
	al_destroy_bitmap(data->bg);
	al_destroy_font(data->font);
	TM_Destroy(data->timeline);
	free(data);
}

void Gamestate_Start(struct Game* game, struct GamestateResources* data) {
	// Called when this gamestate gets control. Good place for initializing state,
	// playing music etc.
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->fields[i][j].type = rand() % FIELD_TYPE_ANIMALS;
			data->fields[i][j].animal->spritesheets = data->archetypes[data->fields[i][j].type]->spritesheets;
			SelectSpritesheet(game, data->fields[i][j].animal, "stand");
			data->fields[i][j].hiding = Tween(game, 0.0, 0.0, TWEEN_STYLE_LINEAR, 0.0);
			data->fields[i][j].falling = Tween(game, 1.0, 1.0, TWEEN_STYLE_LINEAR, 0.0);
		}
	}
	data->current = (struct FieldID){-1, -1};
	while (MarkMatching(game, data)) {
		EmptyMatching(game, data);
		Gravity(game, data);
	}
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
}
