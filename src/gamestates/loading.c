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

/*! \brief Resources used by Loading state. */
struct GamestateResources {
	bool unused;
};

int Gamestate_ProgressCount = -1;

void Gamestate_ProcessEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev){};

void Gamestate_Logic(struct Game* game, struct GamestateResources* data, double delta){};

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	al_draw_filled_rectangle(0, game->viewport.height * 0.98, game->viewport.width, game->viewport.height, al_map_rgba(32, 32, 32, 32));
	al_draw_filled_rectangle(0, game->viewport.height * 0.98, game->loading_progress * game->viewport.width, game->viewport.height, al_map_rgba(128, 128, 128, 128));
};

void* Gamestate_Load(struct Game* game, void (*progress)(struct Game*)) {
	struct GamestateResources* data = malloc(sizeof(struct GamestateResources));
	return data;
}

void Gamestate_Unload(struct Game* game, struct GamestateResources* data) {
	free(data);
}

void Gamestate_Start(struct Game* game, struct GamestateResources* data) {}
void Gamestate_Stop(struct Game* game, struct GamestateResources* data) {}

void Gamestate_Reload(struct Game* game, struct GamestateResources* data) {}

void Gamestate_Pause(struct Game* game, struct GamestateResources* data) {}
void Gamestate_Resume(struct Game* game, struct GamestateResources* data) {}
