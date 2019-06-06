/*! \file levels.c
 *  \brief Handling of levels.
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

void LoadLevel(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < ANIMAL_TYPES; i++) {
		data->level.animals[i] = true;
	}
	for (int i = 0; i < SPECIAL_TYPES; i++) {
		data->level.specials[i] = true;
	}
	data->level.sleeping = true;
	data->level.supers = true;
	data->level.specials[SPECIAL_TYPE_EGG] = false;
	data->level.infinite = true;
	data->level.goals[0].type = GOAL_TYPE_NONE;
	data->level.goals[1].type = GOAL_TYPE_NONE;
	data->level.goals[2].type = GOAL_TYPE_NONE;

	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->fields[i][j].animation.hiding = StaticTween(game, 0.0);
			data->fields[i][j].animation.falling = StaticTween(game, 1.0);

			data->fields[i][j].animation.time_to_action = (int)((rand() % 250000 + 500000) * (rand() / (double)RAND_MAX));
			data->fields[i][j].animation.time_to_blink = (int)((rand() % 100000 + 200000) * (rand() / (double)RAND_MAX));
		}
	}

	// temporary
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->level.fields[i][j].field_type = FIELD_TYPE_ANIMAL;
			data->level.fields[i][j].animal_type = (j * COLS + i) % ANIMAL_TYPES;
			data->level.fields[i][j].random_animal = true;
			data->level.fields[i][j].sleeping = false;
			data->level.fields[i][j].super = false;
		}
	}

	PrintConsole(game, "level: %d", game->data->level);
	if (game->data->level >= 0) {
		data->level.id = game->data->level;

		data->goals[0] = data->level.goals[0];
		data->goals[1] = data->level.goals[1];
		data->goals[2] = data->level.goals[2];
		data->infinite = data->level.infinite;
		data->moves_goal = data->level.moves;

		for (int i = 0; i < COLS; i++) {
			for (int j = 0; j < ROWS; j++) {
				if (data->level.fields[i][j].field_type == FIELD_TYPE_EMPTY) {
					GenerateField(game, data, &data->fields[i][j], false);
				} else {
					data->fields[i][j].type = data->level.fields[i][j].field_type;

					switch (data->level.fields[i][j].field_type) {
						case FIELD_TYPE_COLLECTIBLE:
							data->fields[i][j].data.collectible.type = data->level.fields[i][j].collectible_type;
							break;
						case FIELD_TYPE_FREEFALL:
							data->fields[i][j].data.freefall.variant = rand() % SPECIAL_ACTIONS[SPECIAL_TYPE_EGG].actions;
							break;
						case FIELD_TYPE_ANIMAL:
							if (data->level.fields[i][j].random_animal) {
								GenerateAnimal(game, data, &data->fields[i][j], false);
							} else {
								data->fields[i][j].data.animal.type = data->level.fields[i][j].animal_type;
							}
							data->fields[i][j].data.animal.sleeping = data->level.fields[i][j].sleeping;
							data->fields[i][j].data.animal.super = data->level.fields[i][j].super;
							break;
						default:
							break;
					}
					UpdateDrawable(game, data, data->fields[i][j].id);
				}
			}
		}
		data->moves = 0;
		data->score = 0;
	}

	data->current = (struct FieldID){-1, -1};
	do {
		DoRemoval(game, data);
		Gravity(game, data);
	} while (MarkMatching(game, data));
	StopAnimations(game, data);
}

void RestartLevel(struct Game* game, struct GamestateResources* data) {
	LoadLevel(game, data);
	data->failed = false;
	data->failing = StaticTween(game, 0.0);
	data->locked = false;
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			SpawnParticles(game, data, (struct FieldID){i, j}, 16);
		}
	}
}

void FinishLevel(struct Game* game, struct GamestateResources* data) {
	data->done = true;
	data->finishing = Tween(game, 0.0, 1.0, TWEEN_STYLE_BOUNCE_OUT, 1.0);
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			SpawnParticles(game, data, (struct FieldID){i, j}, 16);
		}
	}
	UnlockLevel(game, data->level.id + 1);
}

void FailLevel(struct Game* game, struct GamestateResources* data) {
	data->failed = true;
	data->failing = Tween(game, 0.0, 1.0, TWEEN_STYLE_BOUNCE_OUT, 1.0);
}
