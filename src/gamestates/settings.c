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
	ALLEGRO_BITMAP *bg, *back_onbmp, *back_offbmp, *frame, *frame_bg;
	ALLEGRO_FONT* font;
	struct Character *back, *animals[6];
	bool back_hover, transitions, allow_continue, less_movement, solid_backgrounds, reset_progress;
};

int Gamestate_ProgressCount = 18; // number of loading steps as reported by Gamestate_Load; 0 when missing

static void UpdateAnimals(struct Game* game, struct GamestateResources* data) {
	SelectSpritesheet(game, data->animals[0], data->transitions ? "stand" : "blink");
	SelectSpritesheet(game, data->animals[1], data->allow_continue ? "stand" : "blink");
	SelectSpritesheet(game, data->animals[2], data->less_movement ? "stand" : "blink");
	SelectSpritesheet(game, data->animals[3], data->solid_backgrounds ? "stand" : "blink");
	SelectSpritesheet(game, data->animals[4], data->reset_progress ? "stand" : "blink");
	SelectSpritesheet(game, data->animals[5], "stand");

	ALLEGRO_COLOR color_on = al_map_rgb(255, 255, 255), color_off = al_map_rgba(100, 100, 100, 100);
	data->animals[0]->tint = data->transitions ? color_on : color_off;
	data->animals[1]->tint = data->allow_continue ? color_on : color_off;
	data->animals[2]->tint = data->less_movement ? color_on : color_off;
	data->animals[3]->tint = data->solid_backgrounds ? color_on : color_off;
	data->animals[4]->tint = data->reset_progress ? color_on : color_off;
}

void Gamestate_Logic(struct Game* game, struct GamestateResources* data, double delta) {
	// Here you should do all your game logic as if <delta> seconds have passed.
	if (data->back_hover) {
		data->back->tint = al_map_rgba_f(1.5, 1.5, 1.5, 1.0);
	} else {
		data->back->tint = al_map_rgba_f(1.0, 1.0, 1.0, 1.0);
	}
}

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	// Draw everything to the screen here.
	if (game->data->config.solid_background) {
		al_clear_to_color(al_map_rgb(228, 235, 155));
	} else {
		al_clear_to_color(al_map_rgb(255, 255, 255));
		al_draw_tinted_bitmap(data->bg, al_map_rgba(192, 192, 192, 192), 0, 0, 0);
	}
	DrawCharacter(game, data->back);
	ALLEGRO_COLOR color_on = al_map_rgb(0, 0, 0), color_off = al_map_rgb(130, 130, 130);
	al_draw_text(data->font, data->transitions ? color_on : color_off, game->viewport.width / 2.0, 400, ALLEGRO_ALIGN_CENTER, "animated transitions");
	al_draw_text(data->font, data->allow_continue ? color_on : color_off, game->viewport.width / 2.0, 550, ALLEGRO_ALIGN_CENTER, "continue after losing");
	al_draw_text(data->font, data->less_movement ? color_on : color_off, game->viewport.width / 2.0, 700, ALLEGRO_ALIGN_CENTER, "less movement");
	al_draw_text(data->font, data->solid_backgrounds ? color_on : color_off, game->viewport.width / 2.0, 850, ALLEGRO_ALIGN_CENTER, "solid backgrounds");
	al_draw_text(data->font, data->reset_progress ? color_on : color_off, game->viewport.width / 2.0, 1000, ALLEGRO_ALIGN_CENTER, "reset progress");

	for (int i = 0; i < 5; i++) {
		data->animals[i]->scaleX = 0.9;
		data->animals[i]->scaleY = 0.9;
		SetCharacterPosition(game, data->animals[i], 80, 440 + i * 150, 0);
		DrawCharacter(game, data->animals[i]);
		SetCharacterPosition(game, data->animals[i], 640, 440 + i * 150, 0);
		DrawCharacter(game, data->animals[i]);
	}

	ALLEGRO_TRANSFORM t;
	al_identity_transform(&t);
	al_translate_transform(&t, -game->viewport.width / 2.0, -200);
	al_scale_transform(&t, 0.8, 0.6);
	al_translate_transform(&t, game->viewport.width / 2.0, 200);
	PushTransform(game, &t);
	if (game->data->config.solid_background) {
		al_draw_filled_rectangle(114, -20 + 83,
			114 + al_get_bitmap_width(data->frame_bg), -20 + 83 + al_get_bitmap_height(data->frame_bg),
			al_map_rgb(185, 140, 89));
	} else {
		al_draw_bitmap(data->frame_bg, 114, -20 + 83, 0);
	}
	al_draw_bitmap(data->frame, 44, -20, 0);
	PopTransform(game);
	al_draw_text(data->font, al_map_rgb(255, 255, 255), game->viewport.width / 2.0, 160, ALLEGRO_ALIGN_CENTER, "SETTINGS");
}

