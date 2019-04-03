/*! \file specials.c
 *  \brief Handling of special fields that launch special actions
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

static void TurnFieldToSuper(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	struct Field* field = GetField(game, data, id);
	if (field->locked || !field->match_mark) {
		return;
	}
	field->data.animal.super = true;
	field->to_remove = false;
	UpdateDrawable(game, data, id);
	SpawnParticles(game, data, id, 64);
	data->score += 50;
	data->scoring = Tween(game, 1.0, 0.0, TWEEN_STYLE_SINE_OUT, 1.0);
}

void TurnMatchToSuper(struct Game* game, struct GamestateResources* data, int matched, int mark) {
	if (!data->level.supers) {
		// no supers on this level
		return;
	}

	struct Field *field1 = GetField(game, data, data->swap1), *field2 = GetField(game, data, data->swap2);
	struct FieldID super = {-1, -1};
	if (field1->matched && field1->match_mark == mark) {
		super = field1->id;
	} else if (field2->matched && field2->match_mark == mark) {
		super = field2->id;
	} else {
		int nr = rand() % matched;
		for (int i = 0; i < COLS; i++) {
			for (int j = 0; j < ROWS; j++) {
				if (data->fields[i][j].matched && data->fields[i][j].match_mark == mark) {
					nr--;
					if (nr < 0) {
						super = data->fields[i][j].id;
						break;
					}
				}
			}
		}
		if (nr >= 0) {
			PrintConsole(game, "TurnMatchToSuper failed to select a field!");
			return;
		}
	}

	TurnFieldToSuper(game, data, super);

	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			if (data->fields[i][j].match_mark == mark) {
				data->fields[i][j].match_mark = 0;
				data->fields[i][j].animation.super = super;
			}
		}
	}
}

static TM_ACTION(DoSpawnParticles) {
	TM_RunningOnly;
	struct Field* field = TM_Arg(0);
	int* count = TM_Arg(1);
	SpawnParticles(game, data, field->id, *count);
	data->score += 10;
	data->scoring = Tween(game, 1.0, 0.0, TWEEN_STYLE_SINE_OUT, 1.0);
	free(count);
	return true;
}

static void LaunchSpecial(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	TM_AddAction(data->timeline, AnimateSpecial, TM_Args(GetField(game, data, id)));

	struct FieldID left = ToLeft(id), right = ToRight(id), top = ToTop(id), bottom = ToBottom(id);
	while (IsValidID(left) || IsValidID(right) || IsValidID(top) || IsValidID(bottom)) {
		struct Field* field = GetField(game, data, left);
		if (field) {
			HandleSpecialed(game, data, field);
		}
		field = GetField(game, data, right);
		if (field) {
			HandleSpecialed(game, data, field);
		}
		field = GetField(game, data, top);
		if (field) {
			HandleSpecialed(game, data, field);
		}
		field = GetField(game, data, bottom);
		if (field) {
			HandleSpecialed(game, data, field);
		}
		left = ToLeft(left);
		right = ToRight(right);
		top = ToTop(top);
		bottom = ToBottom(bottom);
	}
	TM_WrapArg(int, count, 64);
	TM_AddAction(data->timeline, DoSpawnParticles, TM_Args(GetField(game, data, id), count));
	data->score += 200;
	data->scoring = Tween(game, 1.0, 0.0, TWEEN_STYLE_SINE_OUT, 1.0);
}

bool AnimateSpecials(struct Game* game, struct GamestateResources* data) {
	bool found = false;
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			if (data->fields[i][j].to_remove && !data->fields[i][j].handled) {
				if (data->fields[i][j].type == FIELD_TYPE_ANIMAL && data->fields[i][j].data.animal.super) {
					data->fields[i][j].handled = true;
					LaunchSpecial(game, data, data->fields[i][j].id);
					found = true;
				}
			}
		}
	}
	return found;
}

TM_ACTION(AnimateSpecial) {
	TM_RunningOnly;
	struct Field* field = TM_Arg(0);
	field->animation.launching = Tween(game, 0.0, 1.0, TWEEN_STYLE_SINE_IN_OUT, LAUNCHING_TIME);
	return true;
}

void HandleSpecialed(struct Game* game, struct GamestateResources* data, struct Field* field) {
	TM_WrapArg(int, count, 64);
	TM_AddAction(data->timeline, DoSpawnParticles, TM_Args(field, count));
	TM_AddDelay(data->timeline, 0.0333);
	if (field->type != FIELD_TYPE_FREEFALL && field->type != FIELD_TYPE_DISABLED) {
		if (field->type == FIELD_TYPE_ANIMAL) {
			SelectSpritesheet(game, field->drawable, ANIMAL_ACTIONS[field->data.animal.type].names[rand() % ANIMAL_ACTIONS[field->type].actions]);
		}
		field->to_remove = true;
		field->to_highlight = true;
	}
}
