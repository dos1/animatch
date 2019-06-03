/*! \file game.c
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

#include "game/game.h"

int Gamestate_ProgressCount = 80; // number of loading steps as reported by Gamestate_Load

void Gamestate_Logic(struct Game* game, struct GamestateResources* data, double delta) {
	// Called 60 times per second (by default). Here you should do all your game logic.
	data->counter += delta * sqrt(1.0 + data->counter_speed * data->counter_strength);
	data->counter_speed -= delta;
	data->counter_strength -= delta * 8;
	if (data->counter_speed <= 0.0 || data->counter_strength <= 0.0) {
		data->counter_speed = 0.0;
		data->counter_strength = 0.0;
	}

	TM_Process(data->timeline, delta);
	UpdateParticles(game, data->particles, delta);
	UpdateTween(&data->acorn_top.tween, delta);
	UpdateTween(&data->acorn_bottom.tween, delta);
	UpdateTween(&data->scoring, delta);
	data->snail_blink -= delta;

	if (!game->data->config.less_movement) {
		AnimateCharacter(game, data->beetle, delta, 1.0);
		if (data->snail_blink < 0) {
			data->snail_blink = 6.0 + rand() / (float)RAND_MAX * 16.0;
			data->snail->pos = rand() % data->snail->spritesheet->frame_count;
			data->snail->frame = &data->snail->spritesheet->frames[data->snail->pos];
		}
	} else {
		data->scoring.pos = 1.0;
	}

	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			if (IsDrawable(data->fields[i][j].type)) {
				AnimateCharacter(game, data->fields[i][j].drawable, delta, 1.0);
			}
			if (data->fields[i][j].overlay_visible) {
				if (!game->data->config.less_movement) {
					AnimateCharacter(game, data->fields[i][j].overlay, delta, 1.0);
				}
			}
			UpdateTween(&data->fields[i][j].animation.falling, delta);
			UpdateTween(&data->fields[i][j].animation.hiding, delta);
			UpdateTween(&data->fields[i][j].animation.swapping, delta);
			UpdateTween(&data->fields[i][j].animation.shaking, delta);
			UpdateTween(&data->fields[i][j].animation.hinting, delta);
			UpdateTween(&data->fields[i][j].animation.launching, delta);
			UpdateTween(&data->fields[i][j].animation.collecting, delta);

			if (data->fields[i][j].to_highlight) {
				data->fields[i][j].highlight += delta * 8.0;
			} else {
				data->fields[i][j].highlight -= delta * 2.0;
			}
			data->fields[i][j].highlight = Clamp(0.0, 1.0, data->fields[i][j].highlight);

			if (IsSleeping(&data->fields[i][j])) {
				continue;
			}
			if (!IsDrawable(data->fields[i][j].type)) {
				continue;
			}

			if (!game->data->config.less_movement) {
				if (data->fields[i][j].animation.blink_time) {
					data->fields[i][j].animation.blink_time -= (int)(delta * 1000);
					if (data->fields[i][j].animation.blink_time <= 0) {
						data->fields[i][j].animation.blink_time = 0;
						if (data->fields[i][j].type == FIELD_TYPE_ANIMAL) {
							SelectSpritesheet(game, data->fields[i][j].drawable, "stand");
						}
					}
				} else if (data->fields[i][j].animation.action_time) {
					data->fields[i][j].animation.action_time -= (int)(delta * 1000);
					if (data->fields[i][j].animation.action_time <= 0) {
						data->fields[i][j].animation.action_time = 0;
						if (data->fields[i][j].type == FIELD_TYPE_ANIMAL) {
							SelectSpritesheet(game, data->fields[i][j].drawable, "stand");
						}
					}
				} else {
					data->fields[i][j].animation.time_to_action -= (int)(delta * 1000);
					data->fields[i][j].animation.time_to_blink -= (int)(delta * 1000);

					if (data->fields[i][j].animation.time_to_action <= 0) {
						data->fields[i][j].animation.time_to_action = rand() % 250000 + 500000;
						data->fields[i][j].animation.action_time = rand() % 2000 + 1000;
						if (data->fields[i][j].type == FIELD_TYPE_ANIMAL) {
							SelectSpritesheet(game, data->fields[i][j].drawable, ANIMAL_ACTIONS[data->fields[i][j].data.animal.type].names[rand() % ANIMAL_ACTIONS[data->fields[i][j].data.animal.type].actions]);
						}
					}

					if (data->fields[i][j].animation.time_to_blink <= 0) {
						if (strcmp(data->fields[i][j].drawable->spritesheet->name, "stand") == 0) {
							if (data->fields[i][j].type == FIELD_TYPE_ANIMAL) {
								SelectSpritesheet(game, data->fields[i][j].drawable, "blink");
							}
							data->fields[i][j].animation.time_to_blink = rand() % 100000 + 200000;
							data->fields[i][j].animation.blink_time = rand() % 400 + 100;
						}
					}
				}
			}
		}
	}

	if (data->done) {
		data->locked = true;
		UpdateTween(&data->finishing, delta);
	}

	if (data->failed) {
		data->locked = true;
		UpdateTween(&data->failing, delta);
	}

	if (data->restart_hover) {
		data->restart_btn->tint = al_map_rgb_f(1.5, 1.5, 1.5);
	} else {
		data->restart_btn->tint = al_map_rgb_f(1.0, 1.0, 1.0);
	}

	DrawDebugInterface(game, data);
}

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	// Called as soon as possible, but no sooner than next Gamestate_Logic call.
	// Draw everything to the screen here.
	int offsetY = (int)((game->viewport.height - (ROWS * 90)) / 2.0);

	al_set_target_bitmap(data->board);
	ClearToColor(game, al_map_rgba(0, 0, 0, 0));
	al_set_clipping_rectangle(0, offsetY, game->viewport.width, game->viewport.height - offsetY * 2);
	al_hold_bitmap_drawing(true);
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			bool hovered = IsSameID(data->hovered, (struct FieldID){i, j});
			ALLEGRO_COLOR color = al_map_rgba(222, 222, 222, 222);
			if (data->locked || game->data->touch || !hovered) {
				color = al_map_rgba(180, 180, 180, 180);
			}
			color = InterpolateColor(color, al_map_rgba(240, 240, 240, 240), data->fields[i][j].highlight);
			if (data->fields[i][j].type != FIELD_TYPE_DISABLED) {
				ALLEGRO_BITMAP* bmp = data->field_bgs[(j * ROWS + i + j % 2) % 4];
				al_draw_tinted_scaled_bitmap(bmp, color, 0, 0, al_get_bitmap_width(bmp), al_get_bitmap_height(bmp),
					i * 90 + 1, j * 90 + offsetY + 1, 90 - 2, 90 - 2, 0);
			}
		}
	}
	al_hold_bitmap_drawing(false);

	al_use_shader(data->desaturate_shader);
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			DrawField(game, data, data->fields[i][j].id);
		}
	}
	al_reset_clipping_rectangle();
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			DrawOverlay(game, data, data->fields[i][j].id);
		}
	}
	al_use_shader(NULL);

	SetFramebufferAsTarget(game);
	ClearToColor(game, al_map_rgb(0, 0, 0));
	DrawScene(game, data);

	float size[2] = {al_get_bitmap_width(data->lowres_scene_blur), al_get_bitmap_height(data->lowres_scene_blur)};

	al_use_shader(data->combine_shader);
	al_set_shader_sampler("tex_bg", data->lowres_scene_blur, 1);
	al_set_shader_float_vector("size", 2, size, 1);
	al_draw_bitmap(data->board, 0, 0, 0);
	al_use_shader(NULL);

	al_hold_bitmap_drawing(true);
	SetCharacterPosition(game, data->ui, 0, 0, 0);
	DrawUIElement(game, data->ui, data->level.infinite ? UI_ELEMENT_BALOON_MEDIUM : UI_ELEMENT_BALOON_BIG);
	DrawUIElement(game, data->ui, UI_ELEMENT_BALOON_MINI);
	DrawUIElement(game, data->ui, UI_ELEMENT_SCORE);
	al_hold_bitmap_drawing(false);

	al_draw_text(data->font, al_map_rgb(64, 72, 5), 622, 53, ALLEGRO_ALIGN_CENTER, "MOVES");
	al_draw_textf(data->moves >= 100 ? data->font_num_medium : data->font_num_big, al_map_rgb(49, 84, 2), 620, data->moves >= 100 ? 96 : 82, ALLEGRO_ALIGN_CENTER, "%d", data->moves);
	al_draw_text(data->font, al_map_rgb(55, 28, 20), 118, 160, ALLEGRO_ALIGN_CENTER, "LEVEL");
	if (data->level.infinite) {
		al_draw_text(data->font_num_medium, al_map_rgb(255, 255, 194), 118, 200, ALLEGRO_ALIGN_CENTER, "∞");
	} else {
		al_draw_textf(data->font_num_medium, al_map_rgb(255, 255, 194), 118, 200, ALLEGRO_ALIGN_CENTER, "%d", data->level.id);
	}

	SetCharacterPosition(game, data->snail, 476, 178, 0);
	DrawCharacter(game, data->snail);

	al_draw_bitmap(data->leaf, -32, 1083, 0);
	SetCharacterPosition(game, data->beetle, 0, 1194, 0);
	DrawCharacter(game, data->beetle);

	if (data->level.infinite) {
		al_draw_text(data->font, al_map_rgb(64, 72, 5), 322 + 70, 48, ALLEGRO_ALIGN_CENTER, "SCORE");

		ALLEGRO_TRANSFORM transform, orig = *al_get_current_transform();
		al_identity_transform(&transform);
		al_scale_transform(&transform, 1.0 + GetTweenValue(&data->scoring) / 4.0, 1.0 + GetTweenValue(&data->scoring) / 4.0);
		al_translate_transform(&transform, 322 + 70, 35 + 105 / 2.0 + 30);
		al_compose_transform(&transform, &orig);
		al_use_transform(&transform);
		al_draw_textf(data->font_num_big, al_map_rgb(49, 84, 2), 0, -30, ALLEGRO_ALIGN_CENTER, "%d", data->score);
		al_use_transform(&orig);
	} else {
		al_draw_bitmap(data->placeholder, 240, 45, 0);
	}

	DrawParticles(game, data->particles);

	al_use_shader(data->desaturate_shader);
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			if (IsDrawable(data->fields[i][j].type) && GetTweenPosition(&data->fields[i][j].animation.launching) < 1.0) {
				al_set_shader_bool("enabled", IsSleeping(&data->fields[i][j]));
				DrawCharacter(game, data->fields[i][j].drawable);
				if (data->fields[i][j].overlay_visible) {
					DrawCharacter(game, data->fields[i][j].overlay);
				}
			}
		}
	}
	al_use_shader(NULL);

	if (data->menu) {
		al_draw_filled_rectangle(0, 0, game->viewport.width, game->viewport.height, al_map_rgba(0, 0, 0, 96));
		al_draw_bitmap_region(data->bg, 0, 1400, 80, 40, 0, 1400, 0);

		al_draw_bitmap(data->leaf, -32, 1083, 0);

		DrawUIElement(game, data->ui, game->config.mute ? UI_ELEMENT_NOSOUND : (game->config.music ? UI_ELEMENT_NOTE : UI_ELEMENT_FX));
		DrawUIElement(game, data->ui, UI_ELEMENT_HINT);
		DrawUIElement(game, data->ui, UI_ELEMENT_HOME);

		DrawCharacter(game, data->beetle);
	}

	if (data->done) {
		al_draw_filled_rectangle(0, 0, game->viewport.width, game->viewport.height, al_map_rgba(0, 0, 0, 160 * GetTweenPosition(&data->finishing)));
		al_draw_bitmap(data->frame_bg, 114, -410 + (508 + 410) * GetTweenValue(&data->finishing) + 83, 0);
		al_draw_bitmap(data->frame, 44, -410 + (508 + 410) * GetTweenValue(&data->finishing), 0);

		al_draw_textf(data->font_num_big, al_map_rgb(255, 255, 194), 720 / 2.0, -410 + (508 + 410) * GetTweenValue(&data->finishing) + 120, ALLEGRO_ALIGN_CENTER, "LEVEL");
		al_draw_textf(data->font_num_big, al_map_rgb(255, 255, 194), 720 / 2.0, -410 + (508 + 410) * GetTweenValue(&data->finishing) + 204, ALLEGRO_ALIGN_CENTER, "COMPLETE!");
	}

	if (GetTweenValue(&data->failing)) {
		al_draw_filled_rectangle(0, 0, game->viewport.width, game->viewport.height, al_map_rgba(0, 0, 0, 160 * GetTweenPosition(&data->failing)));
		al_draw_bitmap(data->frame_bg, 114, -410 + (508 + 410) * GetTweenValue(&data->failing) + 83, 0);
		al_draw_bitmap(data->frame, 44, -410 + (508 + 410) * GetTweenValue(&data->failing), 0);

		al_draw_textf(data->font_num_big, al_map_rgb(255, 255, 194), 720 / 2.0, -410 + (508 + 410) * GetTweenValue(&data->failing) + 120, ALLEGRO_ALIGN_CENTER, "LEVEL");
		al_draw_textf(data->font_num_big, al_map_rgb(255, 255, 194), 720 / 2.0, -410 + (508 + 410) * GetTweenValue(&data->failing) + 204, ALLEGRO_ALIGN_CENTER, "FAILED!");

		//al_draw_textf(data->font_small, al_map_rgb(255, 255, 255), 130, -410 + (508 + 410) * GetTweenValue(&data->failing) + 353, ALLEGRO_ALIGN_LEFT, "CONTINUE >");

		SetCharacterPosition(game, data->restart_btn, 440 + 169 / 2.0, 175 / 2.0 + 780 - (508 + 410) * (1.0 - GetTweenValue(&data->failing)), 0);
		DrawCharacter(game, data->restart_btn);
	}

	if (data->paused) {
		DrawDebugInterface(game, data);
	}
}

void Gamestate_ProcessEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev) {
	// Called for each event in Allegro event queue.
	// Here you can handle user input, expiring timers etc.
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		StartTransition(game, 0.5, 0.5);
		StopCurrentGamestate(game);
		StartGamestate(game, "menu");
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_AXES) || (ev->type == ALLEGRO_EVENT_TOUCH_MOVE) || (ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) || (ev->type == ALLEGRO_EVENT_TOUCH_BEGIN)) {
		if (!IsOnCharacter(game, data->restart_btn, game->data->mouseX * game->viewport.width, game->data->mouseY * game->viewport.height, true)) {
			data->restart_hover = false;
		}
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) || (ev->type == ALLEGRO_EVENT_TOUCH_BEGIN)) {
		data->restart_hover = IsOnCharacter(game, data->restart_btn, game->data->mouseX * game->viewport.width, game->data->mouseY * game->viewport.height, true);
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) || (ev->type == ALLEGRO_EVENT_TOUCH_END)) {
		if (data->failed && data->restart_hover) {
			RestartLevel(game, data);
		}
		data->restart_hover = false;
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) || (ev->type == ALLEGRO_EVENT_TOUCH_BEGIN)) {
		if (data->done) {
			if (GetTweenPosition(&data->finishing) == 1.0) {
				StartTransition(game, game->data->mouseX, game->data->mouseY);
				StopCurrentGamestate(game);
				StartGamestate(game, "menu");
				UnlockLevel(game, data->level.id + 1);
			}
			return;
		}

		if (data->failed) {
			return;
		}

		if (IsOnCharacter(game, data->beetle, game->data->mouseX * game->viewport.width, game->data->mouseY * game->viewport.height, true)) {
			data->menu = !data->menu;
			return;
		}
		if (data->menu) {
			if (IsOnUIElement(game, data->ui, UI_ELEMENT_NOTE, game->data->mouseX * game->viewport.width, game->data->mouseY * game->viewport.height)) {
				ToggleAudio(game);
				return;
			}

			if (IsOnUIElement(game, data->ui, UI_ELEMENT_HINT, game->data->mouseX * game->viewport.width, game->data->mouseY * game->viewport.height)) {
				data->menu = false;
				ShowHint(game, data);
				return;
			}

			if (IsOnUIElement(game, data->ui, UI_ELEMENT_HOME, game->data->mouseX * game->viewport.width, game->data->mouseY * game->viewport.height)) {
				StartTransition(game, 0.5, 0.5);
				StopCurrentGamestate(game);
				StartGamestate(game, "menu");
				return;
			}

			data->menu = false;
			return;
		}
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_AXES) || (ev->type == ALLEGRO_EVENT_TOUCH_MOVE) || (ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) || (ev->type == ALLEGRO_EVENT_TOUCH_BEGIN)) {
		int offsetY = (int)((game->viewport.height - (ROWS * 90)) / 2.0);
		data->hovered.i = (int)(game->data->mouseX * game->viewport.width / 90);
		data->hovered.j = (int)((game->data->mouseY * game->viewport.height - offsetY) / 90);
		if ((data->hovered.i < 0) || (data->hovered.j < 0) || (data->hovered.i >= COLS) || (data->hovered.j >= ROWS) || (game->data->mouseY * game->viewport.height <= offsetY) || (game->data->mouseX == 0.0)) {
			data->hovered.i = -1;
			data->hovered.j = -1;
		}
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) || (ev->type == ALLEGRO_EVENT_TOUCH_BEGIN)) {
		if (IsOnCharacter(game, data->acorn_top.character, game->data->mouseX * game->viewport.width, game->data->mouseY * game->viewport.height, true)) {
			data->acorn_top.tween = Tween(game, fmod(GetTweenValue(&data->acorn_top.tween), 2 * ALLEGRO_PI), ALLEGRO_PI * 8, TWEEN_STYLE_QUADRATIC_OUT, 1.5);
		}
		if (IsOnCharacter(game, data->acorn_bottom.character, game->data->mouseX * game->viewport.width, game->data->mouseY * game->viewport.height, true)) {
			data->acorn_bottom.tween = Tween(game, 0.0, 1.0, TWEEN_STYLE_BOUNCE_OUT, 0.75);
		}
	}

	if (data->locked) {
		return;
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) || (ev->type == ALLEGRO_EVENT_TOUCH_BEGIN)) {
		data->current = data->hovered;
		data->clicked = true;
	}

	if (((ev->type == ALLEGRO_EVENT_MOUSE_AXES) || (ev->type == ALLEGRO_EVENT_TOUCH_MOVE)) && (data->clicked)) {
		Turn(game, data, data->current, data->hovered);
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) || (ev->type == ALLEGRO_EVENT_TOUCH_END)) {
		data->clicked = false;
	}

	if (game->config.debug.enabled) {
		HandleDebugEvent(game, data, ev);
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
	for (size_t i = 0; i < sizeof(ANIMALS) / sizeof(ANIMALS[0]); i++) {
		data->animal_archetypes[i] = CreateCharacter(game, StrToLower(game, ANIMALS[i]));
		RegisterSpritesheet(game, data->animal_archetypes[i], "stand");
		RegisterSpritesheet(game, data->animal_archetypes[i], "blink");
		for (int j = 0; j < ANIMAL_ACTIONS[i].actions; j++) {
			RegisterSpritesheet(game, data->animal_archetypes[i], ANIMAL_ACTIONS[i].names[j]);
		}
		LoadSpritesheets(game, data->animal_archetypes[i], progress);
	}

	data->leaves = CreateCharacter(game, "bg");
	RegisterSpritesheet(game, data->leaves, "bg");
	LoadSpritesheets(game, data->leaves, progress);
	SelectSpritesheet(game, data->leaves, "bg");

	data->acorn_top.character = CreateCharacter(game, "bg");
	RegisterSpritesheet(game, data->acorn_top.character, "top");
	LoadSpritesheets(game, data->acorn_top.character, progress);
	SelectSpritesheet(game, data->acorn_top.character, "top");
	data->acorn_top.tween = StaticTween(game, 0.0);

	data->acorn_bottom.character = CreateCharacter(game, "bg");
	RegisterSpritesheet(game, data->acorn_bottom.character, "bottom");
	LoadSpritesheets(game, data->acorn_bottom.character, progress);
	SelectSpritesheet(game, data->acorn_bottom.character, "bottom");
	data->acorn_bottom.tween = StaticTween(game, 0.0);

	data->ui = CreateCharacter(game, "ui");
	RegisterSpritesheet(game, data->ui, "ui");
	LoadSpritesheets(game, data->ui, progress);
	SelectSpritesheet(game, data->ui, "ui");

	data->beetle = CreateCharacter(game, "beetle");
	RegisterSpritesheet(game, data->beetle, "beetle");
	LoadSpritesheets(game, data->beetle, progress);
	SelectSpritesheet(game, data->beetle, "beetle");

	data->snail = CreateCharacter(game, "snail");
	RegisterSpritesheet(game, data->snail, "snail");
	LoadSpritesheets(game, data->snail, progress);
	SelectSpritesheet(game, data->snail, "snail");

	for (size_t i = 0; i < sizeof(SPECIALS) / sizeof(SPECIALS[0]); i++) {
		data->special_archetypes[i] = CreateCharacter(game, StrToLower(game, SPECIALS[i]));
		for (int j = 0; j < SPECIAL_ACTIONS[i].actions; j++) {
			RegisterSpritesheet(game, data->special_archetypes[i], SPECIAL_ACTIONS[i].names[j]);
		}
		LoadSpritesheets(game, data->special_archetypes[i], progress);
	}

	data->bg = al_load_bitmap(GetDataFilePath(game, "bg.webp"));
	progress(game);

	data->leaf = al_load_bitmap(GetDataFilePath(game, "leaf.webp"));
	progress(game);

	data->frame = al_load_bitmap(GetDataFilePath(game, "frame_small.webp"));
	progress(game);

	data->frame_bg = al_load_bitmap(GetDataFilePath(game, "frame_small_bg.webp"));
	progress(game);

	data->restart_btn = CreateCharacter(game, "restart_btn");
	data->restart = al_load_bitmap(GetDataFilePath(game, "przycisk_do_tylu_on.webp"));
	RegisterSpritesheetFromBitmap(game, data->restart_btn, "restart", data->restart);
	LoadSpritesheets(game, data->restart_btn, progress);

	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->fields[i][j].drawable = CreateCharacter(game, NULL);
			data->fields[i][j].drawable->shared = true;
			data->fields[i][j].overlay = CreateCharacter(game, NULL);
			data->fields[i][j].overlay->shared = true;
			SetParentCharacter(game, data->fields[i][j].overlay, data->fields[i][j].drawable);
			SetCharacterPosition(game, data->fields[i][j].overlay, 108 / 2.0, 108 / 2.0, 0); // FIXME: subcharacters should be positioned by parent pivot
			data->fields[i][j].id.i = i;
			data->fields[i][j].id.j = j;
			data->fields[i][j].matched = false;
			data->fields[i][j].match_mark = 0;
			data->fields[i][j].locked = true;
			data->fields[i][j].animation.super = (struct FieldID){-1, -1};
		}
	}
	progress(game);

	data->field_bgs[0] = al_load_bitmap(GetDataFilePath(game, "kwadrat1.webp"));
	progress(game);
	data->field_bgs[1] = al_load_bitmap(GetDataFilePath(game, "kwadrat2.webp"));
	progress(game);
	data->field_bgs[2] = al_load_bitmap(GetDataFilePath(game, "kwadrat3.webp"));
	progress(game);
	data->field_bgs[3] = al_load_bitmap(GetDataFilePath(game, "kwadrat4.webp"));
	progress(game);

	data->placeholder = al_load_bitmap(GetDataFilePath(game, "placeholder.webp"));

	data->scene = CreateNotPreservedBitmap(game->viewport.width, game->viewport.height);
	data->lowres_scene = CreateNotPreservedBitmap(game->viewport.width / BLUR_DIVIDER, game->viewport.height / BLUR_DIVIDER);
	data->lowres_scene_blur = al_create_bitmap(game->viewport.width / BLUR_DIVIDER, game->viewport.height / BLUR_DIVIDER);
	data->board = CreateNotPreservedBitmap(game->viewport.width, game->viewport.height);
	progress(game);

	data->combine_shader = CreateShader(game, GetDataFilePath(game, "shaders/vertex.glsl"), GetDataFilePath(game, "shaders/combine.glsl"));
	progress(game);
	data->desaturate_shader = CreateShader(game, GetDataFilePath(game, "shaders/vertex.glsl"), GetDataFilePath(game, "shaders/desaturate.glsl"));
	progress(game);

	data->font = al_load_font(GetDataFilePath(game, "fonts/Caroni.ttf"), 35, 0);
	progress(game);
	data->font_small = al_load_font(GetDataFilePath(game, "fonts/Caroni.ttf"), 24, 0);
	progress(game);
	data->font_num_small = al_load_font(GetDataFilePath(game, "fonts/Brizel.ttf"), 42, 0);
	progress(game);
	data->font_num_medium = al_load_font(GetDataFilePath(game, "fonts/Brizel.ttf"), 55, 0);
	progress(game);
	data->font_num_big = al_load_font(GetDataFilePath(game, "fonts/Brizel.ttf"), 88, 0);
	progress(game);

	data->timeline = TM_Init(game, data, "timeline");

	data->particles = CreateParticleBucket(game, 4096, true);

	return data;
}

void Gamestate_PostLoad(struct Game* game, struct GamestateResources* data) {
	data->field_bgs_bmp = al_create_bitmap(88 * 4, 88);
	al_set_target_bitmap(data->field_bgs_bmp);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	for (int i = 0; i < 4; i++) {
		al_draw_bitmap(data->field_bgs[i], i * 88, 0, 0);
		al_destroy_bitmap(data->field_bgs[i]);
		data->field_bgs[i] = al_create_sub_bitmap(data->field_bgs_bmp, i * 88, 0, 88, 88);
	}

	UpdateBlur(game, data);
}

void Gamestate_Unload(struct Game* game, struct GamestateResources* data) {
	// Called when the gamestate library is being unloaded.
	// Good place for freeing all allocated memory and resources.
	DestroyParticleBucket(game, data->particles);
	DestroyCharacter(game, data->leaves);
	DestroyCharacter(game, data->ui);
	DestroyCharacter(game, data->beetle);
	DestroyCharacter(game, data->snail);
	DestroyCharacter(game, data->acorn_top.character);
	DestroyCharacter(game, data->acorn_bottom.character);
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			DestroyCharacter(game, data->fields[i][j].drawable);
			DestroyCharacter(game, data->fields[i][j].overlay);
		}
	}
	for (size_t i = 0; i < sizeof(ANIMALS) / sizeof(ANIMALS[0]); i++) {
		DestroyCharacter(game, data->animal_archetypes[i]);
	}
	for (size_t i = 0; i < sizeof(SPECIALS) / sizeof(SPECIALS[0]); i++) {
		DestroyCharacter(game, data->special_archetypes[i]);
	}
	for (int i = 0; i < 4; i++) {
		al_destroy_bitmap(data->field_bgs[i]);
	}
	al_destroy_bitmap(data->restart);
	al_destroy_bitmap(data->placeholder);
	al_destroy_bitmap(data->field_bgs_bmp);
	al_destroy_bitmap(data->bg);
	al_destroy_bitmap(data->leaf);
	al_destroy_bitmap(data->scene);
	al_destroy_bitmap(data->lowres_scene);
	al_destroy_bitmap(data->lowres_scene_blur);
	al_destroy_bitmap(data->board);
	al_destroy_font(data->font);
	al_destroy_font(data->font_num_small);
	al_destroy_font(data->font_num_medium);
	al_destroy_font(data->font_num_big);
	al_destroy_font(data->font_small);
	DestroyShader(game, data->combine_shader);
	DestroyShader(game, data->desaturate_shader);
	TM_Destroy(data->timeline);
	free(data);
}

void Gamestate_Start(struct Game* game, struct GamestateResources* data) {
	// Called when this gamestate gets control. Good place for initializing state,
	// playing music etc.
	game->data->last_unlocked_level = -1;

	data->counter = 0.0;
	data->counter_speed = 0.0;
	data->counter_strength = 0.0;
	data->locked = false;
	data->clicked = false;
	data->paused = false;
	data->menu = false;
	data->done = false;
	data->failed = false;
	data->snail_blink = 0.0;
	data->finishing = StaticTween(game, 0.0);
	data->failing = StaticTween(game, 0.0);
	data->scoring = StaticTween(game, 0.0);

	LoadLevel(game, data);
}

void Gamestate_Stop(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets stopped. Stop timers, music etc. here.
	TM_CleanQueue(data->timeline);
	TM_CleanBackgroundQueue(data->timeline);
}

void Gamestate_Pause(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets paused (so only Draw is being called, no Logic nor ProcessEvent)
	// Pause your timers and/or sounds here.
	data->paused = true;
}

void Gamestate_Resume(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets resumed. Resume your timers and/or sounds here.
	data->paused = false;
}

void Gamestate_Reload(struct Game* game, struct GamestateResources* data) {
	// Called when the display gets lost and not preserved bitmaps need to be recreated.
	// Unless you want to support mobile platforms, you should be able to ignore it.
	data->scene = CreateNotPreservedBitmap(game->viewport.width, game->viewport.height);
	data->lowres_scene = CreateNotPreservedBitmap(game->viewport.width / BLUR_DIVIDER, game->viewport.height / BLUR_DIVIDER);
	data->board = CreateNotPreservedBitmap(game->viewport.width, game->viewport.height);
}
