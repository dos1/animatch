/*! \file board.c
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

bool IsSameID(struct FieldID one, struct FieldID two) {
	return one.i == two.i && one.j == two.j;
}

bool IsValidID(struct FieldID id) {
	return id.i != -1 && id.j != -1;
}

struct FieldID ToLeft(struct FieldID id) {
	id.i--;
	if (id.i < 0) {
		return (struct FieldID){-1, -1};
	}
	return id;
}

struct FieldID ToRight(struct FieldID id) {
	id.i++;
	if (id.i >= COLS) {
		return (struct FieldID){-1, -1};
	}
	return id;
}

struct FieldID ToTop(struct FieldID id) {
	id.j--;
	if (id.j < 0) {
		return (struct FieldID){-1, -1};
	}
	return id;
}

struct FieldID ToBottom(struct FieldID id) {
	id.j++;
	if (id.j >= ROWS) {
		return (struct FieldID){-1, -1};
	}
	return id;
}

struct Field* GetField(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	if (!IsValidID(id)) {
		return NULL;
	}
	return &data->fields[id.i][id.j];
}
