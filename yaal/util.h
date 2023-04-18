#ifndef UTIL_H
#define UTIL_H

#include <sglthing/net.h>
#include "yaal.h"

// server
void yaal_update_player_action(struct player_action action, struct network_client* client, struct network* network);

#endif