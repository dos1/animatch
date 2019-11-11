/*! \file loading.c
 *  \brief Loading screen.
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

#define BLUR_DIVIDER 8

/*! \brief Resources used by Loading state. */
struct GamestateResources {
	ALLEGRO_BITMAP* pangolin;
	ALLEGRO_BITMAP *bg, *bg_lowres, *bg_blur;
	ALLEGRO_BITMAP *bar1, *bar2;
};

int Gamestate_ProgressCount = -1;

void Gamestate_ProcessEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev){};

void Gamestate_Logic(struct Game* game, struct GamestateResources* data, double delta){};

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	al_clear_to_color(al_map_rgb(255, 255, 255));
	al_draw_tinted_scaled_bitmap(data->bg_blur, al_map_rgba(64, 64, 64, 64), 0, 0,
		al_get_bitmap_width(data->bg_blur), al_get_bitmap_height(data->bg_blur),
		0, 0,
		game->viewport.width, game->viewport.height, 0);
	al_draw_bitmap(data->pangolin, (game->viewport.width - al_get_bitmap_width(data->pangolin)) / 2.0,
		(game->viewport.height - al_get_bitmap_height(data->pangolin)) / 2.0 - al_get_bitmap_height(data->bar1) / 2.0, 0);

	al_draw_bitmap(data->bar1, (game->viewport.width - al_get_bitmap_width(data->bar1)) / 2.0,
		(game->viewport.height + al_get_bitmap_height(data->pangolin)) / 2.0, 0);
	al_draw_bitmap_region(data->bar2, 0, 0,
		al_get_bitmap_width(data->bar2) * game->loading.progress, al_get_bitmap_height(data->bar2),
		(game->viewport.width - al_get_bitmap_width(data->bar1)) / 2.0,
		(game->viewport.height + al_get_bitmap_height(data->pangolin)) / 2.0, 0);
};

void* Gamestate_Load(struct Game* game, void (*progress)(struct Game*)) {
	struct GamestateResources* data = malloc(sizeof(struct GamestateResources));
	data->pangolin = al_load_bitmap(GetDataFilePath(game, "holypangolin.webp"));
	data->bg = al_load_bitmap(GetDataFilePath(game, "bg.webp"));

	data->bar1 = al_load_bitmap(GetDataFilePath(game, "bar1.webp"));
	data->bar2 = al_load_bitmap(GetDataFilePath(game, "bar2.webp"));

	data->bg_lowres = CreateNotPreservedBitmap(game->viewport.width / BLUR_DIVIDER, game->viewport.height / BLUR_DIVIDER);
	data->bg_blur = CreateNotPreservedBitmap(game->viewport.width / BLUR_DIVIDER, game->viewport.height / BLUR_DIVIDER);

	return data;
}

void Gamestate_PostLoad(struct Game* game, struct GamestateResources* data) {
	al_set_target_bitmap(data->bg_blur);

	if (game->data->config.solid_background) {
		al_clear_to_color(al_map_rgb(208, 215, 125));
		return;
	}

	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_draw_scaled_bitmap(data->bg, 0, 0, al_get_bitmap_width(data->bg), al_get_bitmap_height(data->bg),
		0, 0, al_get_bitmap_width(data->bg_blur), al_get_bitmap_height(data->bg_blur), 0);

	float size[2] = {al_get_bitmap_width(data->bg_blur), al_get_bitmap_height(data->bg_blur)};

	al_set_target_bitmap(data->bg_lowres);
	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_use_shader(game->data->kawese_shader);
	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_set_shader_float_vector("size", 2, size, 1);
	al_set_shader_float("kernel", 0);
	al_draw_bitmap(data->bg_blur, 0, 0, 0);
	al_use_shader(NULL);

	al_set_target_bitmap(data->bg_blur);
	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_use_shader(game->data->kawese_shader);
	al_set_shader_float_vector("size", 2, size, 1);
	al_set_shader_float("kernel", 0);
	al_draw_bitmap(data->bg_lowres, 0, 0, 0);
	al_use_shader(NULL);
}

void Gamestate_Unload(struct Game* game, struct GamestateResources* data) {
	al_destroy_bitmap(data->pangolin);
	al_destroy_bitmap(data->bg);
	al_destroy_bitmap(data->bg_lowres);
	al_destroy_bitmap(data->bg_blur);
	al_destroy_bitmap(data->bar1);
	al_destroy_bitmap(data->bar2);
	free(data);
}

void Gamestate_Start(struct Game* game, struct GamestateResources* data) {
	if (game->data->config.animated_transitions) {
		EnableCompositor(game, Compositor);
		game->data->transition.progress = 1.0;
		game->data->transition.gamestate = GetGamestate(game, NULL);
		game->data->transition.x = 0.5;
		game->data->transition.y = 0.5;
	}
}

void Gamestate_Stop(struct Game* game, struct GamestateResources* data) {}

void Gamestate_Reload(struct Game* game, struct GamestateResources* data) {
	data->bg_lowres = CreateNotPreservedBitmap(game->viewport.width / BLUR_DIVIDER, game->viewport.height / BLUR_DIVIDER);
	data->bg_blur = CreateNotPreservedBitmap(game->viewport.width / BLUR_DIVIDER, game->viewport.height / BLUR_DIVIDER);
}

void Gamestate_Pause(struct Game* game, struct GamestateResources* data) {}
void Gamestate_Resume(struct Game* game, struct GamestateResources* data) {}
