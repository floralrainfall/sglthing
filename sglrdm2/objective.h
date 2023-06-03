#ifndef OBJECTIVE_H
#define OBJECTIVE_H

enum objective_type
{
    RDM_OBJECTIVE_KILL_PLAYER,
    RDM_OBJECTIVE_STEAL_ITEM,
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
            char player_name[64];
        } kill;
        struct {
            int item_id;
        } steal;
    } objectives;
};

struct antagonist
{
    enum antagonist_type type;
    struct objective objectives[4];
};

void objective_description(char* out, int len, struct objective objective);

#endif