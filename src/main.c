/*! \file main.c
 *  \brief Main file of Super Examples.
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

int main(int argc, char** argv) {
	srand(time(NULL));

	al_set_org_name("Holy Pangolin");
	al_set_app_name(LIBSUPERDERPY_GAMENAME_PRETTY);

	struct Game* game = libsuperderpy_init(argc, argv, LIBSUPERDERPY_GAMENAME,
	  (struct Params){
	    720,
	    1440,
	    .show_loading_on_launch = true,
	    .handlers = (struct Handlers){
	      .event = GlobalEventHandler,
	      .destroy = DestroyGameData,
	      .postdraw = DrawBuildInfo,
	      .postlogic = PostLogic,
	    },
	  });

	if (!game) { return 1; }

	LoadGamestate(game, "menu");
	LoadGamestate(game, "game");

	StartGamestate(game, "menu");

	game->data = CreateGameData(game);

	al_show_mouse_cursor(game->display);

	return libsuperderpy_run(game);
}
