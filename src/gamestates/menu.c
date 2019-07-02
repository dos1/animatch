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
	ALLEGRO_BITMAP *bg, *logo, *frame, *framebg, *leaf, *leaf1, *leaf2, *leaf1b, *leaf2b, *infinitybmp, *back_onbmp, *back_offbmp;
	ALLEGRO_FONT* font;
	bool infinity_hover, back_hover;
	struct Character *beetle, *ui, *snail, *infinity, *back;

	int levels;
	int highlight;
	bool scrolling;

	int snail_offset[621];

	struct ScrollingViewport menu;
};

int Gamestate_ProgressCount = 16; // number of loading steps as reported by Gamestate_Load; 0 when missing

void Gamestate_Logic(struct Game* game, struct GamestateResources* data, double delta) {
	// Here you should do all your game logic as if <delta> seconds have passed.
	AnimateCharacter(game, data->beetle, delta, 1.0);
	AnimateCharacter(game, data->snail, data->scrolling ? delta : (delta * sqrt(fabs(data->menu.speed))), 1.0);

	if (data->infinity_hover) {
		data->infinity->tint = al_map_rgba_f(1.5, 1.5, 1.5, 1.0);
	} else {
		data->infinity->tint = al_map_rgba_f(1.0, 1.0, 1.0, 1.0);
	}

	if (game->data->in_progress) {
		data->back->spritesheet = data->back->spritesheets;
		if (data->back_hover) {
			data->back->tint = al_map_rgba_f(1.5, 1.5, 1.5, 1.0);
		} else {
			data->back->tint = al_map_rgba_f(1.0, 1.0, 1.0, 1.0);
		}
	} else {
		data->back->tint = al_map_rgba_f(0.4, 0.4, 0.4, 0.4);
		data->back->spritesheet = data->back->spritesheets->next;
	}
	data->back->frame = &data->back->spritesheet->frames[0];
}

void Gamestate_Tick(struct Game* game, struct GamestateResources* data) {
	UpdateScrollingViewport(game, &data->menu);
}

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	// Draw everything to the screen here.

	if (game->data->config.solid_background) {
		al_clear_to_color(al_map_rgb(208, 215, 125));
		al_draw_filled_rectangle(90, 412, 90 + 536, 412 + 621, al_map_rgb(185, 140, 89));
	} else {
		al_draw_bitmap(data->bg, 0, 0, 0);
		al_draw_bitmap(data->framebg, 90, 412, 0);
	}

	al_draw_bitmap(data->logo, 54, 35, 0);

	SetScrollingViewportAsTarget(game, &data->menu);

	for (int i = 1; i <= data->levels; i++) {
		int ii = i - 1;
		if (i > game->data->unlocked_levels) {
			al_draw_tinted_bitmap(((ii / 3) % 2) ? data->leaf1b : data->leaf2b, al_map_rgba_f(0.4, 0.4, 0.4, 0.4), 50 + 150 * (ii % 3), 25 + 175 * floor(ii / 3.0), 0);
			al_draw_textf(data->font, al_map_rgba_f(0.0, 0.0, 0.0, 0.4), 50 + 150 * (ii % 3) + 150 / 2.0 + (((ii / 3) % 2) ? 7 : 0), 25 + 175 * floor(ii / 3.0) + 150 * 0.3 + (((ii / 3) % 2) ? -10 : 0), ALLEGRO_ALIGN_CENTER, "%d", i);
		} else {
			ALLEGRO_COLOR color = al_map_rgb(255, 255, 255);
			if (data->highlight == i) {
				color = al_map_rgba_f(1.5, 1.5, 1.5, 1.0);
			}
			ALLEGRO_BITMAP* bitmap = ((ii / 3) % 2) ? data->leaf1 : data->leaf2;
			al_draw_tinted_rotated_bitmap(bitmap, color,
				al_get_bitmap_width(bitmap) / 2.0, al_get_bitmap_height(bitmap) / 2.0,
				50 + 150 * (ii % 3) + al_get_bitmap_width(bitmap) / 2.0, 25 + 175 * floor(ii / 3.0) + al_get_bitmap_height(bitmap) / 2.0,
				game->data->last_unlocked_level == i ? (sin(game->time * 4.0) / 16.0) : 0, 0);
			al_draw_textf(data->font, al_map_rgb(0, 0, 0), 50 + 150 * (ii % 3) + 150 / 2.0 + (((ii / 3) % 2) ? 7 : 0), 25 + 175 * floor(ii / 3.0) + 150 * 0.3 + (((ii / 3) % 2) ? -10 : 0), ALLEGRO_ALIGN_CENTER, "%d", i);
		}
	}

	SetScrollingViewportAsTarget(game, NULL);

	al_draw_bitmap(data->frame, 30, 315, 0);

	float pos = Clamp(0.12, 0.88, 0.12 + 0.76 * data->menu.pos / (float)(data->menu.content - data->menu.h));
	int off = ((int)(620 * pos) / 8) * 8;
	SetCharacterPosition(game, data->snail, data->snail_offset[off] - 14 + data->menu.x + data->menu.w, data->menu.y + data->menu.h * pos, ALLEGRO_PI / 2.0);
	DrawCharacter(game, data->snail);

	DrawCharacter(game, data->infinity);
	DrawCharacter(game, data->back);

	al_draw_bitmap(data->leaf, -32, 1083, 0);
	DrawCharacter(game, data->beetle);
	DrawUIElement(game, data->ui, game->config.mute ? UI_ELEMENT_NOSOUND : (game->config.music ? UI_ELEMENT_NOTE : UI_ELEMENT_FX));
	DrawUIElement(game, data->ui, UI_ELEMENT_SETTINGS);
	DrawUIElement(game, data->ui, UI_ELEMENT_ABOUT);
}

