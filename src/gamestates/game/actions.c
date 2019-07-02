/*! \file actions.c
 *  \brief Actions performed by the player.
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

void Turn(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two) {
	if (!IsValidID(one) || !IsValidID(two)) {
		return;
	}

	if (!IsValidMove(one, two)) {
		return;
	}

	PrintConsole(game, "swap %dx%d with %dx%d", one.i, one.j, two.i, two.j);
	data->clicked = false;

	if (!AreSwappable(game, data, one, two)) {
		GetField(game, data, one)->animation.shaking = Tween(game, 0.0, 1.0, TWEEN_STYLE_SINE_OUT, SHAKING_TIME);
		return;
	}

	if (WillMatchAfterSwapping(game, data, one, two)) {
		data->moves++;
		StartSwapping(game, data, one, two);
	} else {
		StartBadSwapping(game, data, one, two);
	}
}

bool ShowHint(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			struct FieldID id = {.i = i, .j = j};
			if (CanBeMatched(game, data, id)) {
				GetField(game, data, id)->animation.hinting = Tween(game, 0.0, 1.0, TWEEN_STYLE_SINE_IN_OUT, HINT_TIME);
				return true;
			}
		}
	}
	return false;
}

bool AutoMove(struct Game* game, struct GamestateResources* data) {
	if (data->locked) {
		return false;
	}
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			struct FieldID id = {.i = i, .j = j};
			struct FieldID (*callbacks[])(struct FieldID) = {ToLeft, ToRight, ToTop, ToBottom};

			for (int q = 0; q < 4; q++) {
				if (IsValidMove(id, callbacks[q](id))) {
					if (WillMatch(game, data, id, callbacks[q](id))) {
						data->moves++;
						StartSwapping(game, data, id, callbacks[q](id));
						return true;
					}
				}
			}
		}
	}
	return false;
}
