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
 *  - StartSwapping gets called, putting AnimateSwapping and TriggerProcessing to the timeline queue
 *
 *  - AnimateSwapping locks the controls, waits for animation and finishes with the actual swap in the memory
 *  - TriggerProcessing simply calls ProcessFields
 *
 *  - ProcessFields is where the actual magic happens:
 *    - MarkMatching finds matches and sets proper field attributes
 *    - Collect gets called, dealing with collectibles
 *    - AnimateSpecials triggers and queues animation for the special fields
 *    - if necessary, Collect and AnimateSpecials are repeated as long as there's something to do for them
 *    - now:
 *      - if something happened earlier, DispatchAnimations is queued after any previously queued animation
 *      - if nothing happened and there are no possible moves left, HandleDeadlock is called which is supposed
 *        to shuffle the board in order to get out of deadlock; DispatchAnimations is then queued
 *      - otherwise, the controls are unlocked, goals/turns checked and the execution flow stops there.
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

void UpdateGoal(struct Game* game, struct GamestateResources* data, enum GOAL_TYPE type, int val) {
	if (data->goal_lock) {
		return;
	}
	for (int i = 0; i < 3; i++) {
		if (data->goals[i].type == type) {
			data->goals[i].value -= val;
		}
	}
}

void AddScore(struct Game* game, struct GamestateResources* data, int val) {
	data->score += val;
	data->scoring = Tween(game, 1.0, 0.0, TWEEN_STYLE_SINE_OUT, 1.0);
	UpdateGoal(game, data, GOAL_TYPE_SCORE, val);
}

