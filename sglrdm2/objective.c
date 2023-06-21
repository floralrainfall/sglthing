#include "objective.h"
#include <stdio.h>
#include <sglthing/net.h>
#include "rdm_net.h"

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

#define TRAITOR_PERCENTAGE 1.0

void antagonist_set_objectives(struct antagonist* antag_data)
{
    
}

void antagonist_select(GArray* server_clients)
{
    float population = server_clients->len;
    float antagonists = 0.0;
    float traitors = roundf(population * TRAITOR_PERCENTAGE); 
    antagonists += traitors;
    printf("rdm2[server]: selecting %.0f antagonists out of a population of %.0f\n", antagonists, population);
    while(antagonists != 0)
    {
        for(int i = 0; i < server_clients->len; i++)
        {
            struct network_client* client = &g_array_index(server_clients, struct network_client, i);
            struct rdm_player* player = client->user_data;
            double player_chance = g_random_double_range(0.0,1.0);
            if(player_chance < 0.20)
            {
                if(traitors > 0)
                {
                    printf("rdm2[server]: %s is a traitor\n", client->client_name);
                    antagonist_set_objectives(&player->antagonist_data);
                    net_player_syncinfo(client->owner, player);
                    player->antagonist_data.type = RDM_ANTAGONIST_TRAITOR;
                    traitors--;
                }
            }
        }
        antagonists = traitors;
    }
}

char* antagonist_name(enum antagonist_type type)
{
    switch(type)
    {
        case RDM_ANTAGONIST_TRAITOR:
            return "Traitor";
        case RDM_ANTAGONIST_SPY:
            return "Spy";
        default:
            return "Antagonist";
    }
}