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

#define LIBSUPERDERPY_DATA_TYPE struct CommonResources
#include <defines.h>
#include <libsuperderpy.h>

#define STRINGIFY(a) #a
#if defined(__clang__) || defined(__codemodel__)
#define SUPPRESS_WARNING(x) _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wpragmas\"") _Pragma(STRINGIFY(clang diagnostic ignored x))
#define SUPPRESS_END _Pragma("clang diagnostic pop")
#elif defined(__GNUC__) && !defined(MAEMO5)
#define SUPPRESS_WARNING(x) _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wpragmas\"") _Pragma(STRINGIFY(GCC diagnostic ignored x))
#define SUPPRESS_END _Pragma("GCC diagnostic pop")
#else
#define SUPPRESS_WARNING(x)
#define SUPPRESS_END
#endif

enum UI_ELEMENT {
	UI_ELEMENT_HOME,
	UI_ELEMENT_FX,
	UI_ELEMENT_FX2,
	UI_ELEMENT_NOTE,
	UI_ELEMENT_NOTE2,
	UI_ELEMENT_NOSOUND,
	UI_ELEMENT_HINT,
	UI_ELEMENT_BALOON_BIG,
	UI_ELEMENT_BALOON_MINI,
	UI_ELEMENT_BALOON_SMALL,
	UI_ELEMENT_BALOON_MEDIUM,
	UI_ELEMENT_ABOUT,
	UI_ELEMENT_SETTINGS,
	UI_ELEMENT_FLOWER,
	UI_ELEMENT_WAND,
	UI_ELEMENT_SKILL1_ACTIVE,
	UI_ELEMENT_SKILL1_INACTIVE,
	UI_ELEMENT_SKILL2_ACTIVE,
	UI_ELEMENT_SKILL2_INACTIVE,
	UI_ELEMENT_SKILL3_ACTIVE,
	UI_ELEMENT_SKILL3_INACTIVE,
	UI_ELEMENT_SWAP,
	UI_ELEMENT_SCORE
};

struct CommonResources {
	// Fill in with common data accessible from all gamestates.
	double mouseX, mouseY;
	bool touch;
	float loading_fade;
	ALLEGRO_BITMAP* silhouette;
	ALLEGRO_SHADER* kawese_shader;
};

void Compositor(struct Game* game, struct Gamestate* gamestates, ALLEGRO_BITMAP* loading_fb);
void PostLogic(struct Game* game, double delta);
struct CommonResources* CreateGameData(struct Game* game);
void DestroyGameData(struct Game* game);
bool GlobalEventHandler(struct Game* game, ALLEGRO_EVENT* ev);
void DrawBuildInfo(struct Game* game);
void DrawUIElement(struct Game* game, struct Character* ui, enum UI_ELEMENT element);
bool IsOnUIElement(struct Game* game, struct Character* ui, enum UI_ELEMENT element, float x, float y);
