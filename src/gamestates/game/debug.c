/*! \file debug.c
 *  \brief Debug interface.
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

static void UpdateField(struct Game* game, struct GamestateResources* data, struct Field* field) {
	if (field->type == FIELD_TYPE_FREEFALL) {
		field->data.freefall.variant = fmin(field->data.freefall.variant, SPECIAL_ACTIONS[SPECIAL_TYPE_EGG].actions - 1);
	}
	if (field->type == FIELD_TYPE_ANIMAL) {
		field->data.animal.type = fmin(field->data.animal.type, ANIMAL_TYPES - 1);
	}
	if (field->type == FIELD_TYPE_COLLECTIBLE) {
		field->data.collectible.type = fmin(field->data.collectible.type, COLLECTIBLE_TYPES - 1);
		field->data.collectible.variant = 0;
	}
	UpdateDrawable(game, data, field->id);
}

void HandleDebugEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev) {
#ifdef LIBSUPERDERPY_IMGUI
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN && ev->keyboard.keycode == ALLEGRO_KEY_SPACE) || (ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN && ev->mouse.button == 2)) {
		PrintConsole(game, "Debug interface toggled.");
		data->debug = !data->debug;
		return;
	}
#endif

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
			UpdateField(game, data, field);
			PrintConsole(game, "Field %dx%d, sleeping = %d", field->id.i, field->id.j, field->data.animal.sleeping);
			return;
		}

		if (ev->keyboard.keycode == ALLEGRO_KEY_D) {
			if (field->type != FIELD_TYPE_ANIMAL) {
				return;
			}
			field->data.animal.super = !field->data.animal.super;
			UpdateField(game, data, field);
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
		UpdateField(game, data, field);
	}
}

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
		ImVec4 purple = {0.5, 0.0, 0.5, 1.0};
		ImVec4 violet = {0.5, 0.25, 1.0, 1.0};

		igSetNextWindowSize((ImVec2){1024, 700}, ImGuiCond_FirstUseEver);
		igBegin("Animatch Debug Toolbox", &data->debug, 0);
		igSetWindowFontScale(1.5);

		igTextColored(data->locked ? gray : white, "Enabled: %d", !data->locked);
		igText("Particles: %d", data->particles->active);
		igText("Possible moves: %d", CountMoves(game, data));
		igSeparator();

		if (igCollapsingHeader("Board", 0)) {
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
								igTextColored(violet, "SLEEPING");
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

					if (field->matched) {
						igTextColored(purple, "MATCHED %d", field->match_mark);
					} else if (CanBeMatched(game, data, id)) {
						igTextColored(red, "MATCHABLE");
					} else {
						igText("");
					}

					igText("%s %s %s", field->handled ? "H" : " ", field->matched ? "M" : " ", field->to_remove ? "R" : " ");

					igEndGroup();
					if (igIsItemHovered(0)) {
						data->hovered = (struct FieldID){i, j};
					}

					char buf[12];
					snprintf(buf, 12, "transmute%d", j * COLS + i);

					if (igIsItemClicked(0)) {
						data->current = (struct FieldID){i, j};
						igOpenPopup(buf);
					}

					igSetWindowFontScale(1.5);
					if (igBeginPopup(buf, 0)) {

#define AddFieldTypeMenuItem(t)                                      \
	if (igMenuItemBool(#t, "", field->type == FIELD_TYPE_##t, true)) { \
		field->type = FIELD_TYPE_##t;                                    \
		UpdateField(game, data, field);                                  \
	}

						FOREACH_FIELD_TYPE(AddFieldTypeMenuItem)

						igSeparator();

						if (field->type == FIELD_TYPE_COLLECTIBLE) {
#define AddCollectibleMenuItem(t)                                                           \
	if (igMenuItemBool(#t, "", field->data.collectible.type == COLLECTIBLE_TYPE_##t, true)) { \
		field->data.collectible.type = COLLECTIBLE_TYPE_##t;                                    \
		UpdateField(game, data, field);                                                         \
	}

							FOREACH_COLLECTIBLE(AddCollectibleMenuItem)
						} else if (field->type == FIELD_TYPE_ANIMAL) {
							char b[2] = "0";

#define AddAnimalMenuItem(t)                                                     \
	b[0] = '1' + ANIMAL_TYPE_##t;                                                  \
	if (igMenuItemBool(#t, b, field->data.animal.type == ANIMAL_TYPE_##t, true)) { \
		field->data.animal.type = ANIMAL_TYPE_##t;                                   \
		UpdateField(game, data, field);                                              \
	}

							FOREACH_ANIMAL(AddAnimalMenuItem)

							igSeparator();

							if (igMenuItemBool("Sleeping", "S", field->data.animal.sleeping, true)) {
								field->data.animal.sleeping = !field->data.animal.sleeping;
								UpdateField(game, data, field);
							}
							if (igMenuItemBool("Super", "D", field->data.animal.super, true)) {
								field->data.animal.super = !field->data.animal.super;
								UpdateField(game, data, field);
							}
						}
						igEndPopup();
					}

					igNextColumn();
				}

				igSeparator();
			}
			igColumns(1, NULL, false);
		}

		if (igButton("Auto-move", (ImVec2){0, 0})) {
			AutoMove(game, data);
		}

		igSameLine(0, 0);
		if (igButton("Hint", (ImVec2){0, 0})) {
			ShowHint(game, data);
		}

		igSameLine(0, 0);
		if (igButton("Process", (ImVec2){0, 0})) {
			Gravity(game, data);
			ProcessFields(game, data);
		}

		igEnd();
	}
#endif
}
