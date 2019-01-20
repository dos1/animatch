/*! \file logic.c
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

struct DandelionParticleData {
	double angle, dangle;
	double scale, dscale;
	ALLEGRO_COLOR color;
	struct GravityParticleData* data;
};

static struct DandelionParticleData* DandelionParticleData(ALLEGRO_COLOR color) {
	struct DandelionParticleData* data = calloc(1, sizeof(struct DandelionParticleData));
	data->data = GravityParticleData((rand() / (double)RAND_MAX - 0.5) / 64.0, (rand() / (double)RAND_MAX - 0.5) / 64.0, 0.000075, 0.000075);
	data->angle = rand() / (double)RAND_MAX * 2 * ALLEGRO_PI;
	data->dangle = (rand() / (double)RAND_MAX - 0.5) * ALLEGRO_PI;
	data->scale = 0.6 + 0.1 * rand() / (double)RAND_MAX;
	data->dscale = (rand() / (double)RAND_MAX - 0.5) * 0.002;
	color = InterpolateColor(color, al_map_rgb(255, 255, 255), 1.0 - rand() / (double)RAND_MAX * 0.3);
	double opacity = 0.9 - rand() / (double)RAND_MAX * 0.1;
	data->color = al_map_rgba_f(color.r * opacity, color.g * opacity, color.b * opacity, color.a * opacity);
	return data;
}

static bool DandelionParticle(struct Game* game, struct ParticleState* particle, double delta, void* d) {
	struct DandelionParticleData* data = d;
	data->angle += data->dangle * delta;
	data->scale += data->dscale * delta;

	if (particle) {
		particle->tint = data->color;
		particle->angle = data->angle;
		particle->scaleX = data->scale;
		particle->scaleY = data->scale;
	}
	bool res = GravityParticle(game, particle, delta, data->data);
	if (!res) {
		free(data);
	}
	return res;
}

bool IsSleeping(struct Field* field) {
	return (field->type == FIELD_TYPE_ANIMAL && field->data.animal.sleeping);
}

inline bool IsDrawable(enum FIELD_TYPE type) {
	return (type == FIELD_TYPE_ANIMAL) || (type == FIELD_TYPE_FREEFALL) || (type == FIELD_TYPE_COLLECTIBLE);
}

static int IsMatching(struct Game* game, struct GamestateResources* data, struct FieldID id) {
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
		for (int i = 0; i < lchain; i++) {
			lfields[i]->match_mark = id.j * COLS + id.i;
		}
	}
	if (tchain >= 2) {
		chain += tchain;
		for (int i = 0; i < tchain; i++) {
			tfields[i]->match_mark = id.j * COLS + id.i;
		}
	}
	if (chain) {
		chain++;
		orig->match_mark = id.j * COLS + id.i;
	}
	//PrintConsole(game, "field %dx%d %s lchain %d tchain %d chain %d", id.i, id.j, ANIMALS[orig->type], lchain, tchain, chain);
	return chain;
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

/*
static bool AreAdjacentMatching(struct Game* game, struct GamestateResources* data, struct FieldID id, struct FieldID (*func)(struct FieldID)) {
	for (int i = 0; i < 3; i++) {
		id = func(id);
		if (!IsValidID(id) || !GetField(game, data, id)->matched) {
			return false;
		}
	}
	return true;
}

static int IsMatchExtension(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	return AreAdjacentMatching(game, data, id, ToTop) || AreAdjacentMatching(game, data, id, ToBottom) || AreAdjacentMatching(game, data, id, ToLeft) || AreAdjacentMatching(game, data, id, ToRight);
}
*/

