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

void LoadLevel(struct Game* game, struct GamestateResources* data, int id) {
	if (id == 0) {
		// infinite level
		for (int i = 0; i < FIELD_TYPES; i++) {
			data->level.field_types[i] = true;
		}
		for (int i = 0; i < ANIMAL_TYPES; i++) {
			data->level.animals[i] = true;
		}
		for (int i = 0; i < COLLECTIBLE_TYPES; i++) {
			data->level.collectibles[i] = true;
		}
		for (int i = 0; i < GOAL_TYPES; i++) {
			data->level.requirements[i] = 0;
		}
		data->level.sleeping = true;
		data->level.supers = true;
		data->level.field_types[FIELD_TYPE_FREEFALL] = false;
		data->level.infinite = true;
		data->level.goals[0].type = GOAL_TYPE_NONE;
		data->level.goals[1].type = GOAL_TYPE_NONE;
		data->level.goals[2].type = GOAL_TYPE_NONE;

		for (int i = 0; i < COLS; i++) {
			for (int j = 0; j < ROWS; j++) {
				data->level.fields[i][j].field_type = FIELD_TYPE_ANIMAL;
				data->level.fields[i][j].animal_type = (j * COLS + i) % ANIMAL_TYPES;
				data->level.fields[i][j].random_subtype = true;
				data->level.fields[i][j].sleeping = false;
				data->level.fields[i][j].super = false;
				data->level.fields[i][j].variant = 0;
			}
		}
		data->level.id = id;
		data->level.infinite = true;
		return;
	}

	char* name = malloc(255 * sizeof(char));
	snprintf(name, 255, "%d.lvl", id);

	ALLEGRO_FILE* file;

	ALLEGRO_PATH* path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
	ALLEGRO_PATH* p = al_create_path(name);
	al_join_paths(path, p);
	const char* filename = al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP);

	if (!al_filename_exists(filename)) {
		snprintf(name, 255, "levels/%d.lvl", id);
		const char* filename2 = FindDataFilePath(game, name);
		if (!filename2) {
			FatalError(game, false, "Could not find level data file: %s", name);
			al_destroy_path(p);
			al_destroy_path(path);
			free(name);
			return;
		}
		file = al_fopen(filename2, "rb");
		filename = filename2;
	} else {
		file = al_fopen(filename, "rb");
	}

	if (!file) {
		FatalError(game, false, "Could not open level data file: %s", filename);
		al_destroy_path(p);
		al_destroy_path(path);
		free(name);
		return;
	}

	char buf[14];
	al_fread(file, buf, 14);
	if (strncmp("ANIMATCH_LEVEL", buf, 14) != 0) {
		FatalError(game, false, "Incorrect level data: %s", filename);
		goto err;
	}

	int version = al_fread32le(file);
	if (version > 1) {
		FatalError(game, false, "Incompatible version (%d) in level data: %s", version, filename);
		goto err;
	}

	data->level.moves = al_fread16le(file);

	int val = al_fread16le(file);
	if (val != 0) {
		FatalError(game, false, "Invalid number of score thresholds (%d) in level data: %s", val, filename);
		goto err;
	}

	val = al_fread16le(file); // nr of goal groups
	if (val > 1) {
		FatalError(game, false, "Too many goal groups (%d) in level data: %s", val, filename);
		goto err;
	}

	{
		val = al_fread16le(file); // goal group type (AND)
		if (val != 0) {
			FatalError(game, false, "Invalid goal group type (%d) in level data: %s", val, filename);
			goto err;
		}

		val = al_fread16le(file); // nr of goals

		if (val > 3) {
			FatalError(game, false, "Invalid number of goals (%d) in level data: %s", val, filename);
			goto err;
		}

		for (int i = 0; i < 3; i++) {
			data->level.goals[i].type = GOAL_TYPE_NONE;
		}
		for (int i = 0; i < val; i++) {
			data->level.goals[i].type = al_fread16le(file);
			data->level.goals[i].value = al_fread16le(file);
		}
	}

	val = al_fread16le(file);
	if (val != FIELD_TYPES) {
		FatalError(game, false, "Invalid number of field types (%d) in level data: %s", val, filename);
		goto err;
	}

	for (int i = 0; i < FIELD_TYPES; i++) {
		data->level.field_types[i] = al_fread16le(file);
	}

	val = al_fread16le(file);
	if (val != ANIMAL_TYPES) {
		FatalError(game, false, "Invalid number of animal types (%d) in level data: %s", val, filename);
		goto err;
	}

	for (int i = 0; i < ANIMAL_TYPES; i++) {
		data->level.animals[i] = al_fread16le(file);
	}

	val = al_fread16le(file);
	if (val != COLLECTIBLE_TYPES) {
		FatalError(game, false, "Invalid number of special types (%d) in level data: %s", val, filename);
		goto err;
	}
	for (int i = 0; i < COLLECTIBLE_TYPES; i++) {
		data->level.collectibles[i] = al_fread16le(file);
	}

	val = al_fread16le(file);
	if (val != GOAL_TYPES && val != 0) {
		FatalError(game, false, "Invalid number of spawn requirements (%d) in level data: %s", val, filename);
		goto err;
	}
	for (int i = 0; i < val; i++) {
		data->level.requirements[i] = al_fread16le(file);
	}

	val = al_fread16le(file);
	if (val != 2) {
		FatalError(game, false, "Invalid number of config options (%d) in level data: %s", val, filename);
		goto err;
	}
	data->level.supers = al_fread16le(file);
	data->level.sleeping = al_fread16le(file);

	val = al_fread16le(file);
	if (val != ROWS) {
		FatalError(game, false, "Invalid number of rows (%d) in level data: %s", val, filename);
		goto err;
	}
	val = al_fread16le(file);
	if (val != COLS) {
		FatalError(game, false, "Invalid number of cols (%d) in level data: %s", val, filename);
		goto err;
	}

	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->level.fields[i][j].field_type = al_fread16le(file);

			switch (data->level.fields[i][j].field_type) {
				case FIELD_TYPE_ANIMAL:
					data->level.fields[i][j].animal_type = al_fread16le(file);
					break;
				case FIELD_TYPE_COLLECTIBLE:
					data->level.fields[i][j].collectible_type = al_fread16le(file);
					break;
				default:
					al_fread16le(file);
			}
			data->level.fields[i][j].random_subtype = al_fread16le(file); // random subtype
			data->level.fields[i][j].variant = 0;
			if (version >= 1) {
				data->level.fields[i][j].variant = al_fread16le(file);
			}
			data->level.fields[i][j].sleeping = al_fread16le(file);
			data->level.fields[i][j].super = al_fread16le(file);
		}
	}

	data->level.id = id;
	data->level.infinite = false;

