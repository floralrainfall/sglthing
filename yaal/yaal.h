#ifndef YAAL_H
#define YAAL_H
#include <sglthing/net.h>

enum direction
{
    north,
    south,
    west,
    east,
};

enum yaal_packet_type
{
    YAAL_ENTER_LEVEL = 0x4a11,
    YAAL_UPDATE_POSITION,
};

struct xtra_packet_data
{
    union
    {
        struct
        {
            int level_id;
            int level_song_id;
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