static int ShouldBeCollected(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	if (GetField(game, data, id)->handled) {
		PrintConsole(game, "Not collecting already handled field %d, %d", id.i, id.j);
		return false;
	}
	return IsMatching(game, data, ToTop(id)) || IsMatching(game, data, ToBottom(id)) || IsMatching(game, data, ToLeft(id)) || IsMatching(game, data, ToRight(id));
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

static bool IsValidMove(struct FieldID one, struct FieldID two) {
	if (one.i == two.i && abs(one.j - two.j) == 1) {
		return true;
	}
	if (one.j == two.j && abs(one.i - two.i) == 1) {
		return true;
	}
	return false;
}

static bool IsSwappable(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	if (!IsValidID(id)) {
		return false;
	}
	struct Field* field = GetField(game, data, id);
	if (((field->type == FIELD_TYPE_ANIMAL) && (!IsSleeping(field))) || (field->type == FIELD_TYPE_COLLECTIBLE)) {
		return true;
	}
	return false;
}

static bool AreSwappable(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two) {
	return IsSwappable(game, data, one) && IsSwappable(game, data, two);
}

void GenerateField(struct Game* game, struct GamestateResources* data, struct Field* field) {
	if (rand() / (float)RAND_MAX < 0.001) {
		field->type = FIELD_TYPE_FREEFALL;
		field->data.freefall.variant = rand() % SPECIAL_ACTIONS[SPECIAL_TYPE_EGG].actions;
	} else if (rand() / (float)RAND_MAX < 0.01) {
		field->type = FIELD_TYPE_COLLECTIBLE;
		field->data.collectible.type = rand() % COLLECTIBLE_TYPES;
		field->data.collectible.variant = 0;
	} else {
		field->type = FIELD_TYPE_ANIMAL;
		field->data.animal.type = rand() % ANIMAL_TYPES;
		if (rand() / (float)RAND_MAX < 0.005) {
			field->data.animal.sleeping = true;
		}
		field->data.animal.super = false;
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

static bool WillMatch(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two) {
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

bool AutoMove(struct Game* game, struct GamestateResources* data) {
	for (int i = 0; i < COLS; i++) {
		for (int j = 0; j < ROWS; j++) {
			struct FieldID id = {.i = i, .j = j};
			struct FieldID (*callbacks[])(struct FieldID) = {ToLeft, ToRight, ToTop, ToBottom};

			for (int q = 0; q < 4; q++) {
				if (IsValidMove(id, callbacks[q](id))) {
					if (WillMatch(game, data, id, callbacks[q](id))) {
						AnimateSwapping(game, data, id, callbacks[q](id));
						return true;
					}
				}
			}
		}
	}
	return false;
}

void Turn(struct Game* game, struct GamestateResources* data) {
	if (!IsValidMove(data->current, data->hovered)) {
		return;
	}

	PrintConsole(game, "swap %dx%d with %dx%d", data->current.i, data->current.j, data->hovered.i, data->hovered.j);
	data->clicked = false;

	if (!AreSwappable(game, data, data->current, data->hovered)) {
		GetField(game, data, data->current)->animation.shaking = Tween(game, 0.0, 1.0, TWEEN_STYLE_SINE_OUT, SHAKING_TIME);
		return;
	}
	data->locked = true;

	AnimateSwapping(game, data, data->current, data->hovered);

	Swap(game, data, data->current, data->hovered);
	if (IsMatching(game, data, data->current) || IsMatching(game, data, data->hovered)) {
		data->moves++;
	} else {
		AnimateSwapping(game, data, data->current, data->hovered);
	}
	Swap(game, data, data->current, data->hovered);
}

static void UpdateOverlay(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	struct Field* field = GetField(game, data, id);
	if (!IsDrawable(field->type)) {
		return;
	}
	char* name = NULL;
	int index = -1;
	if (field->type == FIELD_TYPE_ANIMAL) {
		if (field->data.animal.super) {
			name = "eyes";
			index = SPECIAL_TYPE_EYES;
		}
	}

	if (name) {
		struct Character* archetype = data->special_archetypes[index];
		if (field->overlay->name) {
			free(field->overlay->name);
		}
		field->overlay->name = strdup(archetype->name);
		field->overlay->spritesheets = archetype->spritesheets;

		SelectSpritesheet(game, field->overlay, name);
		field->overlay_visible = true;
	} else {
		field->overlay_visible = false;
	}
}

void UpdateDrawable(struct Game* game, struct GamestateResources* data, struct FieldID id) {
	struct Field* field = GetField(game, data, id);
	if (!IsDrawable(field->type)) {
		return;
	}

	char* name = "stand";
	if (IsSleeping(field)) {
		name = "blink";
	}

	struct Character* archetype = NULL;
	int index = 0;
	if (field->type == FIELD_TYPE_FREEFALL) {
		index = SPECIAL_TYPE_EGG;
		name = SPECIAL_ACTIONS[index].names[field->data.freefall.variant];

		archetype = data->special_archetypes[index];
	} else if (field->type == FIELD_TYPE_COLLECTIBLE) {
		index = FIRST_COLLECTIBLE + field->data.collectible.type;
		int variant = field->data.collectible.variant;
		if (variant == SPECIAL_ACTIONS[index].actions) {
			variant--;
		}
		name = SPECIAL_ACTIONS[index].names[variant];

		archetype = data->special_archetypes[index];
	} else if (field->type == FIELD_TYPE_ANIMAL) {
		index = field->data.animal.type;
		if (field->data.animal.super) {
			index = SPECIAL_TYPE_SUPER;
			name = StrToLower(game, ANIMALS[field->data.animal.type]);
			archetype = data->special_archetypes[index];
		} else {
			archetype = data->animal_archetypes[index];
		}
	} else {
		PrintConsole(game, "Incorrect drawable!");
		return;
	}

	if (field->drawable->name) {
		free(field->drawable->name);
	}
	field->drawable->name = strdup(archetype->name);
	field->drawable->spritesheets = archetype->spritesheets;

	SelectSpritesheet(game, field->drawable, name);

	UpdateOverlay(game, data, id);
}

void SpawnParticles(struct Game* game, struct GamestateResources* data, struct FieldID id, int num) {
	struct Field* field = GetField(game, data, id);
	ALLEGRO_COLOR color = al_map_rgb(255, 255, 255);
	if (field->type == FIELD_TYPE_ANIMAL) {
		color = ANIMAL_COLORS[field->data.animal.type];
	}
	for (int p = 0; p < num; p++) {
		data->special_archetypes[SPECIAL_TYPE_DANDELION]->pos = rand() % data->special_archetypes[SPECIAL_TYPE_DANDELION]->spritesheet->frame_count;
		if (rand() % 2) {
			data->special_archetypes[SPECIAL_TYPE_DANDELION]->pos = 0;
		}
		float x = GetCharacterX(game, field->drawable) / (double)game->viewport.width, y = GetCharacterY(game, field->drawable) / (double)game->viewport.height;
		EmitParticle(game, data->particles, data->special_archetypes[SPECIAL_TYPE_DANDELION], FaderParticle, SpawnParticleBetween(x - 0.01, y - 0.01, x + 0.01, y + 0.01), FaderParticleData(1.0, 0.025, DandelionParticle, DandelionParticleData(color)));
	}
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

TM_ACTION(DoSpawnParticles) {
	TM_RunningOnly;
	struct Field* field = TM_Arg(0);
	int* count = TM_Arg(1);
	SpawnParticles(game, data, field->id, *count);
	free(count);
	return true;
}
