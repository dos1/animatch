/*! \file logic.c
 *  \brief Game logic (matching, collecting, dispatching animations...)
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

/*
 * What consists of a typical turn:
 *  - AnimateSwapping gets called, putting StartSwapping and AfterSwapping to the timeline queue
 *
 *  - AfterSwapping does the actual swap in the memory and calls ProcessFields
 *  - ProcessFields is where the actual magic happens:
 *    - If there's nothing to do, it does nothing and stops there. Otherwise...
 *    - MarkMatching finds matches and sets proper field attributes
 *    - Collect gets called, dealing with collectibles
 *    - AnimateSpecials triggers and queues animation for the special fields
 *    - if necessary, Collect and AnimateSpecials are repeated as long as there's something to do for them
 *    - DispatchAnimations is queued after any previously queued animation
 *
 *  - DispatchAnimations does two things:
 *    - call PerformActions, which triggers animations and turns fields into specials
 *    - queries AfterMatching
 *
 *  - AfterMatching then calls:
 *    - DoRemoval, which actually removes the fields that we've just animated away
 *    - Gravity moves the fields to fill up the free space, generating new ones if there's such need
 *    - and goes back to ProcessFields, which may handle combos now, or may just stop if there are none.
 *
 *  (routine naming is hard...)
 */

int MarkMatching(struct Game* game, struct GamestateResources* data) {
	int matching = 0;
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->fields[i][j].matched = IsMatching(game, data, (struct FieldID){i, j});
			if (data->fields[i][j].matched) {
				data->fields[i][j].to_remove = true;
				data->fields[i][j].to_highlight = true;
				matching++;
			}
		}
	}
	PrintConsole(game, "matched %d", matching);
	return matching;
}

static int Collect(struct Game* game, struct GamestateResources* data) {
	int collected = 0;
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			if (data->fields[i][j].type == FIELD_TYPE_FREEFALL) {
				if (j == ROWS - 1) {
					data->fields[i][j].to_remove = true;
					data->fields[i][j].handled = true;
					data->fields[i][j].to_highlight = true;
					collected++;
				}
			} else if (ShouldBeCollected(game, data, data->fields[i][j].id) || data->fields[i][j].to_remove) {
				if (IsSleeping(&data->fields[i][j])) {
					data->fields[i][j].data.animal.sleeping = false;
					data->fields[i][j].to_remove = false;
					data->fields[i][j].handled = true;
					UpdateDrawable(game, data, data->fields[i][j].id);
					data->fields[i][j].animation.collecting = Tween(game, 0.0, 1.0, TWEEN_STYLE_BOUNCE_OUT, COLLECTING_TIME);
					data->fields[i][j].to_highlight = true;
					collected++;
				} else if (data->fields[i][j].type == FIELD_TYPE_COLLECTIBLE) {
					data->fields[i][j].data.collectible.variant++;

					if (data->fields[i][j].data.collectible.variant >= SPECIAL_ACTIONS[FIRST_COLLECTIBLE + data->fields[i][j].data.collectible.type].actions) {
						data->fields[i][j].data.collectible.variant = SPECIAL_ACTIONS[FIRST_COLLECTIBLE + data->fields[i][j].data.collectible.type].actions - 1;
						data->fields[i][j].to_remove = true;
						PrintConsole(game, "collecting field %d, %d", i, j);
					} else {
						data->fields[i][j].to_remove = false;
						PrintConsole(game, "advancing field %d, %d", i, j);
					}
					UpdateDrawable(game, data, data->fields[i][j].id);
					data->fields[i][j].animation.collecting = Tween(game, 0.0, 1.0, TWEEN_STYLE_BOUNCE_OUT, COLLECTING_TIME);
					data->fields[i][j].handled = true;
					data->fields[i][j].to_highlight = true;
					collected++;
				}
			}
		}
	}

	PrintConsole(game, "collected %d", collected);
	return collected;
}

