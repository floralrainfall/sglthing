#ifndef NETMGR_H
#define NETMGR_H

#include <sglthing/world.h>
#include "yaal.h"
#include "util.h"

enum yaal_packet_type
{
    YAAL_ENTER_LEVEL = 0x4a11,
    YAAL_LEVEL_DATA,

    YAAL_CREATE_OBJECT,
    YAAL_UPDATE_OBJECT,
    YAAL_REMOVE_OBJECT,
    YAAL_INTERACT_OBJECT,

    YAAL_UPDATE_POSITION,
    YAAL_UPDATE_PLAYERANIM,
    YAAL_UPDATE_PLAYER_ACTION,
    YAAL_UPDATE_COMBAT_MODE,
    YAAL_UPDATE_STATS,

    YAAL_RPG_MESSAGE,

    YAAL_PLAYER_ACTION,
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
            int level_id;
            vec3 delta_pos;
            versor new_versor;
            float pitch;
            bool urgent;
            double lag;
        } update_position;
        struct 
        {
            int player_id;
            int animation_id;
        } update_playeranim;
        struct
        {
            int player_id;
            int player_health;
            int player_max_health;
            int player_mana;
            int player_max_mana;
            int player_bombs;
            int player_coins;
        } update_stats;
        struct
        {
            struct map_object object;
        } map_object_create;
        struct
        {
            struct map_object object;
        } map_object_update;
        struct
        {
            int object_ref_id;
        } map_object_destroy;
        struct
        {
            int tile_x;
            int tile_y;
        } map_object_interact;
        struct
        {
            int hotbar_id;
            struct player_action new_action_data;
        } player_update_action;
        struct
        {
            enum action_id action_id;
            int player_id;
            int level_id;
            vec3 position;
        } player_action;
        struct
        {
            int player_id;
            bool combat_mode;
        } update_combat_mode;
        struct
        {
            char text[256];
        } rpg_message;
    } packet;
};

struct net_radius_detection
{
    float distance;
    struct network_client* client;
};

#define NEW_RADIUS g_array_new(true, true, sizeof(struct net_radius_detection));

void net_init(struct world* world);
void net_players_in_radius(GArray* clients, float radius, vec3 position, int level_id, GArray* out);
void net_upd_player(struct network* network, struct network_client* client);
void net_player_hurt(struct network* network, struct network_client* client, int damage);

#endif