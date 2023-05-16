#ifndef YAAL_H
#define YAAL_H
#include <sglthing/sglthing.h>
#include <sglthing/world.h>
#include <sglthing/net.h>
#include <sglthing/model.h>
#include <sglthing/texture.h>
#include <sglthing/shader.h>
#include <sglthing/keyboard.h>
#include <sglthing/animator.h>
#include <sglthing/particles.h>
#include <sglthing/light.h>
#include <sglthing/io.h>
#ifndef HEADLESS
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif
#include <stdlib.h>
#include <glib.h>
#include "chat.h"
#include "fx.h"

#define MAP_SIZE_MAX_X 32
#define MAP_SIZE_MAX_Y 16
#define MAP_GRAPHICS_IDS 5
#define MAP_TEXTURE_IDS 7
#define ACTION_PICTURE_IDS 7
#define MAP_TILE_SIZE 2
#define HOTBAR_NUM_KEYS 9
#define ANIMATIONS_TO_LOAD 64

enum action_id {
    ACTION_EXIT,
    ACTION_SERVER_MENU,
    ACTION_BOMB_THROW,
    ACTION_BOMB_DROP,
    ACTION_HEAL
};

enum object_id {
    OBJECT_BOMB,
    OBJECT_DOOR,
    OBJECT_TRANSPORT,
};

enum __attribute__((__packed__)) direction 
{
    DIR_NORTH,
    DIR_SOUTH,
    DIR_WEST,
    DIR_EAST,
    DIR_MAX,
};

enum __attribute__((__packed__)) yaal_light_type
{
    LIGHT_NONE,
    LIGHT_OBJ4_LAMP,
};

enum skill_level_type
{
    SKILL_SWORD,
    SKILL_DEMOLITIONS,
    SKILL_SURVIVAL,
    SKILL_AGILITY,
    SKILL_SNEAK,
    SKILL_PERCEPTION,
    SKILL_INTELLIGENCE,
    __SKILL_MAX
};

struct map_tile_data
{
    enum __attribute__((__packed__)) {
        TILE_AIR,
        TILE_FLOOR,
        TILE_WALL,
        TILE_WALL_CHEAP,
        TILE_WALKAIR,
        TILE_WATER,
        TILE_DEBUG,
    } map_tile_type;

    char tile_graphics_id;
    char tile_graphics_ext_id;
    char tile_graphics_tex;
    enum yaal_light_type tile_light_type;

    enum direction direction;
};

struct map_object
{
    int object_ref_id;
    enum object_id object_id;
    char object_graphics_id;
    char object_graphics_tex;
    int object_tile_x;
    int object_tile_y;

    char object_name[64];

    union object_data
    {
        struct transport_data
        {
            int level_id_to;
            int tile_x;
            int tile_y;
        };
    };
};

struct map_file_data
{
    char level_name[64];
    int level_song_id;
    int level_id;

    struct {
        struct map_tile_data data[MAP_SIZE_MAX_Y];
    } map_row[MAP_SIZE_MAX_X];

    int map_right_id;
    int map_top_id;
    int map_left_id;
    int map_bottom_id;
    bool map_pvp_enabled;
    int map_song_id;

    int map_object_count;
    struct map_object map_objects[UCHAR_MAX];
};

struct player_action
{
    enum action_id action_id;
    int action_picture_id;
    int action_count;
    char action_name[9];
    char action_desc[256];
    bool visible;
    bool combat_mode;
    bool action_aim;
};

struct player {
    struct network_client* client;
    struct animator animator;

    enum direction dir;

    vec3 player_position;
    vec3 old_position;

    int player_id;
    int level_id;

    int map_tile_x;
    int map_tile_y;

    int player_health;
    int player_max_health;
    int player_mana;
    int player_max_mana;
    int player_bombs;
    int player_coins;

    versor rotation_versor;

    bool combat_mode;

    mat4 model_matrix;
    mat4 ghost_matrix;

    struct light player_light;

    int skill_level[__SKILL_MAX];
};

struct yaal_state {
    int current_level_id;
    char level_name[64];
    char rpg_message[256];
    bool show_rpg_message;

    enum
    {
        YAAL_STATE_MENU,
        YAAL_STATE_MAP,
        YAAL_STATE_GAME,
    } mode;

    int player_id;
    struct player* current_player;

    GArray* tmp_range_calculation;
    float player_heart_end;
    int player_heart_amount;

    struct model* player_model;
    struct model* arrow_model;
    struct model* menu_yaal_model;
    struct model* menu_city_model;
    struct light arrow_light;

    struct ui_font* damage_font;

    struct model* map_tiles[MAP_GRAPHICS_IDS];
    int map_graphics_tiles[MAP_TEXTURE_IDS];
    struct light_area* area;

    int skill_icons[__SKILL_MAX];
    
    bool player_first_person;
    bool player_menu_open;
    bool player_chat_mode;
    bool player_action_disable;
    bool player_action_debounce[9];
    int player_action_images[ACTION_PICTURE_IDS];
    int player_action_disabled_tex;
    int player_heart_full_tex;
    int player_heart_half_tex;
    int player_heart_none_tex;
    int player_mana_full_tex;
    int player_mana_half_tex;
    int player_mana_none_tex;
    int player_combat_on_tex;
    int player_combat_off_tex;
    int player_combat_mode_tex;
    int player_chat_on_tex;
    int player_chat_off_tex;
    int player_menu_button_tex;
    int player_cancel_button_tex;
    int player_play_button_tex;
    int player_ok_button_tex;
    int player_hotbar_bar_tex;
    int player_coins_tex;
    int player_lag_tex;
    int player_exit_tex;
    int player_yes_tex;
    int player_shader;
    int friends_tex;
    int select_particle_tex;
    int object_shader;
    int current_aiming_action;
    vec3 aiming_arrow_position;
    vec3 last_aim_position;

    int last_player_list_id;

    GHashTable* map_objects;
    GHashTable* maps;

    int current_map_song_id;
    struct snd* current_song;

    struct animation player_animations[ANIMATIONS_TO_LOAD];

    int map_downloaded_count;
    bool map_downloaded[MAP_SIZE_MAX_X];
    bool map_downloading;

    struct player_action player_actions[9];
    struct particle_system particles;
    struct chat_system* chat;
    struct fx_manager fx_manager;
    struct map_tile_data map_data[MAP_SIZE_MAX_X][MAP_SIZE_MAX_Y];
};

extern struct yaal_state yaal_state;
extern struct yaal_state server_state;

#endif