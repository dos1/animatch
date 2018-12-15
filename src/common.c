/*! \file common.c
 *  \brief Common stuff that can be used by all gamestates.
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

#include "common.h"
#include <libsuperderpy.h>
#include <stdio.h>

void Compositor(struct Game* game, struct Gamestate* gamestates, ALLEGRO_BITMAP* loading_fb) {
	struct Gamestate* tmp = gamestates;
	if (game->data->loading_fade) {
		al_set_target_bitmap(loading_fb);

		al_set_blender(ALLEGRO_ADD, ALLEGRO_ZERO, ALLEGRO_INVERSE_ALPHA);

		double scale = Interpolate((1.0 - game->data->loading_fade) * 0.7 + 0.3, TWEEN_STYLE_QUINTIC_IN) * 6.0;
		al_draw_scaled_rotated_bitmap(game->data->silhouette,
			al_get_bitmap_width(game->data->silhouette) / 2.0,
			al_get_bitmap_height(game->data->silhouette) / 2.0,
			game->viewport.width / 2.0, game->viewport.height / 2.0,
			scale, scale, 0.0, 0);

		al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);

		al_set_target_backbuffer(game->display);
	}
	ClearToColor(game, al_map_rgb(0, 0, 0));
	while (tmp) {
		if ((tmp->loaded) && (tmp->started)) {
			al_draw_bitmap(tmp->fb, game->clip_rect.x, game->clip_rect.y, 0);
		}
		tmp = tmp->next;
	}
	if (game->data->loading_fade) {
		al_draw_bitmap(loading_fb, game->clip_rect.x, game->clip_rect.y, 0);
	}
}

void PostLogic(struct Game* game, double delta) {
	if (!game->loading.shown && game->data->loading_fade) {
		game->data->loading_fade -= 0.02 * delta / (1 / 60.0);
		if (game->data->loading_fade <= 0.0) {
			DisableCompositor(game);
			game->data->loading_fade = 0.0;
		}
	}
}

void DrawBuildInfo(struct Game* game) {
	if (game->config.debug.enabled || game->_priv.showconsole) {
		int x, y, w, h;
		al_get_clipping_rectangle(&x, &y, &w, &h);
		al_hold_bitmap_drawing(true);
		DrawTextWithShadow(game->_priv.font_console, al_map_rgb(255, 255, 255), w - 10, h * 0.935, ALLEGRO_ALIGN_RIGHT, "Animatch PREALPHA");
		char revs[255];
		snprintf(revs, 255, "%s-%s", LIBSUPERDERPY_GAME_GIT_REV, LIBSUPERDERPY_GIT_REV);
		DrawTextWithShadow(game->_priv.font_console, al_map_rgb(255, 255, 255), w - 10, h * 0.965, ALLEGRO_ALIGN_RIGHT, revs);
		al_hold_bitmap_drawing(false);
	}
}

bool GlobalEventHandler(struct Game* game, ALLEGRO_EVENT* ev) {
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_F)) {
		game->config.fullscreen = !game->config.fullscreen;
		if (game->config.fullscreen) {
			SetConfigOption(game, "SuperDerpy", "fullscreen", "1");
		} else {
			SetConfigOption(game, "SuperDerpy", "fullscreen", "0");
		}
#ifdef ALLEGRO_ANDROID
		al_set_display_flag(game->display, ALLEGRO_FRAMELESS, game->config.fullscreen);
#endif
		al_set_display_flag(game->display, ALLEGRO_FULLSCREEN_WINDOW, game->config.fullscreen);
		SetupViewport(game);
		PrintConsole(game, "Fullscreen toggled");
	}
	if (ev->type == ALLEGRO_EVENT_MOUSE_AXES) {
		game->data->mouseX = Clamp(0, 1, (ev->mouse.x - game->clip_rect.x) / (double)game->clip_rect.w);
		game->data->mouseY = Clamp(0, 1, (ev->mouse.y - game->clip_rect.y) / (double)game->clip_rect.h);
	}
	if ((ev->type == ALLEGRO_EVENT_TOUCH_BEGIN) || (ev->type == ALLEGRO_EVENT_TOUCH_MOVE)) {
		game->data->mouseX = Clamp(0, 1, (ev->touch.x - game->clip_rect.x) / (double)game->clip_rect.w);
		game->data->mouseY = Clamp(0, 1, (ev->touch.y - game->clip_rect.y) / (double)game->clip_rect.h);
	}

	if (ev->type == ALLEGRO_EVENT_TOUCH_BEGIN) {
		game->data->touch = true;
	}
	if (ev->type == ALLEGRO_EVENT_MOUSE_AXES) {
		game->data->touch = false;
	}

#ifdef ALLEGRO_ANDROID
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_BACK)) {
		QuitGame(game, true);
	}
#endif

	return false;
}

struct CommonResources* CreateGameData(struct Game* game) {
	struct CommonResources* data = calloc(1, sizeof(struct CommonResources));
	data->kawese_shader = CreateShader(game, GetDataFilePath(game, "shaders/vertex.glsl"), GetDataFilePath(game, "shaders/kawese.glsl"));
	char* names[] = {"silhouette/frog.webp", "silhouette/bee.webp", "silhouette/ladybug.webp", "silhouette/cat.webp", "silhouette/fish.webp"};
	data->silhouette = al_load_bitmap(GetDataFilePath(game, names[rand() % (sizeof(names) / sizeof(names[0]))]));
	return data;
}

void DestroyGameData(struct Game* game) {
	DestroyShader(game, game->data->kawese_shader);
	al_destroy_bitmap(game->data->silhouette);
	free(game->data);
}
