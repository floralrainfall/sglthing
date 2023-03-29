#ifndef S7_SCRIPT_NETWORKING_H
#define S7_SCRIPT_NETWORKING_H

#include "s7.h"
#include "../net.h"
#include "../script.h"

void sgls7_registernetworking(s7_scheme* sc);

void nets7_init_network(s7_scheme* sc, struct network* net);
void nets7_tick_network(s7_scheme* sc, struct network* net, struct network_client* client);
void nets7_receive_event(s7_scheme* sc, struct network* net, struct network_client* client, struct network_packet* packet);

#endif