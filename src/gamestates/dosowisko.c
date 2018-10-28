/*! \file dosowisko.c
 *  \brief Init animation with dosowisko.net logo.
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
#include <math.h>

#define NEXT_GAMESTATE "game"
#define SKIP_GAMESTATE NEXT_GAMESTATE

struct GamestateResources {
	ALLEGRO_FONT* font;
	ALLEGRO_SAMPLE *sample, *kbd_sample, *key_sample;
	ALLEGRO_SAMPLE_INSTANCE *sound, *kbd, *key;
	ALLEGRO_BITMAP *bitmap, *checkerboard, *pixelator;
	int pos;
	double fade, tan;
	char text[255];
	bool underscore, fadeout;
	struct Timeline* timeline;
};

int Gamestate_ProgressCount = 5;

static const char* text = "# dosowisko.net";

//==================================Timeline manager actions BEGIN
static TM_ACTION(FadeIn) {
	switch (action->state) {
		case TM_ACTIONSTATE_START:
			data->fade = 0;
			return false;
		case TM_ACTIONSTATE_RUNNING:
			data->fade += 2 * action->delta / (1 / 60.0);
			data->tan += action->delta / (1 / 60.0);
			return data->fade >= 255;
		case TM_ACTIONSTATE_DESTROY:
			data->fade = 255;
			return false;
		default:
			return false;
	}
}

static TM_ACTION(FadeOut) {
	TM_RunningOnly;
	data->fadeout = true;
	return true;
}

static TM_ACTION(End) {
	TM_RunningOnly;
	SwitchCurrentGamestate(game, NEXT_GAMESTATE);
	return true;
}

static TM_ACTION(Play) {
	TM_RunningOnly;
	al_play_sample_instance(TM_Arg(0));
	return true;
}

static TM_ACTION(Type) {
	TM_RunningOnly;
	strncpy(data->text, text, data->pos++);
	data->text[data->pos] = 0;
	if (strcmp(data->text, text) != 0) {
		TM_AddBackgroundAction(data->timeline, Type, NULL, 60 + rand() % 60);
	} else {
		al_stop_sample_instance(data->kbd);
	}
	return true;
}
//==================================Timeline manager actions END

void Gamestate_Logic(struct Game* game, struct GamestateResources* data, double delta) {
	TM_Process(data->timeline, delta);
	data->underscore = Fract(game->time) >= 0.5;
}

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	if (!data->fadeout) {
		char t[255] = "";
		strncpy(t, data->text, 255);
		if (data->underscore) {
			strncat(t, "_", 2);
		} else {
			strncat(t, " ", 2);
		}

		al_set_target_bitmap(data->bitmap);
		al_clear_to_color(al_map_rgba(0, 0, 0, 0));

		al_draw_text(data->font, al_map_rgba(255, 255, 255, 10), 320 / 2.0,
			480 * 0.4667, ALLEGRO_ALIGN_CENTRE, t);

		double tg = tan(-data->tan / 384.0 * ALLEGRO_PI - ALLEGRO_PI / 2);

		int fade = data->fadeout ? 255 : (int)data->fade;

		al_set_target_bitmap(data->pixelator);
		al_clear_to_color(al_map_rgb(35, 31, 32));

		al_draw_tinted_scaled_bitmap(data->bitmap, al_map_rgba(fade, fade, fade, fade), 0, 0,
			al_get_bitmap_width(data->bitmap), al_get_bitmap_height(data->bitmap),
			-tg * al_get_bitmap_width(data->bitmap) * 0.05,
			-tg * al_get_bitmap_height(data->bitmap) * 0.05,
			al_get_bitmap_width(data->bitmap) + tg * 0.1 * al_get_bitmap_width(data->bitmap),
			al_get_bitmap_height(data->bitmap) + tg * 0.1 * al_get_bitmap_height(data->bitmap),
			0);

		al_draw_bitmap(data->checkerboard, 0, 0, 0);

		SetFramebufferAsTarget(game);

		al_draw_scaled_bitmap(data->pixelator, 0, 0, al_get_bitmap_width(data->pixelator), al_get_bitmap_height(data->pixelator), 0, 0, game->viewport.width, game->viewport.height, 0);
	}
}

void Gamestate_Start(struct Game* game, struct GamestateResources* data) {
	data->pos = 1;
	data->fade = 0;
	data->tan = 64;
	data->fadeout = false;
	data->underscore = true;
	strncpy(data->text, "#", 255);
	TM_AddDelay(data->timeline, 300);
	TM_AddQueuedBackgroundAction(data->timeline, FadeIn, NULL, 0);
	TM_AddDelay(data->timeline, 1500);
	TM_AddNamedAction(data->timeline, Play, TM_Args(data->kbd), "PlayKbd");
	TM_AddQueuedBackgroundAction(data->timeline, Type, NULL, 0);
	TM_AddDelay(data->timeline, 3200);
	TM_AddNamedAction(data->timeline, Play, TM_Args(data->key), "PlayKey");
	TM_AddDelay(data->timeline, 50);
	TM_AddAction(data->timeline, FadeOut, NULL);
	TM_AddDelay(data->timeline, 1000);
	TM_AddAction(data->timeline, End, NULL);
	al_play_sample_instance(data->sound);
}

void Gamestate_ProcessEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev) {
	if (((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) || (ev->type == ALLEGRO_EVENT_TOUCH_END)) {
		UnloadAllGamestates(game);
		StartGamestate(game, SKIP_GAMESTATE);
	}
}

void* Gamestate_Load(struct Game* game, void (*progress)(struct Game*)) {
	struct GamestateResources* data = malloc(sizeof(struct GamestateResources));
	int flags = al_get_new_bitmap_flags();
	al_set_new_bitmap_flags(flags ^ ALLEGRO_MAG_LINEAR);

	data->timeline = TM_Init(game, data, "main");
	data->bitmap = CreateNotPreservedBitmap(320, 480);
	data->checkerboard = al_create_bitmap(320, 480);
	data->pixelator = CreateNotPreservedBitmap(320, 480);

	al_set_target_bitmap(data->checkerboard);
	al_lock_bitmap(data->checkerboard, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
	int x, y;
	for (x = 0; x < al_get_bitmap_width(data->checkerboard); x = x + 2) {
		for (y = 0; y < al_get_bitmap_height(data->checkerboard); y = y + 2) {
			al_put_pixel(x, y, al_map_rgba(0, 0, 0, 64));
			al_put_pixel(x + 1, y, al_map_rgba(0, 0, 0, 0));
			al_put_pixel(x, y + 1, al_map_rgba(0, 0, 0, 0));
			al_put_pixel(x + 1, y + 1, al_map_rgba(0, 0, 0, 0));
		}
	}
	al_unlock_bitmap(data->checkerboard);
	al_set_target_backbuffer(game->display);
	(*progress)(game);

	data->font = al_load_font(GetDataFilePath(game, "fonts/DejaVuSansMono.ttf"),
		(int)(180 * 0.1666 / 8) * 8, 0);
	(*progress)(game);
	data->sample = al_load_sample(GetDataFilePath(game, "dosowisko.flac"));
	data->sound = al_create_sample_instance(data->sample);
	al_attach_sample_instance_to_mixer(data->sound, game->audio.music);
	al_set_sample_instance_playmode(data->sound, ALLEGRO_PLAYMODE_ONCE);
	(*progress)(game);

	data->kbd_sample = al_load_sample(GetDataFilePath(game, "kbd.flac"));
	data->kbd = al_create_sample_instance(data->kbd_sample);
	al_attach_sample_instance_to_mixer(data->kbd, game->audio.fx);
	al_set_sample_instance_playmode(data->kbd, ALLEGRO_PLAYMODE_ONCE);
	(*progress)(game);

	data->key_sample = al_load_sample(GetDataFilePath(game, "key.flac"));
	data->key = al_create_sample_instance(data->key_sample);
	al_attach_sample_instance_to_mixer(data->key, game->audio.fx);
	al_set_sample_instance_playmode(data->key, ALLEGRO_PLAYMODE_ONCE);
	(*progress)(game);

	al_set_new_bitmap_flags(flags);

	return data;
}

void Gamestate_Stop(struct Game* game, struct GamestateResources* data) {
	al_stop_sample_instance(data->sound);
	al_stop_sample_instance(data->kbd);
	al_stop_sample_instance(data->key);
}

void Gamestate_Unload(struct Game* game, struct GamestateResources* data) {
	al_destroy_font(data->font);
	al_destroy_sample_instance(data->sound);
	al_destroy_sample(data->sample);
	al_destroy_sample_instance(data->kbd);
	al_destroy_sample(data->kbd_sample);
	al_destroy_sample_instance(data->key);
	al_destroy_sample(data->key_sample);
	al_destroy_bitmap(data->bitmap);
	al_destroy_bitmap(data->checkerboard);
	al_destroy_bitmap(data->pixelator);
	TM_Destroy(data->timeline);
	free(data);
}

void Gamestate_Reload(struct Game* game, struct GamestateResources* data) {
	int flags = al_get_new_bitmap_flags();
	al_set_new_bitmap_flags(flags ^ ALLEGRO_MAG_LINEAR);
	data->bitmap = CreateNotPreservedBitmap(320, 480);
	data->pixelator = CreateNotPreservedBitmap(320, 480);
	al_set_new_bitmap_flags(flags);
}
