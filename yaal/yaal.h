#ifndef YAAL_H
#define YAAL_H
#include <sglthing/net.h>

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
};

enum __attribute__((__packed__)) yaal_light_type
{
    LIGHT_NONE,
    LIGHT_OBJ4_LAMP,
};

#define MAP_SIZE_MAX_X 32
#define MAP_SIZE_MAX_Y 16
#define MAP_GRAPHICS_IDS 5
#define MAP_TEXTURE_IDS 6

struct map_tile_data
{
    enum __attribute__((__packed__)) {
        TILE_AIR,
        TILE_FLOOR,
        TILE_WALL,
        TILE_WALL_CHEAP,

        // event
        TILE_MOVE_EVENT,
    } map_tile_type;

    char tile_graphics_id;
    char tile_graphics_ext_id;
    char tile_graphics_tex;
    enum yaal_light_type tile_light_type;

    enum direction direction;
};

struct map_object
{
    int object_id;
    int object_graphics_id;
    int object_graphics_tex;
};

struct map_file_data
{
    char level_name[64];
    int level_song_id;
    int level_id;

    struct {
        struct map_tile_data data[MAP_SIZE_MAX_Y];
    } map_row[MAP_SIZE_MAX_X];

    struct map_object map_object_list[512];
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
        } update_position;
        struct 
        {
            int player_id;
            int animation_id;
        } update_playeranim;
    } packet;
};

#endif