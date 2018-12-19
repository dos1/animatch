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

int Gamestate_ProgressCount = 69; // number of loading steps as reported by Gamestate_Load

void Gamestate_Logic(struct Game* game, struct GamestateResources* data, double delta) {
	// Called 60 times per second (by default). Here you should do all your game logic.
	TM_Process(data->timeline, delta);
	UpdateParticles(game, data->particles, delta);
	UpdateTween(&data->acorn_top.tween, delta);
	UpdateTween(&data->acorn_bottom.tween, delta);
	AnimateCharacter(game, data->beetle, delta, 1.0);
	data->snail_blink -= delta;

	if (data->snail_blink < 0) {
		data->snail_blink = 6.0 + rand() / (float)RAND_MAX * 16.0;
		data->snail->pos = rand() % data->snail->spritesheet->frame_count;
		data->snail->frame = &data->snail->spritesheet->frames[data->snail->pos];
	}

	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			AnimateCharacter(game, data->fields[i][j].drawable, delta, 1.0);
			if (data->fields[i][j].overlay_visible) {
				AnimateCharacter(game, data->fields[i][j].overlay, delta, 1.0);
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

#ifdef LIBSUPERDERPY_IMGUI

	if (data->debug) {
		ImVec4 white = {1.0, 1.0, 1.0, 1.0};
		ImVec4 red = {1.0, 0.0, 0.0, 1.0};
		ImVec4 green = {0.0, 1.0, 0.0, 1.0};
		ImVec4 blue = {0.25, 0.25, 1.0, 1.0};
		ImVec4 pink = {1.0, 0.75, 0.75, 1.0};
		ImVec4 gray = {0.5, 0.5, 0.5, 1.0};
		ImVec4 yellow = {1.0, 1.0, 0.0, 1.0};

		igSetNextWindowSize((ImVec2){1024, 700}, ImGuiCond_FirstUseEver);
		igBegin("Animatch Debug Toolbox", &data->debug, 0);
		igSetWindowFontScale(1.5);

		igText("Particles: %d", data->particles->active);
		igText("Possible moves: %d", CountMoves(game, data));
		igSeparator();
		for (int j = 0; j < ROWS; j++) {
			igColumns(COLS, "fields", true);
			for (int i = 0; i < COLS; i++) {
				struct FieldID id = {.i = i, .j = j};
				struct Field* field = GetField(game, data, id);

				igBeginGroup();

				igTextColored(IsSameID(id, data->hovered) ? green : white, "i = %d", i);
				igTextColored(IsSameID(id, data->current) ? blue : white, "j = %d", j);

				switch (field->type) {
					case FIELD_TYPE_ANIMAL:
						igTextColored(field->data.animal.super ? pink : white, "ANIMAL %d%s", field->data.animal.type, field->data.animal.super ? "S" : "");

						if (field->data.animal.sleeping) {
							igTextColored((ImVec4){.x = 0.5, .y = 0.2, .z = 0.9, .w = 1.0}, "SLEEPING");
						} else {
							igText("");
						}

						break;
					case FIELD_TYPE_FREEFALL:
						igTextColored(yellow, "FREEFALL %d", field->data.freefall.variant);
						igText("");
						break;
					case FIELD_TYPE_EMPTY:
						igTextColored(gray, "EMPTY");
						igText("");
						break;
					case FIELD_TYPE_DISABLED:
						igTextColored(gray, "DISABLED");
						igText("");
						break;
					case FIELD_TYPE_COLLECTIBLE:
						igTextColored(yellow, "COLLECT. %d", field->data.collectible.type);
						igText("Variant %d", field->data.collectible.variant);
						break;
					default:
						break;
				}

				if (CanBeMatched(game, data, id)) {
					igTextColored(red, "MATCHABLE");
				} else {
					igText("");
				}

				igText("%s %s %s", field->handled ? "H" : " ", field->matched ? "M" : " ", field->to_remove ? "R" : " ");

				igEndGroup();
				if (igIsItemHovered(0)) {
					data->hovered = (struct FieldID){i, j};
				}
				if (igIsItemClicked(0)) {
					data->current = (struct FieldID){i, j};
				}

				igNextColumn();
			}
			igSeparator();
		}
		igEnd();
	}
#endif
}

static void DrawScene(struct Game* game, struct GamestateResources* data) {
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

static void UpdateBlur(struct Game* game, struct GamestateResources* data) {
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

static void DrawUIElement(struct Game* game, struct Character* ui, enum UI_ELEMENT element) {
	ui->pos = element;
	ui->frame = &ui->spritesheet->frames[ui->pos];
	DrawCharacter(game, ui);
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
			float tint = 1.0 - GetTweenValue(&data->fields[i][j].animation.hiding);
			data->fields[i][j].drawable->tint = al_map_rgba_f(tint, tint, tint, tint);

			int levels = data->fields[i][j].animation.fall_levels;
			int level_no = data->fields[i][j].animation.level_no;
			float tween = Interpolate(GetTweenPosition(&data->fields[i][j].animation.falling), TWEEN_STYLE_EXPONENTIAL_OUT) * (0.5 - level_no * 0.1) +
				sqrt(Interpolate(GetTweenPosition(&data->fields[i][j].animation.falling), TWEEN_STYLE_BOUNCE_OUT)) * (0.5 + level_no * 0.1);

			//			tween = Interpolate(GetTweenPosition(&data->fields[i][j].falling), TWEEN_STYLE_ELASTIC_OUT);
			int levelDiff = (int)(levels * 90 * (1.0 - tween));

			int x = i * 90 + 45, y = j * 90 + 45 + offsetY - levelDiff;
			int swapeeX = data->fields[i][j].animation.swapee.i * 90 + 45, swapeeY = data->fields[i][j].animation.swapee.j * 90 + 45 + offsetY;

			y -= (int)(sin(GetTweenValue(&data->fields[i][j].animation.collecting) * ALLEGRO_PI) * 10);

			SetCharacterPosition(game, data->fields[i][j].drawable, Lerp(x, swapeeX, GetTweenValue(&data->fields[i][j].animation.swapping)), Lerp(y, swapeeY, GetTweenValue(&data->fields[i][j].animation.swapping)), 0);

			if (IsDrawable(data->fields[i][j].type)) {
				al_set_shader_bool("enabled", IsSleeping(&data->fields[i][j]));
				data->fields[i][j].drawable->angle = sin(GetTweenValue(&data->fields[i][j].animation.shaking) * 3 * ALLEGRO_PI) / 6.0 + sin(GetTweenValue(&data->fields[i][j].animation.hinting) * 5 * ALLEGRO_PI) / 6.0 + sin(GetTweenPosition(&data->fields[i][j].animation.collecting) * 2 * ALLEGRO_PI) / 12.0 + sin(GetTweenValue(&data->fields[i][j].animation.launching) * 5 * ALLEGRO_PI) / 6.0;
				data->fields[i][j].drawable->scaleX = 1.0 + sin(GetTweenValue(&data->fields[i][j].animation.hinting) * ALLEGRO_PI) / 3.0 + sin(GetTweenValue(&data->fields[i][j].animation.launching) * ALLEGRO_PI) / 3.0;
				data->fields[i][j].drawable->scaleY = data->fields[i][j].drawable->scaleX;
				DrawCharacter(game, data->fields[i][j].drawable);
				if (data->fields[i][j].overlay_visible) {
					DrawCharacter(game, data->fields[i][j].overlay);
				}
			}
		}
	}
	al_use_shader(NULL);
	al_reset_clipping_rectangle();

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
	DrawUIElement(game, data->ui, UI_ELEMENT_BALOON_BIG);
	DrawUIElement(game, data->ui, UI_ELEMENT_BALOON_MINI);
	DrawUIElement(game, data->ui, UI_ELEMENT_SCORE);
	al_hold_bitmap_drawing(false);

	al_draw_text(data->font, al_map_rgb(64, 72, 5), 622, 51, ALLEGRO_ALIGN_CENTER, "MOVES");
	al_draw_textf(data->font_num_big, al_map_rgb(49, 84, 2), 620, 78, ALLEGRO_ALIGN_CENTER, "%d", data->moves);
	al_draw_text(data->font, al_map_rgb(55, 28, 20), 118, 160, ALLEGRO_ALIGN_CENTER, "LEVEL");
	al_draw_textf(data->font_num_medium, al_map_rgb(255, 255, 194), 118, 195, ALLEGRO_ALIGN_CENTER, "%d", data->level);

	SetCharacterPosition(game, data->snail, 0, 0, 0);
	DrawCharacter(game, data->snail);

	SetCharacterPosition(game, data->beetle, 0, 1194, 0);
	DrawCharacter(game, data->beetle);

	al_draw_bitmap(data->placeholder, 240, 45, 0);

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
}

void Gamestate_ProcessEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev) {
	// Called for each event in Allegro event queue.
	// Here you can handle user input, expiring timers etc.
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		UnloadCurrentGamestate(game); // mark this gamestate to be stopped and unloaded
		// When there are no active gamestates, the engine will quit.
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

	if (game->config.debug.enabled) {
#ifdef LIBSUPERDERPY_IMGUI
		if ((ev->type == ALLEGRO_EVENT_KEY_DOWN && ev->keyboard.keycode == ALLEGRO_KEY_SPACE) || (ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN && ev->mouse.button == 2)) {
			PrintConsole(game, "Debug interface toggled.");
			data->debug = !data->debug;
			return;
		}
#endif
	}

	if (data->locked) {
		return;
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) || (ev->type == ALLEGRO_EVENT_TOUCH_BEGIN)) {
		data->current = data->hovered;
		data->clicked = true;
	}

	if (((ev->type == ALLEGRO_EVENT_MOUSE_AXES) || (ev->type == ALLEGRO_EVENT_TOUCH_MOVE)) && (data->clicked)) {
		if (IsValidID(data->current) && IsValidID(data->hovered)) {
			Turn(game, data);
		}
	}

	if ((ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) || (ev->type == ALLEGRO_EVENT_TOUCH_END)) {
		data->clicked = false;
	}

	if (game->config.debug.enabled) {
		if (ev->type == ALLEGRO_EVENT_KEY_DOWN) {
			if (ev->keyboard.keycode == ALLEGRO_KEY_H) {
				ShowHint(game, data);
				return;
			}
			if (ev->keyboard.keycode == ALLEGRO_KEY_A) {
				AutoMove(game, data);
				return;
			}

			int type = ev->keyboard.keycode - ALLEGRO_KEY_1;

			struct Field* field = GetField(game, data, data->hovered);

			if (!field) {
				return;
			}

			if (ev->keyboard.keycode == ALLEGRO_KEY_S) {
				if (field->type != FIELD_TYPE_ANIMAL) {
					return;
				}
				field->data.animal.sleeping = !field->data.animal.sleeping;
				UpdateDrawable(game, data, field->id);
				PrintConsole(game, "Field %dx%d, sleeping = %d", field->id.i, field->id.j, field->data.animal.sleeping);
				return;
			}

			if (ev->keyboard.keycode == ALLEGRO_KEY_D) {
				if (field->type != FIELD_TYPE_ANIMAL) {
					return;
				}
				field->data.animal.super = !field->data.animal.super;
				UpdateDrawable(game, data, field->id);
				PrintConsole(game, "Field %dx%d, super = %d", field->id.i, field->id.j, field->data.animal.super);
				return;
			}

			if (ev->keyboard.keycode == ALLEGRO_KEY_MINUS) {
				Gravity(game, data);
				ProcessFields(game, data);
			}

			if (type == -1) {
				type = 9;
			}
			if (type < 0) {
				return;
			}
			if (type >= ANIMAL_TYPES + FIELD_TYPES) {
				return;
			}
			if (type >= ANIMAL_TYPES) {
				type -= ANIMAL_TYPES - 1;
				if (field->type == (enum FIELD_TYPE)type) {
					field->data.collectible.type++;
					if (field->data.collectible.type == COLLECTIBLE_TYPES) {
						field->data.collectible.type = 0;
					}
				} else {
					field->data.collectible.type = 0;
				}
				field->type = type;
				if (field->type == FIELD_TYPE_FREEFALL) {
					field->data.freefall.variant = rand() % SPECIAL_ACTIONS[SPECIAL_TYPE_EGG].actions;
				} else {
					field->data.collectible.variant = 0;
				}
				PrintConsole(game, "Setting field type to %d", type);
			} else {
				field->type = FIELD_TYPE_ANIMAL;
				field->data.animal.type = type;
				PrintConsole(game, "Setting animal type to %d", type);
			}
			if (IsDrawable(field->type)) {
				UpdateDrawable(game, data, field->id);
			}
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
	for (unsigned int i = 0; i < sizeof(ANIMALS) / sizeof(ANIMALS[0]); i++) {
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

	for (unsigned int i = 0; i < sizeof(SPECIALS) / sizeof(SPECIALS[0]); i++) {
		data->special_archetypes[i] = CreateCharacter(game, StrToLower(game, SPECIALS[i]));
		for (int j = 0; j < SPECIAL_ACTIONS[i].actions; j++) {
			RegisterSpritesheet(game, data->special_archetypes[i], SPECIAL_ACTIONS[i].names[j]);
		}
		LoadSpritesheets(game, data->special_archetypes[i], progress);
	}

	data->bg = al_load_bitmap(GetDataFilePath(game, "bg.webp"));
	progress(game);

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

	data->placeholder = al_load_bitmap(GetDataFilePath(game, "placeholder.png"));

	data->scene = CreateNotPreservedBitmap(game->viewport.width, game->viewport.height);
	data->lowres_scene = CreateNotPreservedBitmap(game->viewport.width / BLUR_DIVIDER, game->viewport.height / BLUR_DIVIDER);
	data->lowres_scene_blur = al_create_bitmap(game->viewport.width / BLUR_DIVIDER, game->viewport.height / BLUR_DIVIDER);
	data->board = CreateNotPreservedBitmap(game->viewport.width, game->viewport.height);
	progress(game);

	data->combine_shader = CreateShader(game, GetDataFilePath(game, "shaders/vertex.glsl"), GetDataFilePath(game, "shaders/combine.glsl"));
	progress(game);
	data->desaturate_shader = CreateShader(game, GetDataFilePath(game, "shaders/vertex.glsl"), GetDataFilePath(game, "shaders/desaturate.glsl"));
	progress(game);

	data->font = al_load_font(GetDataFilePath(game, "fonts/Caroni.otf"), 35, 0);
	data->font_num_small = al_load_font(GetDataFilePath(game, "fonts/Brizel.ttf"), 42, 0);
	data->font_num_medium = al_load_font(GetDataFilePath(game, "fonts/Brizel.ttf"), 55, 0);
	data->font_num_big = al_load_font(GetDataFilePath(game, "fonts/Brizel.ttf"), 96, 0);

	data->timeline = TM_Init(game, data, "timeline");

	data->particles = CreateParticleBucket(game, 3072, true);

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
	for (unsigned int i = 0; i < sizeof(ANIMALS) / sizeof(ANIMALS[0]); i++) {
		DestroyCharacter(game, data->animal_archetypes[i]);
	}
	for (unsigned int i = 0; i < sizeof(SPECIALS) / sizeof(SPECIALS[0]); i++) {
		DestroyCharacter(game, data->special_archetypes[i]);
	}
	for (int i = 0; i < 4; i++) {
		al_destroy_bitmap(data->field_bgs[i]);
	}
	al_destroy_bitmap(data->placeholder);
	al_destroy_bitmap(data->field_bgs_bmp);
	al_destroy_bitmap(data->bg);
	al_destroy_bitmap(data->scene);
	al_destroy_bitmap(data->lowres_scene);
	al_destroy_bitmap(data->lowres_scene_blur);
	al_destroy_bitmap(data->board);
	al_destroy_font(data->font);
	al_destroy_font(data->font_num_small);
	al_destroy_font(data->font_num_medium);
	al_destroy_font(data->font_num_big);
	DestroyShader(game, data->combine_shader);
	DestroyShader(game, data->desaturate_shader);
	TM_Destroy(data->timeline);
	free(data);
}

void Gamestate_Start(struct Game* game, struct GamestateResources* data) {
	// Called when this gamestate gets control. Good place for initializing state,
	// playing music etc.
	data->locked = false;
	data->clicked = false;
	data->snail_blink = 0.0;
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			GenerateField(game, data, &data->fields[i][j]);
			data->fields[i][j].animation.hiding = StaticTween(game, 0.0);
			data->fields[i][j].animation.falling = StaticTween(game, 1.0);

			data->fields[i][j].animation.time_to_action = (int)((rand() % 250000 + 500000) * (rand() / (double)RAND_MAX));
			data->fields[i][j].animation.time_to_blink = (int)((rand() % 100000 + 200000) * (rand() / (double)RAND_MAX));
		}
	}
	data->current = (struct FieldID){-1, -1};
	while (MarkMatching(game, data)) {
		DoRemoval(game, data);
		Gravity(game, data);
	}
	StopAnimations(game, data);

	data->level = 1;
	data->moves = 0;
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
	data->scene = CreateNotPreservedBitmap(game->viewport.width, game->viewport.height);
	data->lowres_scene = CreateNotPreservedBitmap(game->viewport.width / BLUR_DIVIDER, game->viewport.height / BLUR_DIVIDER);
	data->board = CreateNotPreservedBitmap(game->viewport.width, game->viewport.height);
}
