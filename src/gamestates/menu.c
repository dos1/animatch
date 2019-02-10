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
#include <libsuperderpy.h>

struct GamestateResources {
	// This struct is for every resource allocated and used by your gamestate.
	// It gets created on load and then gets passed around to all other function calls.
	ALLEGRO_BITMAP *bg, *logo, *frame, *framebg, *leaf;
	struct Character *beetle, *ui;
};

int Gamestate_ProgressCount = 7; // number of loading steps as reported by Gamestate_Load; 0 when missing

void Gamestate_Logic(struct Game* game, struct GamestateResources* data, double delta) {
	// Here you should do all your game logic as if <delta> seconds have passed.
	AnimateCharacter(game, data->beetle, delta, 1.0);
}

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	// Draw everything to the screen here.
	al_draw_bitmap(data->bg, 0, 0, 0);
	al_draw_bitmap(data->logo, 54, 35, 0);

	al_draw_bitmap(data->framebg, 0, 0, 0);
	al_draw_bitmap(data->frame, 0, 0, 0);

	al_draw_bitmap(data->leaf, -32, 1083, 0);
	DrawCharacter(game, data->beetle);
	DrawUIElement(game, data->ui, UI_ELEMENT_NOTE);
	DrawUIElement(game, data->ui, UI_ELEMENT_SETTINGS);
	DrawUIElement(game, data->ui, UI_ELEMENT_ABOUT);
}

void Gamestate_ProcessEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev) {
	// Called for each event in Allegro event queue.
	// Here you can handle user input, expiring timers etc.
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		UnloadAllGamestates(game); // mark this gamestate to be stopped and unloaded
		// When there are no active gamestates, the engine will quit.
	}
	if ((ev->type == ALLEGRO_EVENT_TOUCH_BEGIN) || (ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)) {
		StopCurrentGamestate(game);
		StartGamestate(game, "game");
	}
}

void* Gamestate_Load(struct Game* game, void (*progress)(struct Game*)) {
	// Called once, when the gamestate library is being loaded.
	// Good place for allocating memory, loading bitmaps etc.
	//
	// NOTE: There's no OpenGL context available here. If you want to prerender something,
	// create VBOs, etc. do it in Gamestate_PostLoad.

	struct GamestateResources* data = calloc(1, sizeof(struct GamestateResources));
	data->bg = al_load_bitmap(GetDataFilePath(game, "bg.webp"));
	progress(game); // report that we progressed with the loading, so the engine can move a progress bar

	data->logo = al_load_bitmap(GetDataFilePath(game, "logo.webp"));
	progress(game);

	data->frame = al_load_bitmap(GetDataFilePath(game, "frame.webp"));
	progress(game);

	data->framebg = al_load_bitmap(GetDataFilePath(game, "frame_bg.webp"));
	progress(game);

	data->leaf = al_load_bitmap(GetDataFilePath(game, "leaf.webp"));
	progress(game);

	data->ui = CreateCharacter(game, "ui");
	RegisterSpritesheet(game, data->ui, "ui");
	LoadSpritesheets(game, data->ui, progress);
	SelectSpritesheet(game, data->ui, "ui");

	data->beetle = CreateCharacter(game, "beetle");
	RegisterSpritesheet(game, data->beetle, "beetle");
	LoadSpritesheets(game, data->beetle, progress);
	SelectSpritesheet(game, data->beetle, "beetle");

	return data;
}

void Gamestate_Unload(struct Game* game, struct GamestateResources* data) {
	// Called when the gamestate library is being unloaded.
	// Good place for freeing all allocated memory and resources.
	al_destroy_bitmap(data->bg);
	al_destroy_bitmap(data->logo);
	DestroyCharacter(game, data->ui);
	DestroyCharacter(game, data->beetle);
	free(data);
}

void Gamestate_Start(struct Game* game, struct GamestateResources* data) {
	// Called when this gamestate gets control. Good place for initializing state,
	// playing music etc.
	SetCharacterPosition(game, data->beetle, 0, 1194, 0);
	SetCharacterPosition(game, data->ui, 0, 0, 0);
}

void Gamestate_Stop(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets stopped. Stop timers, music etc. here.
}

// Optional endpoints:

void Gamestate_PostLoad(struct Game* game, struct GamestateResources* data) {
	// This is called in the main thread after Gamestate_Load has ended.
	// Use it to prerender bitmaps, create VBOs, etc.
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