err:
	al_destroy_path(p);
	al_destroy_path(path);
	free(name);
	al_fclose(file);
}

void CopyLevel(struct Game* game, struct GamestateResources* data) {
	data->level.goals[0] = data->goals[0];
	data->level.goals[1] = data->goals[1];
	data->level.goals[2] = data->goals[2];
	data->level.infinite = data->infinite;
	data->level.moves = data->moves_goal;
	for (int i = 0; i < GOAL_TYPES; i++) {
		data->level.requirements[i] = data->requirements[i];
	}

	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->level.fields[i][j].field_type = data->fields[i][j].type;

			switch (data->level.fields[i][j].field_type) {
				case FIELD_TYPE_COLLECTIBLE:
					data->level.fields[i][j].collectible_type = data->fields[i][j].data.collectible.type;
					break;
				case FIELD_TYPE_ANIMAL:
					data->level.fields[i][j].random_subtype = false;
					data->level.fields[i][j].animal_type = data->fields[i][j].data.animal.type;
					data->level.fields[i][j].sleeping = data->fields[i][j].data.animal.sleeping;
					data->level.fields[i][j].super = data->fields[i][j].data.animal.super;
					break;
				default:
					break;
			}
		}
	}
}

void StartLevel(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			if (data->level.fields[i][j].field_type == FIELD_TYPE_EMPTY) {
				GenerateField(game, data, &data->fields[i][j], false);
			}
		}
	}
	do {
		DoRemoval(game, data);
		Gravity(game, data);
	} while (MarkMatching(game, data));
	StopAnimations(game, data);
}

