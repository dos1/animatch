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

void Compositor(struct Game* game) {
	struct Gamestate* tmp = GetNextGamestate(game, NULL);
	if (game->data->transition.progress) {
		al_set_target_bitmap(GetGamestateFramebuffer(game, game->data->transition.gamestate));

		al_set_blender(ALLEGRO_ADD, ALLEGRO_ZERO, ALLEGRO_INVERSE_ALPHA);

		double scale = Interpolate((1.0 - game->data->transition.progress) * 0.7 + 0.3, TWEEN_STYLE_QUINTIC_IN) * 6.0;
		al_draw_scaled_rotated_bitmap(game->data->silhouette,
			al_get_bitmap_width(game->data->silhouette) / 2.0,
			al_get_bitmap_height(game->data->silhouette) / 2.0,
			game->viewport.width * game->data->transition.x, game->viewport.height * game->data->transition.y,
			scale, scale, 0.0, 0);

		al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);

		al_set_target_backbuffer(game->display);
	}

	ClearToColor(game, al_map_rgb(0, 0, 0));
	while (tmp) {
		if (IsGamestateVisible(game, tmp)) {
			al_draw_bitmap(GetGamestateFramebuffer(game, tmp), game->clip_rect.x, game->clip_rect.y, 0);
		}
		tmp = GetNextGamestate(game, tmp);
	}
	if (game->data->transition.progress) {
		al_draw_bitmap(GetGamestateFramebuffer(game, game->data->transition.gamestate), game->clip_rect.x, game->clip_rect.y, 0);
	}
}

void PostLogic(struct Game* game, double delta) {
	if (!game->loading.shown && game->data->transition.progress) {
		game->data->transition.progress -= 0.025 * delta / (1 / 60.0);
		if (game->data->transition.progress <= 0.0) {
			DisableCompositor(game);
			game->data->transition.progress = 0.0;
		}
	}
}

void DrawBuildInfo(struct Game* game) {
	SUPPRESS_WARNING("-Wdeprecated-declarations")
	if (game->config.debug.enabled || game->_priv.showconsole) {
		int x, y, w, h;
		al_get_clipping_rectangle(&x, &y, &w, &h);
		al_hold_bitmap_drawing(true);
		DrawTextWithShadow(game->_priv.font_console, al_map_rgb(255, 255, 255), w - 10, h * 0.935, ALLEGRO_ALIGN_RIGHT, "Animatch");
		char revs[255];
		snprintf(revs, 255, "%s-%s", LIBSUPERDERPY_GAME_GIT_REV, LIBSUPERDERPY_GIT_REV);
		DrawTextWithShadow(game->_priv.font_console, al_map_rgb(255, 255, 255), w - 10, h * 0.965, ALLEGRO_ALIGN_RIGHT, revs);
		al_hold_bitmap_drawing(false);
	}
	SUPPRESS_END
}

bool GlobalEventHandler(struct Game* game, ALLEGRO_EVENT* ev) {
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_M)) {
		ToggleMute(game);
	}

	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_F)) {
		ToggleFullscreen(game);
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

	if (game->config.debug.enabled) {
		if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_TAB)) {
			PauseAllGamestates(game);
		}
		if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_CAPSLOCK)) {
			ResumeAllGamestates(game);
		}
	}

	return false;
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

void StartTransition(struct Game* game, float x, float y) {
	if (!game->data->config.less_movement) {
		game->data->transition.progress = 1.0;
		game->data->transition.gamestate = GetCurrentGamestate(game);
		game->data->transition.x = x;
		game->data->transition.y = y;
		EnableCompositor(game, Compositor);
	}
}

void ToggleAudio(struct Game* game) {
	if (game->config.music) {
		game->config.music = 0;
	} else {
		ToggleMute(game);
		if (!game->config.mute) {
			game->config.music = 10;
		}
	}
	al_set_mixer_gain(game->audio.music, game->config.music / 10.0);
}

void UnlockLevel(struct Game* game, int level) {
	if (game->data->unlocked_levels + 1 == level) {
		game->data->unlocked_levels = level;
		game->data->last_unlocked_level = game->data->unlocked_levels;
		char val[255] = {};
		snprintf(val, 255, "%d", game->data->unlocked_levels);
		SetConfigOption(game, "Animatch", "unlocked_levels", val);
	}
}

void RegisterScore(struct Game* game, int level, int moves, int score) {
	char namescore[255] = {}, namemoves[255] = {};
	snprintf(namescore, 255, "level%d-score", level);
	snprintf(namemoves, 255, "level%d-moves", level);

	int s = strtol(GetConfigOptionDefault(game, namescore, "score", "0"), NULL, 0);
	int m = strtol(GetConfigOptionDefault(game, namemoves, "moves", "99999"), NULL, 0);

	char val[255] = {};
	if ((score > s) || (score == s && moves < m)) {
		snprintf(val, 255, "%d", score);
		SetConfigOption(game, namescore, "score", val);
		snprintf(val, 255, "%d", moves);
		SetConfigOption(game, namescore, "moves", val);
	}

	if ((moves < m) || (moves == m && score > s)) {
		snprintf(val, 255, "%d", score);
		SetConfigOption(game, namemoves, "score", val);
		snprintf(val, 255, "%d", moves);
		SetConfigOption(game, namemoves, "moves", val);
	}
}

bool LevelExists(struct Game* game, int id) {
	char* name = malloc(255 * sizeof(char));
	snprintf(name, 255, "%d.lvl", id);

	ALLEGRO_PATH* path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
	ALLEGRO_PATH* p = al_create_path(name);
	al_join_paths(path, p);
	const char* filename = al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP);

	bool val = al_filename_exists(filename);

	snprintf(name, 255, "levels/%d.lvl", id);
	val = val | (FindDataFilePath(game, name) != NULL);
	al_destroy_path(p);
	al_destroy_path(path);
	free(name);
	return val;
}

struct CommonResources* CreateGameData(struct Game* game) {
	struct CommonResources* data = calloc(1, sizeof(struct CommonResources));
	data->kawese_shader = CreateShader(game, GetDataFilePath(game, "shaders/vertex.glsl"), GetDataFilePath(game, "shaders/kawese.glsl"));
	char* names[] = {"silhouette/frog.webp", "silhouette/bee.webp", "silhouette/ladybug.webp", "silhouette/cat.webp", "silhouette/fish.webp"};
	data->silhouette = al_load_bitmap(GetDataFilePath(game, names[rand() % (sizeof(names) / sizeof(names[0]))]));

	data->level = 0;
	data->unlocked_levels = strtol(GetConfigOptionDefault(game, "Animatch", "unlocked_levels", "1"), NULL, 0);
	data->last_unlocked_level = -1;
	data->in_progress = false;

	data->config.less_movement = strtol(GetConfigOptionDefault(game, "Animatch", "less_movement", "0"), NULL, 0);
	data->config.solid_background = strtol(GetConfigOptionDefault(game, "Animatch", "solid_background", "0"), NULL, 0);
	data->config.allow_continuing = strtol(GetConfigOptionDefault(game, "Animatch", "allow_continuing", "0"), NULL, 0);

	return data;
}

void DestroyGameData(struct Game* game) {
	DestroyShader(game, game->data->kawese_shader);
	al_destroy_bitmap(game->data->silhouette);
	free(game->data);
}
