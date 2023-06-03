#include "objective.h"
#include <stdio.h>

void objective_description(char* out, int len, struct objective objective)
{
    switch(objective.type)
    {
        case RDM_OBJECTIVE_KILL_PLAYER:
            snprintf(out, len, "Kill %s.", objective.objectives.kill.player_name);
            break;
        case RDM_OBJECTIVE_STEAL_ITEM:
            snprintf(out, len, "Steal a (tmp: %i) and hold on to it for the entire round.", objective.objectives.steal.item_id);
            break;
        default:
            snprintf(out, len, "IDK, LOL!");
            break;
    }
}