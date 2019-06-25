#ifndef ANIMATCH_GAMESTATE_GAME_H
#define ANIMATCH_GAMESTATE_GAME_H

#include "../../common.h"

#define MAX_ACTIONS 16
#define MATCHING_TIME 0.4
#define MATCHING_DELAY_TIME 0.2
#define FALLING_TIME 0.5
#define SWAPPING_TIME 0.15
#define SHAKING_TIME 0.5
#define HINT_TIME 1.0
#define LAUNCHING_TIME 1.5
#define COLLECTING_TIME 0.6

#define BLUR_DIVIDER 8

#define COLS 8
#define ROWS 8

#define FOREACH_ANIMAL(ANIMAL) \
	ANIMAL(BEE)                  \
	ANIMAL(BIRD)                 \
	ANIMAL(CAT)                  \
	ANIMAL(FISH)                 \
	ANIMAL(FROG)                 \
	ANIMAL(LADYBUG)

#define FOREACH_COLLECTIBLE(COLLECTIBLE) \
	COLLECTIBLE(BERRY)                     \
	COLLECTIBLE(STRAWBERRY)                \
	COLLECTIBLE(MUSHROOM)                  \
	COLLECTIBLE(CONE)                      \
	COLLECTIBLE(APPLE)                     \
	COLLECTIBLE(CHESTNUT)

#define FOREACH_SPECIAL(SPECIAL) \
	SPECIAL(EGG)                   \
	FOREACH_COLLECTIBLE(SPECIAL)   \
	SPECIAL(SUPER)                 \
	SPECIAL(EYES)                  \
	SPECIAL(DANDELION)             \
	SPECIAL(CLOUD)

#define FOREACH_FIELD(FIELD) \
	FIELD(ANIMAL)              \
	FIELD(FREEFALL)            \
	FIELD(COLLECTIBLE)         \
	FIELD(EMPTY)               \
	FIELD(DISABLED)

#define FOREACH_GOAL(GOAL)  \
	GOAL(NONE)                \
	GOAL(SCORE)               \
	GOAL(FREEFALL)            \
	GOAL(ANIMAL)              \
	FOREACH_ANIMAL(GOAL)      \
	GOAL(COLLECTIBLE)         \
	FOREACH_COLLECTIBLE(GOAL) \
	GOAL(SLEEPING)            \
	GOAL(SUPER)

#define GENERATE_STRING(VAL) #VAL,
#define GENERATE_ANIMAL_ENUM(VAL) ANIMAL_TYPE_##VAL,
#define GENERATE_SPECIAL_ENUM(VAL) SPECIAL_TYPE_##VAL,
#define GENERATE_COLLECTIBLE_ENUM(VAL) COLLECTIBLE_TYPE_##VAL,
#define GENERATE_FIELD_ENUM(VAL) FIELD_TYPE_##VAL,
#define GENERATE_GOAL_ENUM(VAL) GOAL_TYPE_##VAL,

enum ANIMAL_TYPE {
	FOREACH_ANIMAL(GENERATE_ANIMAL_ENUM)
	//
	ANIMAL_TYPES
};

enum SPECIAL_TYPE {
	FOREACH_SPECIAL(GENERATE_SPECIAL_ENUM)
	//
	SPECIAL_TYPES
};

enum COLLECTIBLE_TYPE {
	FOREACH_COLLECTIBLE(GENERATE_COLLECTIBLE_ENUM)
	//
	COLLECTIBLE_TYPES
};

enum FIELD_TYPE {
	FOREACH_FIELD(GENERATE_FIELD_ENUM)
	//
	FIELD_TYPES
};

enum GOAL_TYPE {
	FOREACH_GOAL(GENERATE_GOAL_ENUM)
	//
	GOAL_TYPES
};

static char* ANIMALS[] = {FOREACH_ANIMAL(GENERATE_STRING)};
static char* SPECIALS[] = {FOREACH_SPECIAL(GENERATE_STRING)};

#define FIRST_COLLECTIBLE SPECIAL_TYPE_BERRY

SUPPRESS_WARNING("-Wunused-variable")

static ALLEGRO_COLOR ANIMAL_COLORS[] = {
	{.r = 0.937, .g = 0.729, .b = 0.353, .a = 1.0}, // bee
	{.r = 0.694, .g = 0.639, .b = 0.894, .a = 1.0}, // bird
	{.r = 0.8, .g = 0.396, .b = 0.212, .a = 1.0}, // cat
	{.r = 0.251, .g = 0.529, .b = 0.91, .a = 1.0}, // fish
	{.r = 0.584, .g = 0.757, .b = 0.31, .a = 1.0}, // frog
	{.r = 0.792, .g = 0.294, .b = 0.314, .a = 1.0} // ladybug
};

