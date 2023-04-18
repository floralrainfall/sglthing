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
#include <sglthing/light.h>
#include <sglthing/io.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <glib.h>

#define MAP_SIZE_MAX_X 32
#define MAP_SIZE_MAX_Y 16
#define MAP_GRAPHICS_IDS 5
#define MAP_TEXTURE_IDS 7
#define ACTION_PICTURE_IDS 4
#define MAP_TILE_SIZE 2
#define HOTBAR_NUM_KEYS 9
#define ANIMATIONS_TO_LOAD 64

enum __attribute__((__packed__)) direction 
{
    DIR_NORTH,
    DIR_SOUTH,
    DIR_WEST,
    DIR_EAST,
    DIR_MAX,
};

enum yaal_packet_type
{
    YAAL_ENTER_LEVEL = 0x4a11,
    YAAL_LEVEL_DATA,
    YAAL_UPDATE_POSITION,
    YAAL_UPDATE_PLAYERANIM,
    YAAL_UPDATE_OBJECT,
    YAAL_UPDATE_PLAYER_ACTION,
    YAAL_PLAYER_ACTION,
};

enum __attribute__((__packed__)) yaal_light_type
{
    LIGHT_NONE,
    LIGHT_OBJ4_LAMP,
};

struct map_tile_data
{
    enum __attribute__((__packed__)) {
        TILE_AIR,
        TILE_FLOOR,
        TILE_WALL,
        TILE_WALL_CHEAP,
        TILE_WALKAIR,
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
    char object_id;
    char object_graphics_id;
    char object_graphics_tex;
    int object_tile_x;
    int object_tile_y;

    char object_name[64];
    int object_data_id;

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

    int map_object_count;
    struct map_object map_objects[UCHAR_MAX];
};

struct player_action
{
    int action_id;
    int action_picture_id;
    char action_name[64];
    char action_desc[256];
    bool visible;
};

struct xtra_packet_data
{
    union
    {
        struct
        {
            int level_id;
            char level_name[64];
        } yaal_level;
        struct
        {
            int level_id;
            int yaal_x;
            struct map_tile_data data[MAP_SIZE_MAX_Y];
        } yaal_level_data;
        struct
        {
            int player_id;
            vec3 delta_pos;
            vec2 delta_angles;
            bool urgent;
        } update_position;
        struct 
        {
            int player_id;
            int animation_id;
        } update_playeranim;
        struct
        {

        } map_object_create;
        struct
        {
            
        } map_object_update;
        struct
        {

        } map_object_destroy;
        struct
        {
            int hotbar_id;
            struct player_action new_action_data;
        } player_update_action;
        struct
        {
            int action_id;
            vec3 position;
        } player_action;
    } packet;
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

    bool combat_mode;

    mat4 model_matrix;
    mat4 ghost_matrix;

    struct light player_light;
};

struct yaal_state {
    int current_level_id;
    char level_name[64];

    int player_id;
    struct player* current_player;

    struct model* player_model;
    struct animation player_animations[ANIMATIONS_TO_LOAD];

    struct model* map_tiles[MAP_GRAPHICS_IDS];
    int map_graphics_tiles[MAP_TEXTURE_IDS];
    struct light_area* area;
    
    struct player_action player_actions[9];
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
    int player_chat_on_tex;
    int player_chat_off_tex;
    int player_menu_button_tex;

    int player_shader;
    int object_shader;

    GHashTable* map_objects;
    GHashTable* maps;

    int map_downloaded_count;
    bool map_downloaded[MAP_SIZE_MAX_X];
    bool map_downloading;
    struct map_tile_data map_data[MAP_SIZE_MAX_X][MAP_SIZE_MAX_Y];
};

#endif