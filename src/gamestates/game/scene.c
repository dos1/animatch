/*! \file scene.c
 *  \brief Scene drawing routines.
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

#include "game.h"

void DrawScene(struct Game* game, struct GamestateResources* data) {
	al_hold_bitmap_drawing(true);
	al_draw_bitmap(data->bg, 0, 0, 0);

	for (int i = 0; i < data->leaves->spritesheet->frame_count; i++) {
		SetCharacterPosition(game, data->leaves, game->viewport.width / 2.0, game->viewport.height / 2.0, sin((game->time * (i / 20.0) + i * 32) / 2.0) * 0.003 + cos((game->time * (i / 14.0) + (i + 1) * 26) / 2.1) * 0.003);
		data->leaves->pos = i;
		data->leaves->frame = &data->leaves->spritesheet->frames[i];
		DrawCharacter(game, data->leaves);
	}

	SetCharacterPosition(game, data->acorn_top.character, 209 + 102 / 2.0, 240 + 105 / 2.0, GetTweenValue(&data->acorn_top.tween));
	DrawCharacter(game, data->acorn_top.character);

	SetCharacterPosition(game, data->acorn_bottom.character, 261 + 165 / 2.0, 1094 + 145 / 2.0 - (sin(GetTweenValue(&data->acorn_bottom.tween) * ALLEGRO_PI) * 16), sin(GetTweenPosition(&data->acorn_bottom.tween) * 2 * ALLEGRO_PI) / 12.0);
	DrawCharacter(game, data->acorn_bottom.character);

	al_hold_bitmap_drawing(false);
}

void UpdateBlur(struct Game* game, struct GamestateResources* data) {
	al_set_target_bitmap(data->scene);
	ClearToColor(game, al_map_rgb(0, 0, 0));
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

void DrawUIElement(struct Game* game, struct Character* ui, enum UI_ELEMENT element) {
	ui->pos = element;
	ui->frame = &ui->spritesheet->frames[ui->pos];
	DrawCharacter(game, ui);
}

bool IsOnUIElement(struct Game* game, struct Character* ui, enum UI_ELEMENT element, float x, float y) {
	ui->pos = element;
	ui->frame = &ui->spritesheet->frames[ui->pos];
	return IsOnCharacter(game, ui, x, y, true);
}