void GenerateField(struct Game* game, struct GamestateResources* data, struct Field* field) {
	while (true) {
		if (rand() / (float)RAND_MAX < 0.001) {
			field->type = FIELD_TYPE_FREEFALL;
			field->data.freefall.variant = rand() % SPECIAL_ACTIONS[SPECIAL_TYPE_EGG].actions;
			if (data->level.specials[SPECIAL_TYPE_EGG]) {
				break;
			}
		} else if (rand() / (float)RAND_MAX < 0.01) {
			field->type = FIELD_TYPE_COLLECTIBLE;
			while (data->level.fields[FIELD_TYPE_COLLECTIBLE]) {
				field->data.collectible.type = rand() % COLLECTIBLE_TYPES;
				if (data->level.specials[FIRST_COLLECTIBLE + field->data.collectible.type]) {
					break;
				}
			}
			field->data.collectible.variant = 0;
			if (data->level.fields[FIELD_TYPE_COLLECTIBLE]) {
				break;
			}
		} else {
			field->type = FIELD_TYPE_ANIMAL;
			while (data->level.fields[FIELD_TYPE_ANIMAL]) {
				field->data.animal.type = rand() % ANIMAL_TYPES;
				if (data->level.animals[field->data.animal.type]) {
					break;
				}
			}
			if (rand() / (float)RAND_MAX < 0.005) {
				field->data.animal.sleeping = true;
			}
			field->data.animal.super = false;
			if (data->level.fields[FIELD_TYPE_ANIMAL]) {
				break;
			}
		}
	}
	field->overlay_visible = false;
	field->locked = true;
	UpdateDrawable(game, data, field->id);
}

static void CreateNewField(struct Game* game, struct GamestateResources* data, struct Field* field) {
	GenerateField(game, data, field);
	field->animation.fall_levels++;
	field->animation.falling = Tween(game, 0.0, 1.0, TWEEN_STYLE_BOUNCE_OUT, FALLING_TIME * (1.0 + field->animation.level_no * 0.025));
	field->animation.hiding = Tween(game, 1.0, 0.0, TWEEN_STYLE_LINEAR, 0.25);
}

void Gravity(struct Game* game, struct GamestateResources* data) {
	bool repeat;
	do {
		repeat = false;
		for (int i = 0; i < COLS; i++) {
			for (int j = ROWS - 1; j >= 0; j--) {
				struct FieldID id = (struct FieldID){i, j};
				struct Field* field = GetField(game, data, id);
				if (field->type != FIELD_TYPE_EMPTY) {
					continue;
				}
				struct FieldID up = ToTop(id);
				while (IsValidID(up) && (GetField(game, data, up)->type == FIELD_TYPE_DISABLED)) {
					up = ToTop(up);
				}
				if (IsValidID(up)) {
					struct Field* upfield = GetField(game, data, up);
					if (upfield->type == FIELD_TYPE_EMPTY) {
						repeat = true;
					} else {
						upfield->animation.level_no = field->animation.level_no++;
						upfield->animation.fall_levels++;
						upfield->animation.falling = Tween(game, 0.0, 1.0, TWEEN_STYLE_BOUNCE_OUT, FALLING_TIME * (1.0 + upfield->animation.level_no * 0.025));
						upfield->animation.falling.predelay = upfield->animation.level_no * 0.01;
						Swap(game, data, id, up);
					}
				} else {
					CreateNewField(game, data, field);
				}
			}
		}
	} while (repeat);

	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->fields[i][j].locked = false;
		}
	}

	data->locked = false;
}

void ProcessFields(struct Game* game, struct GamestateResources* data) {
	bool matched = MarkMatching(game, data);
	bool collected = Collect(game, data);
	if (matched || collected) {
		while (AnimateSpecials(game, data)) {
			Collect(game, data);
		}
		TM_AddAction(data->timeline, DispatchAnimations, NULL);
	}
	PrintConsole(game, "possible moves: %d", CountMoves(game, data));
}

int CountMoves(struct Game* game, struct GamestateResources* data) {
	int moves = 0;
	bool marked[COLS][ROWS] = {};
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			struct FieldID id = {.i = i, .j = j};
			if (CanBeMatched(game, data, id)) {
				if (!marked[i][j]) {
					moves++;
					marked[i][j] = true;
				}
			}
		}
	}
	return moves;
}

static void AnimateRemoval(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			if (data->fields[i][j].to_remove) {
				data->fields[i][j].animation.hiding = Tween(game, 0.0, 1.0, TWEEN_STYLE_LINEAR, MATCHING_TIME);
				data->fields[i][j].animation.hiding.predelay = MATCHING_DELAY_TIME;
			}
		}
	}
}

static void PerformActions(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			if (data->fields[i][j].matched) {
				if (data->fields[i][j].type == FIELD_TYPE_ANIMAL) {
					SelectSpritesheet(game, data->fields[i][j].drawable, ANIMAL_ACTIONS[data->fields[i][j].data.animal.type].names[rand() % ANIMAL_ACTIONS[data->fields[i][j].type].actions]);

					if (data->fields[i][j].matched >= 4 && data->fields[i][j].match_mark) {
						TurnMatchToSuper(game, data, data->fields[i][j].matched, data->fields[i][j].match_mark);
					}
				}
				data->locked = true;

				SpawnParticles(game, data, data->fields[i][j].id, 16);
			}
		}
	}
	AnimateRemoval(game, data);
}

