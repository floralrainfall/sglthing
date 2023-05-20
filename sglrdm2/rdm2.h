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
    struct ui_font2* big_font;
    struct ui_font2* normal_font;

    struct rdm_player* local_player;
    struct map_manager* map_manager;
    int local_player_id;

    struct map_server* map_server;

    int object_shader;
    int skybox_shader;
    int cloud_layer_shader;
    
    struct gamemode_data gamemode;

    vec3 player_velocity;
    bool player_grounded;
};

extern struct rdm2_state client_state;
extern struct rdm2_state server_state;

#endif