static struct {
	int actions;
	char* names[MAX_ACTIONS];
} ANIMAL_ACTIONS[] = {
	{.actions = 3, .names = {"eyeroll", "fly1", "fly2"}}, // bee
	{.actions = 3, .names = {"eyeroll", "sing1", "sing2"}}, // bird
	{.actions = 3, .names = {"action1", "action2", "action3"}}, // cat
	{.actions = 3, .names = {"eyeroll", "swim1", "swim2"}}, // fish
	{.actions = 5, .names = {"bump1", "bump2", "eyeroll", "tonque1", "tonque2"}}, // frog
	{.actions = 6, .names = {"bump1", "bump2", "bump3", "eyeroll1", "eyeroll2", "kiss"}} // ladybug
};

static struct {
	int actions;
	char* names[MAX_ACTIONS];
} SPECIAL_ACTIONS[] = {
	{.actions = 2, .names = {"stand", "stand2"}}, // egg
	{.actions = 1, .names = {"stand"}}, // berry
	{.actions = 1, .names = {"stand"}}, // strawberry
	{.actions = 1, .names = {"stand"}}, // mushroom
	{.actions = 1, .names = {"stand"}}, // cone
	{.actions = 2, .names = {"stand", "stand2"}}, // apple
	{.actions = 3, .names = {"stand", "stand2", "stand3"}}, // chestnut
	{.actions = 6, .names = {"bee", "bird", "cat", "fish", "frog", "ladybug"}}, // special
	{.actions = 1, .names = {"eyes"}}, // eyes
	{.actions = 1, .names = {"stand"}}, // dandelion
	{.actions = 2, .names = {"anim", "anim2"}}, // cloud
};

SUPPRESS_END

struct FieldID {
	int i;
	int j;
};

struct Field {
	struct FieldID id;
	enum FIELD_TYPE type;
	union {
		struct {
			enum COLLECTIBLE_TYPE type;
			int variant;
		} collectible;
		struct {
			enum ANIMAL_TYPE type;
			bool sleeping;
			bool super;
		} animal;
		struct {
			int variant;
		} freefall;
	} data;
	int matched;
	int match_mark;
	bool to_remove;
	bool handled;
	bool to_highlight;
	bool locked;

	struct Character *drawable, *overlay;
	bool overlay_visible;

	float highlight;

	struct {
		struct Tween hiding, falling, swapping, shaking, hinting, launching, collecting;
		struct FieldID swapee, super;
		int fall_levels, level_no;
		int time_to_action, action_time;
		int time_to_blink, blink_time;
	} animation;
};

struct Goal {
	enum GOAL_TYPE type;
	int value;
};

struct Level {
	bool field_types[FIELD_TYPES];
	bool animals[ANIMAL_TYPES];
	bool collectibles[SPECIAL_TYPES];
	struct {
		enum FIELD_TYPE field_type;
		enum COLLECTIBLE_TYPE collectible_type;
		enum ANIMAL_TYPE animal_type;
		int variant;
		bool random_subtype;
		bool sleeping;
		bool super;
	} fields[COLS][ROWS];
	bool supers, sleeping;

	struct Goal goals[3];

	bool infinite;
	int moves;
	int id;
};

struct GamestateResources {
	// This struct is for every resource allocated and used by your gamestate.
	// It gets created on load and then gets passed around to all other function calls.
	ALLEGRO_BITMAP *bg, *leaf;

	ALLEGRO_BITMAP *scene, *lowres_scene, *lowres_scene_blur, *board, *placeholder, *restart;

	ALLEGRO_FONT *font, *font_num_small, *font_num_medium, *font_num_big, *font_small;

	ALLEGRO_BITMAP *frame, *frame_bg;

	struct Character* animal_archetypes[sizeof(ANIMALS) / sizeof(ANIMALS[0])];
	struct Character* special_archetypes[sizeof(SPECIALS) / sizeof(SPECIALS[0])];
	struct FieldID current, hovered, swap1, swap2;
	struct Field fields[COLS][ROWS];

	struct Timeline* timeline;

	ALLEGRO_BITMAP *field_bgs[4], *field_bgs_bmp;

	ALLEGRO_SHADER *combine_shader, *desaturate_shader;

	bool locked, clicked;

	struct ParticleBucket* particles;