void DoRemoval(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->fields[i][j].animation.fall_levels = 0;
			data->fields[i][j].animation.level_no = 0;
			data->fields[i][j].animation.super = (struct FieldID){-1, -1};
			data->fields[i][j].handled = false;
			data->fields[i][j].matched = 0;
			data->fields[i][j].match_mark = 0;
			data->fields[i][j].to_highlight = false;
			if (data->fields[i][j].to_remove) {
				data->fields[i][j].type = FIELD_TYPE_EMPTY;
				data->fields[i][j].to_remove = false;
				data->fields[i][j].animation.hiding = StaticTween(game, 0.0);
				data->fields[i][j].animation.falling = StaticTween(game, 1.0);
				data->fields[i][j].animation.shaking = StaticTween(game, 0.0);
				data->fields[i][j].animation.hinting = StaticTween(game, 0.0);
				data->fields[i][j].animation.launching = StaticTween(game, 0.0);
				data->fields[i][j].animation.collecting = StaticTween(game, 0.0);
			}
		}
	}
}

void StopAnimations(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->fields[i][j].animation.fall_levels = 0;
			data->fields[i][j].animation.level_no = 0;
			data->fields[i][j].animation.hiding = StaticTween(game, 0.0);
			data->fields[i][j].animation.falling = StaticTween(game, 1.0);
			data->fields[i][j].animation.shaking = StaticTween(game, 0.0);
			data->fields[i][j].animation.hinting = StaticTween(game, 0.0);
			data->fields[i][j].animation.launching = StaticTween(game, 0.0);
			data->fields[i][j].animation.collecting = StaticTween(game, 0.0);
			data->fields[i][j].animation.super = (struct FieldID){-1, -1};
		}
	}
}

void Swap(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two) {
	struct Field tmp = data->fields[one.i][one.j];
	data->fields[one.i][one.j] = data->fields[two.i][two.j];
	data->fields[two.i][two.j] = tmp;
	data->fields[one.i][one.j].id = (struct FieldID){.i = one.i, .j = one.j};
	data->fields[two.i][two.j].id = (struct FieldID){.i = two.i, .j = two.j};
	float highlight = data->fields[one.i][one.j].highlight;
	data->fields[one.i][one.j].highlight = data->fields[two.i][two.j].highlight;
	data->fields[two.i][two.j].highlight = highlight;
}

static TM_ACTION(AfterSwapping) {
	TM_RunningOnly;
	struct Field* one = TM_GetArg(action->arguments, 0);
	struct Field* two = TM_GetArg(action->arguments, 1);
	Swap(game, data, one->id, two->id);
	one->animation.swapping = StaticTween(game, 0.0);
	two->animation.swapping = StaticTween(game, 0.0);
	data->locked = false;
	ProcessFields(game, data);
	return true;
}

static TM_ACTION(StartSwapping) {
	TM_RunningOnly;
	struct Field* one = TM_GetArg(action->arguments, 0);
	struct Field* two = TM_GetArg(action->arguments, 1);
	data->locked = true;
	one->animation.swapping = Tween(game, 0.0, 1.0, TWEEN_STYLE_SINE_IN_OUT, SWAPPING_TIME);
	one->animation.swapee = two->id;
	two->animation.swapping = Tween(game, 0.0, 1.0, TWEEN_STYLE_SINE_IN_OUT, SWAPPING_TIME);
	two->animation.swapee = one->id;
	return true;
}

void AnimateSwapping(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two) {
	// TODO: used only for turning into a special, maybe could be done better
	data->swap1 = one;
	data->swap2 = two;

	TM_AddAction(data->timeline, StartSwapping, TM_Args(GetField(game, data, one), GetField(game, data, two)));
	TM_AddDelay(data->timeline, SWAPPING_TIME * 1000);
	TM_AddAction(data->timeline, AfterSwapping, TM_Args(GetField(game, data, one), GetField(game, data, two)));
}

static TM_ACTION(AfterMatching) {
	TM_RunningOnly;
	DoRemoval(game, data);
	Gravity(game, data);
	ProcessFields(game, data);
	return true;
}

TM_ACTION(DispatchAnimations) {
	TM_RunningOnly;
	PerformActions(game, data);
	TM_AddDelay(data->timeline, (int)((MATCHING_TIME + MATCHING_DELAY_TIME) * 1000));
	TM_AddAction(data->timeline, AfterMatching, NULL);
	return true;
}