static bool CheckGoals(struct Game* game, struct GamestateResources* data) {
	if (data->infinite) {
		return false;
	}
	for (int i = 0; i < 3; i++) {
		if (data->goals[i].type != GOAL_TYPE_NONE) {
			if (data->goals[i].value > 0) {
				return false;
			}
		}
	}
	return true;
}

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
					AddScore(game, data, 100);
					collected++;
					UpdateGoal(game, data, GOAL_TYPE_FREEFALL, 1);
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
					AddScore(game, data, 10);
					UpdateGoal(game, data, GOAL_TYPE_SLEEPING, 1);
				} else if (data->fields[i][j].type == FIELD_TYPE_COLLECTIBLE) {
					data->fields[i][j].data.collectible.variant++;

					if (data->fields[i][j].data.collectible.variant >= SPECIAL_ACTIONS[FIRST_COLLECTIBLE + data->fields[i][j].data.collectible.type].actions) {
						data->fields[i][j].data.collectible.variant = SPECIAL_ACTIONS[FIRST_COLLECTIBLE + data->fields[i][j].data.collectible.type].actions - 1;
						data->fields[i][j].to_remove = true;
						PrintConsole(game, "collecting field %d, %d", i, j);
						AddScore(game, data, 50);
					} else {
						data->fields[i][j].to_remove = false;
						PrintConsole(game, "advancing field %d, %d", i, j);
						AddScore(game, data, 20);
						UpdateGoal(game, data, GOAL_TYPE_COLLECTIBLE, 1);
						UpdateGoal(game, data, GOAL_TYPE_COLLECTIBLE + 1 + data->fields[i][j].data.collectible.type, 1);
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

void GenerateAnimal(struct Game* game, struct GamestateResources* data, struct Field* field, bool allow_matches) {
	field->type = FIELD_TYPE_ANIMAL;
	while (data->level.field_types[FIELD_TYPE_ANIMAL]) {
		field->data.animal.type = rand() % ANIMAL_TYPES;
		if (!allow_matches && IsMatching(game, data, field->id)) {
			continue;
		}
		if (data->level.animals[field->data.animal.type]) {
			break;
		}
	}
	if (rand() / (float)RAND_MAX < 0.005) {
		field->data.animal.sleeping = data->level.sleeping;
	}
	field->data.animal.super = false;

	field->overlay_visible = false;
	field->locked = true;
	UpdateDrawable(game, data, field->id);
}

void GenerateField(struct Game* game, struct GamestateResources* data, struct Field* field, bool allow_matches) {
	while (true) {
		if (rand() / (float)RAND_MAX < 0.001) {
			field->type = FIELD_TYPE_FREEFALL;
			field->data.freefall.variant = rand() % SPECIAL_ACTIONS[SPECIAL_TYPE_EGG].actions;
			if (data->level.fields[FIELD_TYPE_FREEFALL]) {
				break;
			}
		} else if (rand() / (float)RAND_MAX < 0.01) {
			field->type = FIELD_TYPE_COLLECTIBLE;
			while (data->level.field_types[FIELD_TYPE_COLLECTIBLE]) {
				field->data.collectible.type = rand() % COLLECTIBLE_TYPES;
				if (data->level.collectibles[field->data.collectible.type]) {
					break;
				}
			}
			field->data.collectible.variant = 0;
			if (data->level.field_types[FIELD_TYPE_COLLECTIBLE]) {
				break;
			}
		} else {
			GenerateAnimal(game, data, field, allow_matches);
			if (data->level.field_types[FIELD_TYPE_ANIMAL]) {
				break;
			}
		}
	}
	field->overlay_visible = false;
	field->locked = true;
	UpdateDrawable(game, data, field->id);
}

static void CreateNewField(struct Game* game, struct GamestateResources* data, struct Field* field) {
	GenerateField(game, data, field, true);
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
}

static void HandleDeadlock(struct Game* game, struct GamestateResources* data) {
	// TODO: randomly swapping all animals around is probably a better idea?
	int J = ROWS / 2;
	for (int i = 0; i < COLS; i++) {
		for (int j = -1; j <= 1; j++) {
			if (data->fields[i][J + j].type == FIELD_TYPE_ANIMAL) {
				data->fields[i][J + j].to_remove = true;
			}
		}
	}
}

void ProcessFields(struct Game* game, struct GamestateResources* data) {
	bool matched = MarkMatching(game, data);
	bool collected = Collect(game, data);
	if (matched || collected) {
		while (AnimateSpecials(game, data)) {
			Collect(game, data);
		}
		TM_AddAction(data->timeline, DispatchAnimations, NULL);
	} else {
		// deadlock handling
		int moves = CountMoves(game, data);
		PrintConsole(game, "possible moves: %d", moves);
		if (moves == 0) {
			HandleDeadlock(game, data);
			TM_AddAction(data->timeline, DispatchAnimations, NULL);
		} else {
			data->locked = false;
			if (!data->goal_lock) {
				if (CheckGoals(game, data)) {
					FinishLevel(game, data);
				} else if (!data->infinite && data->moves == data->moves_goal) {
					FailLevel(game, data);
				}
			}
			data->goal_lock = false;
		}
	}
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
					UpdateGoal(game, data, GOAL_TYPE_ANIMAL, 1);
					UpdateGoal(game, data, GOAL_TYPE_ANIMAL + 1 + data->fields[i][j].data.animal.type, 1);

					if (data->fields[i][j].matched >= 4 && data->fields[i][j].match_mark) {
						TurnMatchToSuper(game, data, data->fields[i][j].matched, data->fields[i][j].match_mark);
					}
				}

				AddScore(game, data, 10);

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

static TM_ACTION(TriggerProcessing) {
	TM_RunningOnly;
	ProcessFields(game, data);
	return true;
}

static TM_ACTION(AnimateSwapping) {
	switch (action->state) {
		case TM_ACTIONSTATE_START: {
			struct Field* one = TM_GetArg(action->arguments, 0);
			struct Field* two = TM_GetArg(action->arguments, 1);
			double* timeout = TM_GetArg(action->arguments, 2);
			data->locked = true;
			one->animation.swapping = Tween(game, 0.0, 1.0, TWEEN_STYLE_SINE_IN_OUT, *timeout);
			one->animation.swapee = two->id;
			two->animation.swapping = Tween(game, 0.0, 1.0, TWEEN_STYLE_SINE_IN_OUT, *timeout);
			two->animation.swapee = one->id;
			return false;
		}
		case TM_ACTIONSTATE_RUNNING: {
			double* timeout = TM_GetArg(action->arguments, 2);
			double t = *timeout;
			*timeout -= action->delta;
			if (*timeout < 0) {
				action->delta -= t;
				return true;
			}
			return false;
		}
		case TM_ACTIONSTATE_STOP: {
			struct Field* one = TM_GetArg(action->arguments, 0);
			struct Field* two = TM_GetArg(action->arguments, 1);
			Swap(game, data, one->id, two->id);
			one->animation.swapping = StaticTween(game, 0.0);
			two->animation.swapping = StaticTween(game, 0.0);
			return true;
		}
		case TM_ACTIONSTATE_DESTROY:
			free(TM_GetArg(action->arguments, 2));
		default:
			return true;
	}
}

void StartSwapping(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two) {
	// TODO: used only for turning into a special, maybe could be done better
	data->swap1 = one;
	data->swap2 = two;

	TM_WrapArg(double, duration, SWAPPING_TIME);
	TM_AddAction(data->timeline, AnimateSwapping, TM_Args(GetField(game, data, one), GetField(game, data, two), duration));
	TM_AddAction(data->timeline, TriggerProcessing, NULL);
}

void StartBadSwapping(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two) {
	{
		TM_WrapArg(double, duration, SWAPPING_TIME);
		TM_AddAction(data->timeline, AnimateSwapping, TM_Args(GetField(game, data, one), GetField(game, data, two), duration));
	}
	{
		// go back
		TM_WrapArg(double, duration, SWAPPING_TIME);
		TM_AddAction(data->timeline, AnimateSwapping, TM_Args(GetField(game, data, one), GetField(game, data, two), duration));
	}
	TM_AddAction(data->timeline, TriggerProcessing, NULL);
}

static TM_ACTION(AfterMatching) {
	TM_RunningOnly;
	assert(data); // silence clang-analyzer
	DoRemoval(game, data);
	Gravity(game, data);
	ProcessFields(game, data);
	return true;
}

TM_ACTION(DispatchAnimations) {
	TM_RunningOnly;
	PerformActions(game, data);
	TM_AddDelay(data->timeline, MATCHING_TIME + MATCHING_DELAY_TIME);
	TM_AddAction(data->timeline, AfterMatching, NULL);
	return true;
}
