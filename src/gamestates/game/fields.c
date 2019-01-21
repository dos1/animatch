/*! \file fields.c
 *  \brief Routines for handling the fields.
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

// TODO: there's a lot of struct allocation for passing and returning values
// which makes everything slow in debug builds. It might be a good idea to refactor
// this and use pointers as much as possible.

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

bool IsSleeping(struct Field* field) {
	return (field->type == FIELD_TYPE_ANIMAL && field->data.animal.sleeping);
}

inline bool IsDrawable(enum FIELD_TYPE type) {
	return (type == FIELD_TYPE_ANIMAL) || (type == FIELD_TYPE_FREEFALL) || (type == FIELD_TYPE_COLLECTIBLE);
}

int IsMatching(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	int lchain = 0, tchain = 0;
	if (!IsValidID(id)) {
		return 0;
	}
	struct Field* orig = GetField(game, data, id);
	if (orig->type != FIELD_TYPE_ANIMAL) {
		return 0;
	}
	if (IsSleeping(orig)) {
		return 0;
	}

	struct Field *lfields[COLS] = {}, *tfields[ROWS] = {};
	struct FieldID (*callbacks[])(struct FieldID) = {ToLeft, ToRight, ToTop, ToBottom};
	int* accumulators[] = {&lchain, &lchain, &tchain, &tchain};
	struct Field** lists[] = {lfields, lfields, tfields, tfields};

	for (int i = 0; i < 4; i++) {
		struct FieldID pos = (callbacks[i])(id);
		while (IsValidID(pos)) {
			struct Field* field = GetField(game, data, pos);
			if (field->type != FIELD_TYPE_ANIMAL) {
				break;
			}
			if (field->data.animal.type != orig->data.animal.type) {
				break;
			}
			if (IsSleeping(field)) {
				break;
			}
			lists[i][*accumulators[i]] = field;
			(*accumulators[i])++;
			pos = (callbacks[i])(pos);
		}
	}

	int chain = 0;
	if (lchain >= 2) {
		chain += lchain;
	}
	if (tchain >= 2) {
		chain += tchain;
	}
	if (chain) {
		chain++;
		if (!orig->match_mark) {
			orig->match_mark = id.j * COLS + id.i;
		}
		if (lchain >= 2) {
			for (int i = 0; i < lchain; i++) {
				if (lfields[i]->match_mark < orig->match_mark) {
					lfields[i]->match_mark = orig->match_mark;
				}
			}
		}
		if (tchain >= 2) {
			for (int i = 0; i < tchain; i++) {
				if (tfields[i]->match_mark < orig->match_mark) {
					tfields[i]->match_mark = orig->match_mark;
				}
			}
		}
	}
	//PrintConsole(game, "field %dx%d %s lchain %d tchain %d chain %d", id.i, id.j, ANIMALS[orig->type], lchain, tchain, chain);
	return chain;
}

static bool AreAdjacentMatching(struct Game* game, struct GamestateResources* data, struct FieldID id, struct FieldID (*func)(struct FieldID)) {
	for (int i = 0; i < 3; i++) {
		id = func(id);
		if (!IsValidID(id) || !GetField(game, data, id)->matched) {
			return false;
		}
	}
	return true;
}

int IsMatchExtension(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	return AreAdjacentMatching(game, data, id, ToTop) || AreAdjacentMatching(game, data, id, ToBottom) || AreAdjacentMatching(game, data, id, ToLeft) || AreAdjacentMatching(game, data, id, ToRight);
}

int ShouldBeCollected(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	if (GetField(game, data, id)->handled) {
		PrintConsole(game, "Not collecting already handled field %d, %d", id.i, id.j);
		return false;
	}
	return IsMatching(game, data, ToTop(id)) || IsMatching(game, data, ToBottom(id)) || IsMatching(game, data, ToLeft(id)) || IsMatching(game, data, ToRight(id));
}

bool IsValidMove(struct FieldID one, struct FieldID two) {
	if (one.i == two.i && abs(one.j - two.j) == 1) {
		return true;
	}
	if (one.j == two.j && abs(one.i - two.i) == 1) {
		return true;
	}
	return false;
}

bool IsSwappable(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	if (!IsValidID(id)) {
		return false;
	}
	struct Field* field = GetField(game, data, id);
	if (((field->type == FIELD_TYPE_ANIMAL) && (!IsSleeping(field))) || (field->type == FIELD_TYPE_COLLECTIBLE)) {
		return true;
	}
	return false;
}

bool AreSwappable(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two) {
	return IsSwappable(game, data, one) && IsSwappable(game, data, two);
}

bool WillMatch(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two) {
	if (!AreSwappable(game, data, one, two)) {
		return false;
	}
	bool res = false;
	Swap(game, data, one, two);
	if (IsMatching(game, data, two)) {
		res = true;
	}
	Swap(game, data, one, two);
	return res;
}

bool CanBeMatched(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	struct FieldID (*callbacks[])(struct FieldID) = {ToLeft, ToRight, ToTop, ToBottom};

	for (int q = 0; q < 4; q++) {
		if (IsValidMove(id, callbacks[q](id))) {
			if (WillMatch(game, data, id, callbacks[q](id))) {
				return true;
			}
		}
	}
	return false;
}
