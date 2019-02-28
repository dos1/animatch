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
	ALLEGRO_BITMAP *bg, *logo, *frame, *framebg, *leaf, *leaf1, *leaf2;
	ALLEGRO_FONT* font;
	struct Character *beetle, *ui;

	double menu_pos;
	double menu_speed;
	double menu_offset;
	bool menu_pressed, menu_triggered;
	double menu_lasttime;
	double menu_lasty;
};

int Gamestate_ProgressCount = 10; // number of loading steps as reported by Gamestate_Load; 0 when missing

void Gamestate_Logic(struct Game* game, struct GamestateResources* data, double delta) {
	// Here you should do all your game logic as if <delta> seconds have passed.
	AnimateCharacter(game, data->beetle, delta, 1.0);
}
void Gamestate_Tick(struct Game* game, struct GamestateResources* data) {
	if (!data->menu_pressed) {
		data->menu_speed *= 0.95;
		data->menu_pos += data->menu_speed * game->viewport.height;

		if (data->menu_pos < 0) {
			data->menu_pos *= 0.75;
			data->menu_speed *= 0.2;
		}
		if (data->menu_pos > 5800 - 621) {
			double diff = data->menu_pos - (5800 - 621);
			diff *= 0.25;
			data->menu_pos -= diff;
			data->menu_speed *= 0.2;
		}
	}
}

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	// Draw everything to the screen here.
	al_draw_bitmap(data->bg, 0, 0, 0);
	al_draw_bitmap(data->logo, 54, 35, 0);

	al_draw_bitmap(data->framebg, 90, 412, 0);

	SetClippingRectangle(90, 412, 536, 621);

	ALLEGRO_TRANSFORM transform;
	al_identity_transform(&transform);
	al_translate_transform(&transform, 90, 412 - data->menu_pos);
	PushTransform(game, &transform);

	for (int i = 0; i < 99; i++) {
		al_draw_bitmap(((i / 3) % 2) ? data->leaf1 : data->leaf2, 50 + 150 * (i % 3), 25 + 175 * (i / 3), 0);
		al_draw_textf(data->font, al_map_rgb(0, 0, 0), 50 + 150 * (i % 3) + 150 / 2 + (((i / 3) % 2) ? 7 : 0), 25 + 175 * (i / 3) + 150 * 0.3 + (((i / 3) % 2) ? -10 : 0), ALLEGRO_ALIGN_CENTER, "%d", i + 1);
	}

	/*
	DrawVerticalGradientRect(0, 0, 536, 621 * 8, al_map_rgb(0, 0, 255), al_map_rgb(255, 0, 0));

	al_draw_text(game->_priv.font_console, al_map_rgb(255, 255, 255), 100, 500, ALLEGRO_ALIGN_LEFT, "yaba");
	al_draw_text(game->_priv.font_console, al_map_rgb(255, 255, 255), 100, 1000, ALLEGRO_ALIGN_LEFT, "daba");
	al_draw_text(game->_priv.font_console, al_map_rgb(255, 255, 255), 100, 1500, ALLEGRO_ALIGN_LEFT, "dooo");
  */

	PopTransform(game);
	ResetClippingRectangle();

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
		data->menu_offset = 0;
		data->menu_speed = 0;
		data->menu_pressed = true;
	}
	if ((ev->type == ALLEGRO_EVENT_TOUCH_END) || (ev->type == ALLEGRO_EVENT_TOUCH_CANCEL) || (ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_UP)) {
		if (data->menu_pressed && !data->menu_triggered) {
			PrintConsole(game, "click");
			StopCurrentGamestate(game);
			StartGamestate(game, "game");
		}

		data->menu_pressed = false;
		data->menu_triggered = false;
		if (fabs(data->menu_speed) < 0.0015) {
			data->menu_speed = 0;
		}
	}
	if ((ev->type == ALLEGRO_EVENT_TOUCH_MOVE) || (ev->type == ALLEGRO_EVENT_MOUSE_AXES)) {
		if (ev->type == ALLEGRO_EVENT_TOUCH_MOVE) {
			data->menu_offset += ev->touch.dy / (float)game->viewport.height;
		} else {
			data->menu_offset += ev->mouse.dy / (float)game->viewport.height;
		}
		if (data->menu_pressed && !data->menu_triggered && fabs(data->menu_offset) > 0.01) {
			double timestamp;
			if (ev->type == ALLEGRO_EVENT_TOUCH_BEGIN) {
				timestamp = ev->touch.timestamp;
			} else {
				timestamp = ev->mouse.timestamp;
			}
			data->menu_triggered = true;
			data->menu_lasty = game->data->mouseY;
			data->menu_lasttime = timestamp;
		}

		if (data->menu_triggered) {
			double timestamp;
			if (ev->type == ALLEGRO_EVENT_TOUCH_MOVE) {
				timestamp = ev->touch.timestamp;
			} else {
				timestamp = ev->mouse.timestamp;
			}

			if (timestamp - data->menu_lasttime) {
				data->menu_speed = (data->menu_lasty - game->data->mouseY) / ((timestamp - data->menu_lasttime) * 1000) * 16.666;
			}

			PrintConsole(game, "speed %f timestamp %f", data->menu_speed, timestamp);

			data->menu_pos += (data->menu_lasty - game->data->mouseY) * game->viewport.height;

			data->menu_lasty = game->data->mouseY;
			data->menu_lasttime = timestamp;
		}
	}
	if (ev->type == ALLEGRO_EVENT_MOUSE_AXES) {
		data->menu_pos -= ev->mouse.dz * 16;
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

	data->leaf1 = al_load_bitmap(GetDataFilePath(game, "leaf1.webp"));
	progress(game);

	data->leaf2 = al_load_bitmap(GetDataFilePath(game, "leaf2.webp"));
	progress(game);

	data->font = al_load_font(GetDataFilePath(game, "fonts/Brizel.ttf"), 88, 0);
	progress(game);

	data->ui = CreateCharacter(game, "ui");
	RegisterSpritesheet(game, data->ui, "ui");
	LoadSpritesheets(game, data->ui, progress);
	SelectSpritesheet(game, data->ui, "ui");

	data->beetle = CreateCharacter(game, "beetle");
	RegisterSpritesheet(game, data->beetle, "beetle");
	LoadSpritesheets(game, data->beetle, progress);
	SelectSpritesheet(game, data->beetle, "beetle");

	data->menu_pos = 0;
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

	data->menu_speed = 0;
	data->menu_pressed = false;
	data->menu_triggered = false;
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
