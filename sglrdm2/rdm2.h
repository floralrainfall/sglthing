#ifndef RDM2_H
#define RDM2_H

#include <sglthing/world.h>
#include "rdm_net.h"
#include "map.h"

void sglthing_init_api(struct world* world);

struct rdm2_state
{
    struct rdm_player* local_player;
    struct map_manager* map_manager;
    int local_player_id;

    int object_shader;
};

extern struct rdm2_state client_state;
extern struct rdm2_state server_state;

#endif