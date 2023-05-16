#ifndef RDM_NET_H
#define RDM_NET_H

#include <sglthing/world.h>
#include <sglthing/net.h>
#include "weapons.h"

enum rdm_packet_type
{
    RDM_PACKET_UPDATE_POSITION = 0x4d52,
    RDM_PACKET_UPDATE_WEAPON,
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
    struct update_position
    {
        vec3 position;
        float look_x, look_y;
        int player_id;        
    };
    struct update_weapon
    {
        enum weapon_type weapon;
        int player_id;
    };
    struct update_team
    {
        enum rdm_team team;
        int player_id;
    };
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