void Gamestate_ProcessEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev) {
	// Called for each event in Allegro event queue.
	// Here you can handle user input, expiring timers etc.
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		StartTransition(game, 0.5, 0.5);
		ChangeCurrentGamestate(game, "menu");
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_AXES) || (ev->type == ALLEGRO_EVENT_TOUCH_MOVE) || (ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) || (ev->type == ALLEGRO_EVENT_TOUCH_BEGIN)) {
		if (!IsOnCharacter(game, data->back, game->data->mouseX * game->viewport.width, game->data->mouseY * game->viewport.height, true)) {
			data->back_hover = false;
		}
	}

	if ((ev->type == ALLEGRO_EVENT_TOUCH_BEGIN) || (ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)) {
		if (IsOnCharacter(game, data->back, game->data->mouseX * game->viewport.width, game->data->mouseY * game->viewport.height, true)) {
			data->back_hover = true;
		}

		if ((game->data->mouseY * game->viewport.height > 400 - 40) && (game->data->mouseY * game->viewport.height < 400 + 110)) {
			data->transitions = !data->transitions;
			game->data->config.animated_transitions = data->transitions;
			UpdateAnimals(game, data);
		}
		if ((game->data->mouseY * game->viewport.height > 550 - 40) && (game->data->mouseY * game->viewport.height < 550 + 110)) {
			data->allow_continue = !data->allow_continue;
			UpdateAnimals(game, data);
		}
		if ((game->data->mouseY * game->viewport.height > 700 - 40) && (game->data->mouseY * game->viewport.height < 700 + 110)) {
			data->less_movement = !data->less_movement;
			UpdateAnimals(game, data);
		}
		if ((game->data->mouseY * game->viewport.height > 850 - 40) && (game->data->mouseY * game->viewport.height < 850 + 110)) {
			data->solid_backgrounds = !data->solid_backgrounds;
			UpdateAnimals(game, data);
		}
		if ((game->data->mouseY * game->viewport.height > 1000 - 40) && (game->data->mouseY * game->viewport.height < 1000 + 110)) {
			data->reset_progress = !data->reset_progress;
			UpdateAnimals(game, data);
		}
	}

	if ((ev->type == ALLEGRO_EVENT_TOUCH_END) || (ev->type == ALLEGRO_EVENT_TOUCH_CANCEL) || (ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_UP)) {
		if (data->back_hover) {
			StartTransition(game, 0.5, 0.5);
			ChangeCurrentGamestate(game, "menu");
		}
		data->back_hover = false;
	}
}

