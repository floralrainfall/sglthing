#ifndef UTIL_H
#define UTIL_H

#include <sglthing/net.h>
#include "yaal.h"

// server
void yaal_update_player_action(int key_id, struct player_action action, struct network_client* client, struct network* network);
// server
#define BOMB_THROW_ACTION_UPDATE(p, c, n) \  
    yaal_update_player_action(0, (struct player_action){.action_name = "Bomb", .action_desc = "Drop a bomb", .action_id = ACTION_BOMB_DROP, .action_picture_id = 2, .visible = true, .action_count = p->player_bombs, .combat_mode = true, .action_aim = false}, c, n); \
    yaal_update_player_action(1, (struct player_action){.action_name = "Throw", .action_desc = "Throw a bomb", .action_id = ACTION_BOMB_THROW, .action_picture_id = 6, .visible = true, .action_count = p->player_bombs, .combat_mode = true, .action_aim = true}, c, n); \

// client
void yaal_update_player_transform(struct player* player);
void yaal_update_song();

#endif