#include "gamemode.h"
#include "rdm_net.h"
#include "rdm2.h"
#include "objective.h"

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
    data->gamemode_end = 10.f;
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
            data->started = true;
            data->gamemode_end = data->network_time + 1740.f; 

            if(data->server)
            {
                printf("rdm2: starting mission\n");
                struct network_packet _pak;
                union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

                _pak.meta.packet_size = 0;
                _pak.meta.packet_type = RDM_PACKET_GAME_START;
                _pak.meta.acknowledge = false;

                network_transmit_packet_all(&world->server, &_pak);

                server_state.map_server->map_seed = g_random_int_range(0,99999);
                printf("rdm2: using seed %i\n", server_state.map_server->map_seed);
                map_server_init(server_state.map_server);
            }

            switch(data->gamemode)
            {
                case GAMEMODE_SECRET:
                    if(data->server)
                    {
                        data->last_team = TEAM_BLUE;
                        for(int i = 0; i < world->server.server_clients->len; i++)
                        {
                            struct network_client* client = &g_array_index(world->server.server_clients, struct network_client, i);
                            struct rdm_player* player = (struct rdm_player*)client->user_data;
                            
                            if(data->last_team == TEAM_RED)
                            {
                                net_player_moveto(&world->server, player, (vec3){RENDER_CHUNK_SIZE*CUBE_SIZE*MAP_SIZE-(RENDER_CHUNK_SIZE*CUBE_SIZE/2),32.f,RENDER_CHUNK_SIZE*CUBE_SIZE*MAP_SIZE-(RENDER_CHUNK_SIZE*CUBE_SIZE/2)});
                                data->last_team = TEAM_BLUE;
                            }
                            else if(data->last_team == TEAM_BLUE)
                            {
                                net_player_moveto(&world->server, player, (vec3){RENDER_CHUNK_SIZE*CUBE_SIZE/2,32.f,RENDER_CHUNK_SIZE*CUBE_SIZE/2});
                                data->last_team = TEAM_RED;
                            }

                            player->team = data->last_team;
                        }

                        antagonist_select(world->server.server_clients);
                    }
                    break;
                case GAMEMODE_FREEFORALL:
                    if(data->server)
                    {
                        for(int i = 0; i < world->server.server_clients->len; i++)
                        {
                            struct network_client* client = &g_array_index(world->server.server_clients, struct network_client, i);
                            struct rdm_player* player = (struct rdm_player*)client->user_data;
                            net_player_moveto(&world->server, player, (vec3){
                                g_random_double_range(1, RENDER_CHUNK_SIZE*CUBE_SIZE*MAP_SIZE-1),
                                32.f,
                                g_random_double_range(1, RENDER_CHUNK_SIZE*CUBE_SIZE*MAP_SIZE-1)
                            });
                            player->team = TEAM_NA;
                        }
                    }
                    break;
            }
        }
    }

    if(data->gamemode_nextannouncement < world->time && data->server)
    {
        net_sync_gamemode(&world->server, data);
        data->gamemode_nextannouncement += world->time + 0.5f;
    }
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
            {
                data->last_team = TEAM_BLUE;
            }
            else if(data->last_team == TEAM_BLUE)
            {
                data->last_team = TEAM_RED;
            }
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