static int WhichLevel(struct Game* game, struct GamestateResources* data) {
	int x = (int)(game->data->mouseX * game->viewport.width - data->menu.x);
	int y = (int)(game->data->mouseY * game->viewport.height - data->menu.y);

	if (data->menu.triggered || !data->menu.pressed) {
		return -1;
	}

	if ((x < 0) || (y < 0) || (x > data->menu.w) || (y > data->menu.h)) {
		return -1;
	}

	y += (int)data->menu.pos;

	if (y > data->menu.content) {
		return -1;
	}

	x -= 50;
	y -= 25;

	if ((x < 0) || (y < 0)) {
		return -1;
	}

	x /= 150;
	y /= 175;

	if (x > 2) {
		return -1;
	}

	return y * 3 + x + 1;
}

void Gamestate_ProcessEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev) {
	// Called for each event in Allegro event queue.
	// Here you can handle user input, expiring timers etc.
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		UnloadAllGamestates(game); // mark this gamestate to be stopped and unloaded
		// When there are no active gamestates, the engine will quit.
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_AXES) || (ev->type == ALLEGRO_EVENT_TOUCH_MOVE) || (ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) || (ev->type == ALLEGRO_EVENT_TOUCH_BEGIN)) {
		if (!IsOnCharacter(game, data->infinity, game->data->mouseX * game->viewport.width, game->data->mouseY * game->viewport.height, true)) {
			data->infinity_hover = false;
		}
		if (!IsOnCharacter(game, data->back, game->data->mouseX * game->viewport.width, game->data->mouseY * game->viewport.height, true)) {
			data->back_hover = false;
		}
	}

	if ((ev->type == ALLEGRO_EVENT_TOUCH_BEGIN) || (ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)) {
		if (IsOnCharacter(game, data->snail, game->data->mouseX * game->viewport.width, game->data->mouseY * game->viewport.height, true)) {
			data->scrolling = true;
		}
		if (IsOnUIElement(game, data->ui, UI_ELEMENT_NOTE, game->data->mouseX * game->viewport.width, game->data->mouseY * game->viewport.height)) {
			ToggleAudio(game);
			return;
		}
		if (IsOnUIElement(game, data->ui, UI_ELEMENT_SETTINGS, game->data->mouseX * game->viewport.width, game->data->mouseY * game->viewport.height)) {
			ChangeCurrentGamestate(game, "settings");
			return;
		}
		if (IsOnCharacter(game, data->infinity, game->data->mouseX * game->viewport.width, game->data->mouseY * game->viewport.height, true)) {
			data->infinity_hover = true;
		}
		if (IsOnCharacter(game, data->back, game->data->mouseX * game->viewport.width, game->data->mouseY * game->viewport.height, true)) {
			data->back_hover = true;
		}
	}

	if (data->scrolling) {
		float pos = ((game->data->mouseY * game->viewport.height) - data->menu.y - 80) / (float)(data->menu.h - 140);
		pos = Clamp(0.0, 1.0, pos);
		data->menu.pos = pos * (data->menu.content - data->menu.h);
		data->menu.speed = 0.0;
	}

	if ((ev->type == ALLEGRO_EVENT_TOUCH_END) || (ev->type == ALLEGRO_EVENT_TOUCH_CANCEL) || (ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_UP)) {
		data->scrolling = false;
		if (data->menu.pressed && !data->menu.triggered) {
			PrintConsole(game, "click");
			game->data->level = WhichLevel(game, data);
			if (game->data->level >= 0 && game->data->level <= game->data->unlocked_levels && game->data->level <= data->levels) {
				StartTransition(game, (data->menu.x + 50 + (game->data->level - 1) % 3 * 150 + 150 / 2.0) / (float)game->viewport.width, (data->menu.y + 25 + floor((game->data->level - 1) / 3.0) * 175 + 175 / 2.0 - data->menu.pos - 10) / (float)game->viewport.height);
				ChangeCurrentGamestate(game, "game");
			}
		}
		if (data->infinity_hover) {
			game->data->level = 0;
			StartTransition(game, 0.5, 0.5);
			ChangeCurrentGamestate(game, "game");
		}
		if (game->data->in_progress && data->back_hover) {
			game->data->level = -1;
			StartTransition(game, 0.5, 0.5);
			ChangeCurrentGamestate(game, "game");
		}
		data->back_hover = false;
		data->infinity_hover = false;
	}

	ProcessScrollingViewportEvent(game, ev, &data->menu);
	data->highlight = WhichLevel(game, data);
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

	for (int i = 0; i < 621; i++) {
		int offset = -1;
		ALLEGRO_COLOR color;
		do {
			offset++;
			color = al_get_pixel(data->frame, 614 + offset - 30, 412 + i - 315);
		} while (color.a < 0.9);
		data->snail_offset[i] = offset;
	}

	progress(game);

	data->framebg = al_load_bitmap(GetDataFilePath(game, "frame_bg.webp"));
	progress(game);

	data->leaf = al_load_bitmap(GetDataFilePath(game, "leaf.webp"));
	progress(game);

	data->leaf1 = al_load_bitmap(GetDataFilePath(game, "leaf1.webp"));
	progress(game);

	data->leaf2 = al_load_bitmap(GetDataFilePath(game, "leaf2.webp"));
	progress(game);

	data->leaf1b = al_load_bitmap(GetDataFilePath(game, "listek1_bw.webp"));
	progress(game);

	data->leaf2b = al_load_bitmap(GetDataFilePath(game, "listek2_bw2.webp"));
	progress(game);

	data->infinitybmp = al_load_bitmap(GetDataFilePath(game, "przycisk_nieskonczonosc_on.webp"));
	data->infinity = CreateCharacter(game, "infinity");
	RegisterSpritesheetFromBitmap(game, data->infinity, "infinity", data->infinitybmp);
	LoadSpritesheets(game, data->infinity, progress);
	SelectSpritesheet(game, data->infinity, "infinity");
	SetCharacterPosition(game, data->infinity, game->viewport.width - al_get_bitmap_width(data->infinitybmp) / 2.0 - 20, game->viewport.height - al_get_bitmap_height(data->infinitybmp) / 2.0 - 40, 0);

	data->back_onbmp = al_load_bitmap(GetDataFilePath(game, "przycisk_do_tylu_on.webp"));
	data->back_offbmp = al_load_bitmap(GetDataFilePath(game, "przycisk_do_tylu_off.webp"));
	data->back = CreateCharacter(game, "back");
	RegisterSpritesheetFromBitmap(game, data->back, "back_off", data->back_offbmp);
	RegisterSpritesheetFromBitmap(game, data->back, "back_on", data->back_onbmp);
	LoadSpritesheets(game, data->back, progress);
	SelectSpritesheet(game, data->back, "back_off");
	SetCharacterPosition(game, data->back, game->viewport.width - al_get_bitmap_width(data->infinitybmp) - al_get_bitmap_width(data->back_onbmp) / 2.0 - 40, game->viewport.height - al_get_bitmap_height(data->back_onbmp) / 2.0 - 40, 0);

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

	data->snail = CreateCharacter(game, "snail");
	RegisterSpritesheet(game, data->snail, "scroll");
	LoadSpritesheets(game, data->snail, progress);
	SelectSpritesheet(game, data->snail, "scroll");
	data->snail->scaleX = 0.5;
	data->snail->scaleY = 0.5;

	data->menu.pos = 0;
	return data;
}

