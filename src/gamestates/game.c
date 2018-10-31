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

#define COLS 8
#define ROWS 12

enum FIELD_TYPE {
	FIELD_TYPE_BEE,
	FIELD_TYPE_BIRD,
	FIELD_TYPE_CAT,
	FIELD_TYPE_FISH,
	FIELD_TYPE_FROG,
	FIELD_TYPE_LADYBUG,

	FIELD_TYPE_ANIMALS,

	FIELD_TYPE_EMPTY,
	FIELD_TYPE_DISABLED
};

struct FieldID {
	int i;
	int j;
};

struct Field {
	struct Character* animal;
	enum FIELD_TYPE type;
	struct FieldID id;
};

struct GamestateResources {
	// This struct is for every resource allocated and used by your gamestate.
	// It gets created on load and then gets passed around to all other function calls.
	ALLEGRO_FONT* font;
	ALLEGRO_BITMAP* bg;
	struct Character* archetypes[sizeof(ANIMALS) / sizeof(ANIMALS[0])];
	struct FieldID current, hovered;
	struct Field fields[COLS][ROWS];
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
}

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	// Called as soon as possible, but no sooner than next Gamestate_Logic call.
	// Draw everything to the screen here.
	al_clear_to_color(al_color_hsl(game->time * 64, 1.0, 0.5));
	al_draw_bitmap(data->bg, 0, 0, 0);

	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 12; j++) {
			SetCharacterPosition(game, data->fields[i][j].animal, i * 90 + 45, j * 90 + 45 + 180, 0);
			bool hovered = IsSameID(data->hovered, (struct FieldID){i, j});
			ALLEGRO_COLOR color = hovered ? al_map_rgba(160, 160, 160, 160) : al_map_rgba(92, 92, 92, 92);
			if (IsSameID(data->current, (struct FieldID){i, j})) {
				color = al_map_rgba(222, 222, 222, 222);
			}
			al_draw_filled_rectangle(i * 90 + 2, j * 90 + 180 + 2, (i + 1) * 90 - 2, (j + 1) * 90 + 180 - 2, color);
			DrawCharacter(game, data->fields[i][j].animal);
		}
	}
}

static void Swap(struct Game* game, struct GamestateResources* data) {
	PrintConsole(game, "swap %dx%d with %dx%d", data->current.i, data->current.j, data->hovered.i, data->hovered.j);
	struct Field tmp = data->fields[data->hovered.i][data->hovered.j];
	data->fields[data->hovered.i][data->hovered.j] = data->fields[data->current.i][data->current.j];
	data->fields[data->current.i][data->current.j] = tmp;
}

void Gamestate_ProcessEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev) {
	// Called for each event in Allegro event queue.
	// Here you can handle user input, expiring timers etc.
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		UnloadCurrentGamestate(game); // mark this gamestate to be stopped and unloaded
		// When there are no active gamestates, the engine will quit.
	}

	data->hovered.i = (int)(game->data->mouseX * game->viewport.width / 90);
	data->hovered.j = (int)((game->data->mouseY * game->viewport.height - 180) / 90);
	if ((data->hovered.i < 0) || (data->hovered.j < 0) || (data->hovered.i >= COLS) || (data->hovered.j >= ROWS) || (game->data->mouseY * game->viewport.height < 180)) {
		data->hovered.i = -1;
		data->hovered.j = -1;
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) || (ev->type == ALLEGRO_EVENT_TOUCH_BEGIN)) {
		data->current = data->hovered;
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) || (ev->type == ALLEGRO_EVENT_TOUCH_END)) {
		if (IsValidID(data->current) && IsValidID(data->hovered)) {
			Swap(game, data);
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
			data->fields[i][j].animal->scaleX = 0.085;
			data->fields[i][j].animal->scaleY = 0.085;
			data->fields[i][j].id.i = i;
			data->fields[i][j].id.j = j;
		}
	}

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
		}
	}
	data->current = (struct FieldID){-1, -1};
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
