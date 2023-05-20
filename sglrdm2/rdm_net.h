#ifndef RDM_NET_H
#define RDM_NET_H

#include <sglthing/world.h>
#include <sglthing/net.h>
#include "weapons.h"
#include "gamemode.h"
#include "map.h"

enum rdm_packet_type
{
    RDM_PACKET_UPDATE_POSITION = 0x4d52,
    RDM_PACKET_UPDATE_WEAPON,
    RDM_PACKET_UPDATE_TEAM,
    RDM_PACKET_UPDATE_CHUNK,
    RDM_PACKET_REQUEST_CHUNK,
    RDM_PACKET_UPDATE_GAMEMODE,
};

union rdm_packet_data
{
    struct
    {
        vec3 position;
        float yaw, pitch;
        int player_id;    
        bool urgent;    
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
        
        struct {
            char chunk_data[RENDER_CHUNK_SIZE]; // z
        } chunk_data[RENDER_CHUNK_SIZE]; // y
    } update_chunk;
    struct
    {
        int chunk_x;
        int chunk_y;
        int chunk_z;
    } request_chunk;
    struct
    {
        struct gamemode_data gm_data;
    } update_gamemode;    
};

struct rdm_player
{
    struct network_client* client;
    vec3 position;
    vec3 replicated_position;
    enum rdm_team team;
    enum weapon_type active_weapon;
    float yaw, pitch;
};

void net_send_chunk(struct network* network, struct network_client* client, int c_x, int c_y, int c_z, struct map_chunk* chunk);
void net_sync_gamemode(struct network* network, struct gamemode_data* gamemode);
void net_init(struct world* world);

#endif