void Gamestate_Unload(struct Game* game, struct GamestateResources* data) {
	// Called when the gamestate library is being unloaded.
	// Good place for freeing all allocated memory and resources.
	al_destroy_bitmap(data->bg);
	al_destroy_bitmap(data->logo);
	al_destroy_bitmap(data->frame);
	al_destroy_bitmap(data->framebg);
	al_destroy_bitmap(data->leaf);
	al_destroy_bitmap(data->leaf1);
	al_destroy_bitmap(data->leaf2);
	al_destroy_bitmap(data->leaf1b);
	al_destroy_bitmap(data->leaf2b);
	al_destroy_font(data->font);
	DestroyCharacter(game, data->ui);
	DestroyCharacter(game, data->beetle);
	DestroyCharacter(game, data->snail);
	DestroyCharacter(game, data->infinity);
	DestroyCharacter(game, data->back);
	al_destroy_bitmap(data->infinitybmp);
	al_destroy_bitmap(data->back_onbmp);
	al_destroy_bitmap(data->back_offbmp);
	free(data);
}

void Gamestate_Start(struct Game* game, struct GamestateResources* data) {
	// Called when this gamestate gets control. Good place for initializing state,
	// playing music etc.
	int i = data->levels;
	do {
		data->levels = i;
		i++;
	} while (LevelExists(game, i));

	SetCharacterPosition(game, data->beetle, 0, 1194, 0);
	SetCharacterPosition(game, data->ui, 0, 0, 0);
	SetScrollingViewportPosition(game, &data->menu, 90, 412, 536, 621, ceil(data->levels / 3.0) * 175 + 25);

	data->highlight = -1;

	data->menu.speed = 0;
	data->menu.pressed = false;
	data->menu.triggered = false;

	data->menu.pos = Clamp(0, data->menu.content - data->menu.h, 175 * floor((game->data->unlocked_levels - 1) / 3.0 - 1));
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
