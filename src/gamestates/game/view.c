/*! \file view.c
 *  \brief Visual effects and graphical representation
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

struct DandelionParticleData* DandelionParticleData(ALLEGRO_COLOR color) {
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

bool DandelionParticle(struct Game* game, struct ParticleState* particle, double delta, void* d) {
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

TM_ACTION(DoSpawnParticles) {
	TM_RunningOnly;
	struct Field* field = TM_Arg(0);
	int* count = TM_Arg(1);
	SpawnParticles(game, data, field->id, *count);
	free(count);
	return true;
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
