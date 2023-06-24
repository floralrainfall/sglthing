#include "objective.h"
#include <stdio.h>
#include <sglthing/net.h>
#include "rdm_net.h"
#include "rdm2.h"

void objective_description(char* out, int len, struct objective objective)
{
    switch(objective.type)
    {
        case RDM_OBJECTIVE_KILL_PLAYER:
            struct network_client* moved = g_hash_table_lookup(client_state.network->players, &objective.objectives.kill.player_id);
            if(moved)
                snprintf(out, len, "Kill %s.", moved->client_name);
            else
                snprintf(out, len, "Bad player %i. We must kill them", objective.objectives.kill.player_id);
            break;
        case RDM_OBJECTIVE_STEAL_ITEM:
            snprintf(out, len, "Steal a (tmp: %i) and hold on to it for the entire round.", objective.objectives.steal.item_id);
            break;
        case RDM_OBJECTIVE_SURVIVE:
            snprintf(out, len, "Survive until the end of the round.");
            break;
        default:
            snprintf(out, len, "IDK, LOL!");
            break;
    }
}

#define TRAITOR_PERCENTAGE 1.0

static bool antagonist_eligible(struct rdm_player* src_player, int player_id)
{
    struct network_client* moved = g_hash_table_lookup(client_state.network->players, &player_id);
    if(!moved)
        return false;
    struct rdm_player* moved_player = (struct rdm_player*)moved->user_data;
    if(!moved_player)
        return false;
    if(src_player)
    {
        if(src_player->player_id == player_id)
            return false;
        if(src_player->team != moved_player->team) // traitors shouldnt 'betray' a team they are not on otherwise its not betrayal
            return false;
    }
    return true;
}

static void antagonist_random_objective(struct rdm_player* player, int antag_id)
{
    struct objective* objective = &player->antagonist_data.objectives[antag_id];
    objective->type = g_random_int_range(RDM_OBJECTIVE_KILL_PLAYER, __RDM_OBJECTIVE_MAX_RANDOM - 1);
    switch(objective->type)
    {
        case RDM_OBJECTIVE_KILL_PLAYER:
            objective->objectives.kill.player_id = g_random_int_range(0, server_state.network->server_clients->len);
            int antag_check_tries = 0;
            while(!antagonist_eligible(player, objective->objectives.kill.player_id))
            {
                if(antag_check_tries > 100)
                    break;
                objective->objectives.kill.player_id = g_random_int_range(0, server_state.network->server_clients->len);
                antag_check_tries++;                
            }
            break;
        case RDM_OBJECTIVE_STEAL_ITEM:
            objective->objectives.steal.item_id = 0;
            break;
    }
}

static void antagonist_set_objectives(struct rdm_player* player)
{
    player->antagonist_data.objective_count = g_random_int_range(2, MAX_OBJECTIVES);
    printf("rdm2[server]: player has %i objectives\n", player->antagonist_data.objective_count);
    for(int i = 0; i < player->antagonist_data.objective_count - 1; i++) // last objective is reserved for living
        antagonist_random_objective(player, i);
    player->antagonist_data.objectives[player->antagonist_data.objective_count - 1].type = RDM_OBJECTIVE_SURVIVE;
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
                    player->antagonist_data.type = RDM_ANTAGONIST_TRAITOR;
                    antagonist_set_objectives(player);
                    net_player_syncinfo(client->owner, player);
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