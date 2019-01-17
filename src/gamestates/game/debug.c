/*! \file debug.c
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

#include "game.h"

void DrawDebugInterface(struct Game* game, struct GamestateResources* data) {
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
