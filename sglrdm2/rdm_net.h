#ifndef RDM_NET_H
#define RDM_NET_H

#include <sglthing/world.h>
#include <sglthing/net.h>
#include "weapons.h"
#include "gamemode.h"
#include "objective.h"
#include "map.h"

enum rdm_packet_type
{
    RDM_PACKET_UPDATE_POSITION = 0x4d52,
    RDM_PACKET_UPDATE_WEAPON,
    RDM_PACKET_UPDATE_TEAM,
    RDM_PACKET_UPDATE_CHUNK,
    RDM_PACKET_UPDATE_HOTBAR,
    RDM_PACKET_UPDATE_INVENTORY,
    RDM_PACKET_REQUEST_CHUNK,
    RDM_PACKET_DESTROY_CHUNK,
    RDM_PACKET_UPDATE_GAMEMODE,
    RDM_PACKET_GAME_START,
    RDM_PACKET_WEAPON_FIRE,
    RDM_PACKET_PLAYER_STATUS,
    RDM_PACKET_REPLICATE_SOUND,
    RDM_PACKET_PLAYER_DEATH,
};

enum rdm_damage_type
{
    RDM_DAMAGE_UNKNOWN,
    RDM_DAMAGE_SHOVEL,
    RDM_DAMAGE_AK47,
    RDM_DAMAGE_FALLING,
};

union rdm_packet_data
{
    struct
    {
        vec3 position;
        versor direction;
        int player_id;    
        bool urgent;    
    } update_position;
    struct 
    {
        int hotbar_id;
        enum weapon_type weapon;
        int player_id;
        int block_color;
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
    struct
    {
        int player_id;
        int hotbar_slot;
        int inventory_id;
    } update_hotbar;
    struct
    {
        int inventory_id;
        int item_id;
    } update_inventory;
    struct
    {
        int player_id;
        bool secondary;
        versor direction;
    } weapon_fire;
    struct
    {
        int player_id;
        bool urgent;
        int health;
        int weapon_ammos[__WEAPON_MAX];
        struct antagonist antagonist_data;
    } player_status;
    struct
    {
        bool sound_3d;
        vec3 position;
        char sound_name[128];
    } replicate_sound;
    struct
    {
        int player_id;
        int attacker_id;
        enum rdm_damage_type type;
    } player_death;
};

struct rdm_player
{
    struct network_client* client;
    vec3 position;
    vec3 replicated_position;
    enum rdm_team team;

    enum weapon_type active_weapon_type;
    int active_hotbar_id;
    int weapon_hotbar[9];
    int inventory[128];

    int weapon_ammos[__WEAPON_MAX];
    unsigned char weapon_block_color;

    versor direction;

    struct antagonist antagonist_data;
    double primary_next_fire;
    double secondary_next_fire;

    int health;
    int max_health;

    int player_id;
};

struct pending_packet
{
    struct network_client* client;
    struct network_packet* packet;
    double giveup;
};

void net_player_syncinfo2(struct network* network, struct rdm_player* player);
void net_player_syncinfo(struct network* network, struct rdm_player* player);
void net_player_damage(struct network* network, struct rdm_player* player, enum rdm_damage_type type, int damage, int attacker_id);
void net_player_moveto(struct network* network, struct rdm_player* player, vec3 position);
void net_send_chunk(struct network* network, struct network_client* client, int c_x, int c_y, int c_z, struct map_chunk* chunk);
void net_sync_gamemode(struct network* network, struct gamemode_data* gamemode);
void net_play_sound(struct network* network, struct rdm_player* player, char* snd_name, vec3 position);
void net_play_g_sound(struct network* network, char* snd_name, vec3 position);
void net_init(struct world* world);

enum weapon_type net_player_get_weapon(struct rdm_player* player);

#endif