void* Gamestate_Load(struct Game* game, void (*progress)(struct Game*)) {
	// Called once, when the gamestate library is being loaded.
	// Good place for allocating memory, loading bitmaps etc.
	//
	// Keep in mind that there's no OpenGL context available here. If you want to prerender something,
	// create VBOs, etc. do it in Gamestate_PostLoad.

	struct GamestateResources* data = calloc(1, sizeof(struct GamestateResources));

	data->bg = al_load_bitmap(GetDataFilePath(game, "bg.webp"));

	progress(game); // report that we progressed with the loading, so the engine can move a progress bar

	data->back_onbmp = al_load_bitmap(GetDataFilePath(game, "przycisk_do_tylu_on.webp"));
	data->back_offbmp = al_load_bitmap(GetDataFilePath(game, "przycisk_do_tylu_off.webp"));
	data->back = CreateCharacter(game, "back");
	RegisterSpritesheetFromBitmap(game, data->back, "back_on", data->back_onbmp);
	LoadSpritesheets(game, data->back, progress);
	SelectSpritesheet(game, data->back, "back_off");
	SetCharacterPosition(game, data->back, game->viewport.width - al_get_bitmap_width(data->back_onbmp) / 2.0 - 40, game->viewport.height - al_get_bitmap_height(data->back_onbmp) / 2.0 - 40, 0);

	progress(game);

	data->font = al_load_font(GetDataFilePath(game, "fonts/Caroni.ttf"), 64, 0);

	progress(game);

	data->animals[0] = CreateCharacter(game, "bee");
	data->animals[1] = CreateCharacter(game, "bird");
	data->animals[2] = CreateCharacter(game, "cat");
	data->animals[3] = CreateCharacter(game, "fish");
	data->animals[4] = CreateCharacter(game, "frog");
	data->animals[5] = CreateCharacter(game, "ladybug");

	for (int i = 0; i < 6; i++) {
		RegisterSpritesheet(game, data->animals[i], "stand");
		RegisterSpritesheet(game, data->animals[i], "blink");
		LoadSpritesheets(game, data->animals[i], progress);
	}

	data->frame = al_load_bitmap(GetDataFilePath(game, "frame_small.webp"));
	progress(game);

	data->frame_bg = al_load_bitmap(GetDataFilePath(game, "frame_small_bg.webp"));
	progress(game);
	return data;
}

void Gamestate_Unload(struct Game* game, struct GamestateResources* data) {
	// Called when the gamestate library is being unloaded.
	// Good place for freeing all allocated memory and resources.
	DestroyCharacter(game, data->back);
	al_destroy_bitmap(data->bg);
	al_destroy_bitmap(data->back_onbmp);
	al_destroy_bitmap(data->back_offbmp);
	al_destroy_bitmap(data->frame);
	al_destroy_bitmap(data->frame_bg);
	al_destroy_font(data->font);
	for (int i = 0; i < 6; i++) {
		DestroyCharacter(game, data->animals[i]);
	}
	free(data);
}

void Gamestate_Start(struct Game* game, struct GamestateResources* data) {
	// Called when this gamestate gets control. Good place for initializing state,
	// playing music etc.
	data->back_hover = false;

	data->less_movement = game->data->config.less_movement;
	data->solid_backgrounds = game->data->config.solid_background;
	data->allow_continue = game->data->config.allow_continuing;
	data->transitions = game->data->config.animated_transitions;
	data->reset_progress = false;

	UpdateAnimals(game, data);
}

void Gamestate_Stop(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets stopped. Stop timers, music etc. here.
	SetConfigOption(game, "Animatch", "less_movement", data->less_movement ? "1" : "0");
	SetConfigOption(game, "Animatch", "solid_background", data->solid_backgrounds ? "1" : "0");
	SetConfigOption(game, "Animatch", "allow_continuing", data->allow_continue ? "1" : "0");
	SetConfigOption(game, "Animatch", "animated_transitions", data->transitions ? "1" : "0");
	if (data->reset_progress) {
		SetConfigOption(game, "Animatch", "unlocked_levels", "1");
		game->data->unlocked_levels = 1;
	}
	if (data->solid_backgrounds != game->data->config.solid_background) {
		UnloadGamestate(game, "game");
		LoadGamestate(game, "game");
	}
	game->data->config.less_movement = data->less_movement;
	game->data->config.solid_background = data->solid_backgrounds;
	game->data->config.allow_continuing = data->allow_continue;
	game->data->config.animated_transitions = data->transitions;
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
