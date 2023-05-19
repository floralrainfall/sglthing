#ifndef RDM_NET_H
#define RDM_NET_H

#include <sglthing/world.h>
#include <sglthing/net.h>
#include "weapons.h"
#include "map.h"

enum rdm_packet_type
{
    RDM_PACKET_UPDATE_POSITION = 0x4d52,
    RDM_PACKET_UPDATE_WEAPON,
    RDM_PACKET_UPDATE_TEAM,
    RDM_PACKET_UPDATE_CHUNK,
    RDM_PACKET_REQUEST_CHUNK,
    RDM_PACKET_FINISH_CHUNK,
};

enum rdm_team
{
    TEAM_NEUTRAL,
    TEAM_RED,
    TEAM_BLUE,
    TEAM_NA,
    TEAM_SPECTATOR,
};

union rdm_packet_data
{
    struct
    {
        vec3 position;
        float look_x, look_y;
        int player_id;        
    } update_position;
    struct 
    {
        enum weapon_type weapon;
        int player_id;
    } update_weapon;
    struct 
    {
        enum rdm_team team;
        int player_id;
    } update_team;
    struct 
    {
        int chunk_x;
        int chunk_y;
        int chunk_z;

        int chunk_data_x;
        int chunk_data_y;

        char chunk_data[RENDER_CHUNK_SIZE]; // z
    } update_chunk;
    struct
    {
        int chunk_x;
        int chunk_y;
        int chunk_z;
    } request_chunk;
};

struct rdm_player
{
    struct network_client* client;
    vec3 position;
    vec3 replicated_position;
    enum rdm_team team;
    enum weapon_type active_weapon;
};

void net_init(struct world* world);

#endif