	struct Character *leaves, *ui, *beetle, *snail, *restart_btn;

	struct {
		struct Character* character;
		struct Tween tween;
	} acorn_top, acorn_bottom;

	float snail_blink;

	int moves, moves_goal, score;
	struct Tween scoring, finishing, failing;

	struct Goal goals[3];

	struct Level level;

	bool debug, paused, menu, done, failed, restart_hover, infinite, goal_lock;
	float counter, counter_speed, counter_strength;
};

// actions
void Turn(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two);
bool ShowHint(struct Game* game, struct GamestateResources* data);
bool AutoMove(struct Game* game, struct GamestateResources* data);

// fields
bool IsSameID(struct FieldID one, struct FieldID two);
bool IsValidID(struct FieldID id);
struct FieldID ToLeft(struct FieldID id);
struct FieldID ToRight(struct FieldID id);
struct FieldID ToTop(struct FieldID id);
struct FieldID ToBottom(struct FieldID id);
struct Field* GetField(struct Game* game, struct GamestateResources* data, struct FieldID id);
bool WillMatchAfterSwapping(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two);
int IsMatching(struct Game* game, struct GamestateResources* data, struct FieldID id);
bool IsSleeping(struct Field* field);
bool IsDrawable(enum FIELD_TYPE type);
int IsMatchExtension(struct Game* game, struct GamestateResources* data, struct FieldID id);
int ShouldBeCollected(struct Game* game, struct GamestateResources* data, struct FieldID id);
bool WillMatch(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two);

// levels
void LoadLevel(struct Game* game, struct GamestateResources* data, int id);
void ApplyLevel(struct Game* game, struct GamestateResources* data);
void StoreLevel(struct Game* game, struct GamestateResources* data);
void RestartLevel(struct Game* game, struct GamestateResources* data);
void SanityCheckLevel(struct Game* game, struct GamestateResources* data);
void FinishLevel(struct Game* game, struct GamestateResources* data);
void FailLevel(struct Game* game, struct GamestateResources* data);
void CopyLevel(struct Game* game, struct GamestateResources* data);

// logic
void UpdateGoal(struct Game* game, struct GamestateResources* data, enum GOAL_TYPE type, int val);
void AddScore(struct Game* game, struct GamestateResources* data, int val);
int MarkMatching(struct Game* game, struct GamestateResources* data);
void GenerateAnimal(struct Game* game, struct GamestateResources* data, struct Field* field, bool allow_matches);
void GenerateField(struct Game* game, struct GamestateResources* data, struct Field* field, bool allow_matches);
void Gravity(struct Game* game, struct GamestateResources* data);
void ProcessFields(struct Game* game, struct GamestateResources* data);
bool CanBeMatched(struct Game* game, struct GamestateResources* data, struct FieldID id);
int CountMoves(struct Game* game, struct GamestateResources* data);
void DoRemoval(struct Game* game, struct GamestateResources* data);
void StopAnimations(struct Game* game, struct GamestateResources* data);
void Swap(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two);
void StartSwapping(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two);
void StartBadSwapping(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two);
void SpawnParticles(struct Game* game, struct GamestateResources* data, struct FieldID id, int num);
TM_ACTION(DispatchAnimations);

// specials
bool AnimateSpecials(struct Game* game, struct GamestateResources* data);
void TurnMatchToSuper(struct Game* game, struct GamestateResources* data, int matched, int mark);
void HandleSpecialed(struct Game* game, struct GamestateResources* data, struct Field* field);
TM_ACTION(AnimateSpecial);

// view
struct DandelionParticleData* DandelionParticleData(ALLEGRO_COLOR color);
bool DandelionParticle(struct Game* game, struct ParticleState* particle, double delta, void* d);
bool IsValidMove(struct FieldID one, struct FieldID two);
bool IsSwappable(struct Game* game, struct GamestateResources* data, struct FieldID id);
bool AreSwappable(struct Game* game, struct GamestateResources* data, struct FieldID one, struct FieldID two);
void UpdateDrawable(struct Game* game, struct GamestateResources* data, struct FieldID id);
void DrawField(struct Game* game, struct GamestateResources* data, struct FieldID id);
void DrawOverlay(struct Game* game, struct GamestateResources* data, struct FieldID id);

// scene
void DrawScene(struct Game* game, struct GamestateResources* data);
void UpdateBlur(struct Game* game, struct GamestateResources* data);

// debug
void HandleDebugEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev);
void DrawDebugInterface(struct Game* game, struct GamestateResources* data);

#endif
