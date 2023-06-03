#ifndef WEAPONS_H
#define WEAPONS_H

#include <stdbool.h>
#include <sglthing/world.h>
#include <sglthing/net.h>

enum weapon_type
{
    WEAPON_HOLSTER,
    WEAPON_SHOVEL,
    WEAPON_BLOCK,
    WEAPON_AK47,

    __WEAPON_MAX,
};

bool weapon_fire_primary(struct network* network, int player_id, bool server);
bool weapon_fire_secondary(struct network* network, int player_id, bool server);

void weapon_trigger_fire(struct world* world, bool secondary);

#endif