#ifndef RDM2_H
#define RDM2_H

#include <sglthing/world.h>
#include <sglthing/http.h>
#include <sglthing/net.h>
#include "rdm_net.h"
#include "map.h"
#include "gamemode.h"

void sglthing_init_api(struct world* world);

struct rdm2_state
{
    struct model* cloud_layer;
    struct model* sun;
    struct model* rdm_guy;
    struct model* vc_bubble;
    
    struct model* weapons[__WEAPON_MAX];
    int weapon_icon_textures[__WEAPON_MAX];

    struct ui_font2* big_font;
    struct ui_font2* normal_font2;
    struct ui_font2* normal_font;

    struct snd* lobby_music;
    struct snd* playing_music;
    struct snd* roundstart_sound;

    struct http_user local_http_user;
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
    struct rdm_player* mouse_player;
    bool mouse_block_v;

    struct gamemode_data gamemode;

    vec3 player_velocity;
    bool player_grounded;

    bool server_motd_dismissed;
    bool server_information_panel;
    bool server_selection_panel;
    bool context_mode;

    GArray* server_list;
    GArray* chunk_packets_pending;
    double next_pending;

    bool client_stencil;

    vec3 last_position;
    float mouse_update_diff;

    float logo_title;
    int logo_title_tex;

    struct network* network;
};

extern struct rdm2_state client_state;
extern struct rdm2_state server_state;

#endif