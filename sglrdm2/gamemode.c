#include "gamemode.h"
#include "rdm_net.h"

char* gamemode_name(enum rdm_gamemode gamemode)
{
    switch(gamemode)
    {
        case GAMEMODE_SECRET:
            return "secret";
        case GAMEMODE_EXTENDED:
            return "extended";
        case GAMEMODE_FREEFORALL:
            return "freeforall";
    }
    return "???";
}

void gamemode_init(struct gamemode_data* data, bool server)
{
    data->secret = false;
    data->gamemode = GAMEMODE_SECRET;
    data->started = false;
    data->gamemode_end = 120.f;
    data->server = server;
    data->gamemode_nextannouncement = 1.f;
    if(data->server)
        printf("rdm2: mission begins in %f seconds\n", data->gamemode_end);
}

void gamemode_frame(struct gamemode_data* data, struct world* world)
{
    profiler_event("gamemode_frame");
    data->network_time = data->server ? world->server.distributed_time : world->client.distributed_time;
    if(data->started)
    {

    }
    else
    {
        if(data->gamemode_end < data->network_time)
        {
            printf("rdm2: starting mission\n");
            data->started = true;
            data->gamemode_end = data->network_time + 1740.f; 

            switch(data->gamemode)
            {
                case GAMEMODE_SECRET:
                    if(data->server)
                    {
                        data->gamemode = g_random_int_range(GAMEMODE_FREEFORALL, __MAX_GAMEMODE - 1);                        
                        if(!data->secret)
                        {

                        }

                        data->last_team = TEAM_BLUE;
                        for(int i = 0; i < world->server.server_clients->len; i++)
                        {
                            struct network_client* client = &g_array_index(world->server.server_clients, struct network_client, i);
                            struct rdm_player* player = (struct rdm_player*)client->user_data;
                            
                            if(data->last_team == TEAM_RED)
                                data->last_team = TEAM_BLUE;
                            else if(data->last_team == TEAM_BLUE)
                                data->last_team = TEAM_RED;
                            player->team = data->last_team;
                        }
                    }
                    break;
            }
        }
    }

    if(data->gamemode_nextannouncement < world->time && data->server)
        net_sync_gamemode(&world->server, data);
    profiler_end();
}

void gamemode_player_add(struct gamemode_data* data, void* _player)
{
    struct rdm_player* player = (struct rdm_player*)_player;
    if(!data->started)
    {
        player->team = TEAM_SPECTATOR;
        return;
    }
    switch(data->gamemode)
    {
        case GAMEMODE_EXTENDED:
        case GAMEMODE_SECRET:
            if(data->last_team == TEAM_RED)
                data->last_team = TEAM_BLUE;
            else if(data->last_team == TEAM_BLUE)
                data->last_team = TEAM_RED;
            player->team = data->last_team;
            break;

        case GAMEMODE_FREEFORALL:
            player->team = TEAM_NA;
            break;

        default:
            player->team = TEAM_SPECTATOR;
            break;
    }
}