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
}

struct objective
{
    enum objective_type type;
    union objectives {
        struct {
            int player_id;
        } kill;
        struct {
            int item_id;
        } steal;
    }
};

struct antagonist
{

};

#endif