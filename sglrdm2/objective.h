#ifndef OBJECTIVE_H
#define OBJECTIVE_H

#include <glib.h>

#define MAX_OBJECTIVES 16

enum objective_type
{
    RDM_OBJECTIVE_NONE,

    RDM_OBJECTIVE_KILL_PLAYER,
    RDM_OBJECTIVE_STEAL_ITEM,

    __RDM_OBJECTIVE_MAX_RANDOM, // ones below will not be selected by antagonist_random_objective

    RDM_OBJECTIVE_SURVIVE,
};

enum antagonist_type
{
    RDM_ANTAGONIST_NEUTRAL,
    RDM_ANTAGONIST_SPY,
    RDM_ANTAGONIST_TRAITOR,
};

struct objective
{
    enum objective_type type;
    union {
        struct {
            int player_id;
        } kill;
        struct {
            int item_id;
        } steal;
    } objectives;
};

struct antagonist
{
    enum antagonist_type type;
    struct objective objectives[MAX_OBJECTIVES];
    int objective_count;
};

void objective_description(char* out, int len, struct objective objective);
char* antagonist_name(enum antagonist_type type);
void antagonist_select(GArray* server_clients);

#endif