#ifndef YAAL_H
#define YAAL_H
#include <sglthing/net.h>

enum direction
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
    YAAL_UPDATE_POSITION,
};

#define MAP_SIZE_MAX_X 32
#define MAP_SIZE_MAX_Y 32
#define MAP_GRAPHICS_IDS 2
#define MAP_TEXTURE_IDS 1

struct map_tile_data
{
    enum {
        TILE_AIR,
        TILE_FLOOR,
        TILE_WALL,
        TILE_WALL_CHEAP,

        // event
        TILE_MOVE_EVENT,
    } map_tile_type;

    int tile_graphics_id;
    int tile_graphics_ext_id;
    int tile_graphics_tex;
    enum direction direction;
};

struct xtra_packet_data
{
    union
    {
        struct
        {
            int level_id;
            int level_song_id;
            
            struct map_tile_data map_data[MAP_SIZE_MAX_X][MAP_SIZE_MAX_Y];
        } yaal_level;
        struct
        {
            int player_id;
            vec3 delta_pos;
            vec2 delta_angles;
        } update_position;
    } packet;
};

#endif