void ApplyLevel(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			data->fields[i][j].animation.hiding = StaticTween(game, 0.0);
			data->fields[i][j].animation.falling = StaticTween(game, 1.0);

			data->fields[i][j].animation.time_to_action = (int)((rand() % 250000 + 500000) * (rand() / (double)RAND_MAX));
			data->fields[i][j].animation.time_to_blink = (int)((rand() % 100000 + 200000) * (rand() / (double)RAND_MAX));
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
		for (int i = 0; i < GOAL_TYPES; i++) {
			data->requirements[i] = data->level.requirements[i];
		}

		for (int i = 0; i < COLS; i++) {
			for (int j = 0; j < ROWS; j++) {
				data->fields[i][j].type = data->level.fields[i][j].field_type;

				switch (data->level.fields[i][j].field_type) {
					case FIELD_TYPE_COLLECTIBLE:
						data->fields[i][j].data.collectible.type = data->level.fields[i][j].collectible_type;
						data->fields[i][j].data.collectible.variant = data->level.fields[i][j].variant;
						break;
					case FIELD_TYPE_FREEFALL:
						data->fields[i][j].data.freefall.variant = data->level.fields[i][j].variant;
						break;
					case FIELD_TYPE_ANIMAL:
						if (data->level.fields[i][j].random_subtype) {
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
		data->moves = 0;
		data->score = 0;
		data->failing = StaticTween(game, 0.0);
		data->failed = false;
		data->finishing = StaticTween(game, 0.0);
		data->done = false;
	}

	data->current = (struct FieldID){-1, -1};
}

void RestartLevel(struct Game* game, struct GamestateResources* data) {
	ApplyLevel(game, data);
	StartLevel(game, data);
	data->failed = false;
	data->failing = StaticTween(game, 0.0);
	data->locked = false;
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			SpawnParticles(game, data, (struct FieldID){i, j}, 16);
		}
	}
}

void SanityCheckLevel(struct Game* game, struct GamestateResources* data) {
	// avoid endless loops from impossible level configurations
	bool used = false;
	for (enum FIELD_TYPE i = 0; i < FIELD_TYPES; i++) {
		if (data->level.field_types[i]) {
			used = true;
		}
	}
	if (!used) {
		data->level.field_types[0] = true;
	}
	int count = 0;
	for (enum ANIMAL_TYPE i = 0; i < ANIMAL_TYPES; i++) {
		if (data->level.animals[i]) {
			count++;
		}
	}
	if (data->level.field_types[FIELD_TYPE_ANIMAL] && count < 3) {
		int i = 0;
		while (count < 3) {
			if (!data->level.animals[i]) {
				data->level.animals[i] = true;
				count++;
			}
			i++;
		}
	}
	used = false;
	for (enum COLLECTIBLE_TYPE i = 0; i < COLLECTIBLE_TYPES; i++) {
		if (data->level.collectibles[i]) {
			used = true;
		}
	}
	if (!used && data->level.field_types[FIELD_TYPE_COLLECTIBLE]) {
		data->level.collectibles[0] = true;
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
	game->data->in_progress = false;
}

void FailLevel(struct Game* game, struct GamestateResources* data) {
	data->failed = true;
	data->failing = Tween(game, 0.0, 1.0, TWEEN_STYLE_BOUNCE_OUT, 1.0);
}

void StoreLevel(struct Game* game, struct GamestateResources* data) {
	char* name = malloc(255 * sizeof(char));
	snprintf(name, 255, "%d.lvl", data->level.id);
	ALLEGRO_PATH* path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
	if (!al_filename_exists(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP))) {
		al_make_directory(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP));
	}
	ALLEGRO_PATH* p = al_create_path(name);
	al_join_paths(path, p);
	const char* filename = al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP);
	// TODO: set a proper writable path
	ALLEGRO_FILE* file = al_fopen(filename, "wb");

	if (!file) {
		FatalError(game, false, "Could not open level data file for writing: %s", filename);
	}

	free(name);
	al_destroy_path(p);
	al_destroy_path(path);

	if (!file) { return; }

	al_fwrite(file, "ANIMATCH_LEVEL", 14);
	al_fwrite32le(file, 1); // file version

	al_fwrite16le(file, data->level.moves);

	al_fwrite16le(file, 0); // nr of score thresholds

	al_fwrite16le(file, 1); // nr of goal groups
	{
		al_fwrite16le(file, 0); // goal group type (AND)
		al_fwrite16le(file, 3); // nr of goals

		for (int i = 0; i < 3; i++) {
			al_fwrite16le(file, data->level.goals[i].type);
			al_fwrite16le(file, data->level.goals[i].value);
		}
	}

	al_fwrite16le(file, FIELD_TYPES);
	for (int i = 0; i < FIELD_TYPES; i++) {
		al_fwrite16le(file, data->level.field_types[i]);
	}

	al_fwrite16le(file, ANIMAL_TYPES);
	for (int i = 0; i < ANIMAL_TYPES; i++) {
		al_fwrite16le(file, data->level.animals[i]);
	}

	al_fwrite16le(file, COLLECTIBLE_TYPES);
	for (int i = 0; i < COLLECTIBLE_TYPES; i++) {
		al_fwrite16le(file, data->level.collectibles[i]);
	}

	al_fwrite16le(file, GOAL_TYPES); // nr of spawn requirements options
	for (int i = 0; i < GOAL_TYPES; i++) {
		al_fwrite16le(file, data->level.requirements[i]);
	}

	al_fwrite16le(file, 2); // nr of config options
	al_fwrite16le(file, data->level.supers);
	al_fwrite16le(file, data->level.sleeping);

	al_fwrite16le(file, ROWS);
	al_fwrite16le(file, COLS);
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			al_fwrite16le(file, data->level.fields[i][j].field_type);
			switch (data->level.fields[i][j].field_type) {
				case FIELD_TYPE_ANIMAL:
					al_fwrite16le(file, data->level.fields[i][j].animal_type);
					break;
				case FIELD_TYPE_COLLECTIBLE:
					al_fwrite16le(file, data->level.fields[i][j].collectible_type);
					break;
				default:
					al_fwrite16le(file, -1);
			}
			al_fwrite16le(file, 0); // random subtype
			al_fwrite16le(file, data->level.fields[i][j].variant);
			al_fwrite16le(file, data->level.fields[i][j].sleeping);
			al_fwrite16le(file, data->level.fields[i][j].super);
		}
	}

	al_fclose(file);
}
