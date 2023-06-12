#ifndef RDM2_H
#define RDM2_H

#include <sglthing/world.h>
#include "rdm_net.h"
#include "map.h"
#include "gamemode.h"

void sglthing_init_api(struct world* world);

struct rdm2_state
{
    struct model* cloud_layer;
    struct model* sun;
    struct model* rdm_guy;
    
    struct model* weapons[__WEAPON_MAX];
    int weapon_icon_textures[__WEAPON_MAX];

    struct ui_font2* big_font;
    struct ui_font2* normal_font;

    struct snd* lobby_music;
    struct snd* playing_music;
    struct snd* roundstart_sound;

    struct rdm_player* local_player;
    struct map_manager* map_manager;
    int local_player_id;

    struct map_server* map_server;

    int object_shader;
    int skybox_shader;
    int cloud_layer_shader;
    int stencil_shader;
    
    int mouse_block_x;
    int mouse_block_y;
    int mouse_block_z;
    bool mouse_block_v;

    struct gamemode_data gamemode;

    vec3 player_velocity;
    bool player_grounded;

    bool server_motd_dismissed;
    bool server_information_panel;
    bool context_mode;

    GArray* chunk_packets_pending;
    double next_pending;

    bool client_stencil;

    vec3 last_position;
    float mouse_update_diff;
};

extern struct rdm2_state client_state;
extern struct rdm2_state server_state;

#endif