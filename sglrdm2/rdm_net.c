#include "rdm_net.h"
#include "rdm2.h"

static char* net_name_manager(struct network* network)
{
    switch(network->mode)
    {
        case NETWORKMODE_SERVER:
            return "server";
        case NETWORKMODE_CLIENT:
            return "client";
    }
    return "unknown";
}

void net_send_chunk(struct network* network, struct network_client* client, int c_x, int c_y, int c_z, struct map_chunk* chunk)
{
    profiler_event("net_send_chunk");
    struct network_packet _pak;
    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

    _pak.meta.packet_type = RDM_PACKET_UPDATE_CHUNK;
    _pak.meta.acknowledge = true;
    _data->update_chunk.chunk_x = c_x;
    _data->update_chunk.chunk_y = c_y;
    _data->update_chunk.chunk_z = c_z;

    for(int x = 0; x < RENDER_CHUNK_SIZE; x++)
    {
        _data->update_chunk.chunk_data_x = x;

        memcpy(_data->update_chunk.chunk_data, chunk->x[x].y, RENDER_CHUNK_SIZE*RENDER_CHUNK_SIZE);

        network_transmit_packet(network, client, _pak);
    }
    profiler_end();
}

void net_sync_gamemode(struct network* network, struct gamemode_data* gamemode)
{
    struct network_packet _pak;
    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

    _pak.meta.packet_type = RDM_PACKET_UPDATE_GAMEMODE;
    _pak.meta.acknowledge = true;

    memcpy(&_data->update_gamemode.gm_data, gamemode, sizeof(struct gamemode_data));

    network_transmit_packet_all(network, _pak);
}

static void net_del_player(struct network* network, struct network_client* client)
{
    free(client->user_data);
}

static void net_new_player(struct network* network, struct network_client* client)
{
    printf("rdm2[%s]: new player %s\n", net_name_manager(network), client->client_name);
    client->user_data = (struct rdm_player*)malloc(sizeof(struct rdm_player));
    struct rdm_player* player = (struct rdm_player*)client->user_data;
    player->client = client;

    glm_vec3_zero(player->position);
    player->position[0] = 16.f;
    player->position[1] = 64.f;
    player->position[2] = 16.f;
    glm_vec3_copy(player->position, player->replicated_position);
    player->active_weapon = WEAPON_HOLSTER;
    player->team = TEAM_NEUTRAL;

    if(network->mode == NETWORKMODE_CLIENT)
    {
        if(client->player_id == client_state.local_player_id)
        {   
            printf("rdm2[%s]: found local player\n", net_name_manager(network));
            client_state.local_player = player;
        }
    }
    else if(network->mode == NETWORKMODE_SERVER)
    {
        gamemode_player_add(&server_state.gamemode, player);
    }
}

static bool net_receive_packet(struct network* network, struct network_client* client, struct network_packet* packet)
{
    struct rdm_player* remote_player = (struct rdm_player*)client->user_data;
    union rdm_packet_data* packet_data = (union rdm_packet_data*)&packet->packet;
    if(!remote_player && network->mode == NETWORKMODE_SERVER) // ignore from players without remote player
        return true;
    switch(packet->meta.packet_type)
    {
        case PACKETTYPE_PING:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                if(!client_state.local_player)
                    return false;
                float distance = glm_vec3_distance(client_state.local_player->position, client_state.local_player->replicated_position);
                if(distance > 0.1f)
                {
                    struct network_packet _pak; 
                    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

                    _pak.meta.packet_type = RDM_PACKET_UPDATE_POSITION;
                    _pak.meta.acknowledge = false;
                    glm_vec3_copy(client_state.local_player->position, _data->update_position.position);
                    _data->update_position.pitch = client_state.local_player->pitch;
                    _data->update_position.yaw = client_state.local_player->yaw;

                    network_transmit_packet(network, client, _pak);
                }
            }
            return true;
        case PACKETTYPE_SERVERINFO:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                client_state.local_player_id = packet->packet.serverinfo.player_id;
            }
            return true;
        case RDM_PACKET_UPDATE_POSITION:
            if(network->mode == NETWORKMODE_SERVER)
            {
                bool collides = map_determine_collision_server(server_state.map_server, packet_data->update_position.position);
                if(!collides)
                {
                    packet_data->update_position.player_id = remote_player->client->player_id;
                    float distance = glm_vec3_distance(remote_player->replicated_position, packet_data->update_position.position);
                    if(distance > 5.f)
                    {
                        struct network_packet _pak; // snap back
                        union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

                        _pak.meta.packet_type = RDM_PACKET_UPDATE_POSITION;
                        _pak.meta.acknowledge = false;
                        glm_vec3_copy(remote_player->replicated_position, _data->update_position.position);
                        _data->update_position.pitch = remote_player->pitch;
                        _data->update_position.yaw = remote_player->yaw;
                        _data->update_position.urgent = true;

                        printf("rdm2[%s]: player %s going too fast (%fu in 1 packet)\n", net_name_manager(network), client->client_name, distance);

                        network_transmit_packet_all(network, _pak);
                    }
                    else
                    {
                        glm_vec3_copy(packet_data->update_position.position, remote_player->replicated_position);
                        network_transmit_packet_all(network, *packet);
                    }
                }
                else
                {
                    struct network_packet _pak;
                    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

                    printf("rdm2[%s]: player %s colliding\n", net_name_manager(network), client->client_name);

                    _pak.meta.packet_type = RDM_PACKET_UPDATE_POSITION;
                    _pak.meta.acknowledge = false;
                    glm_vec3_copy(remote_player->replicated_position, _data->update_position.position);
                    _data->update_position.pitch = remote_player->pitch;
                    _data->update_position.yaw = remote_player->yaw;
                    _data->update_position.urgent = true;

                    network_transmit_packet_all(network, _pak);
                }
            }
            else if(network->mode == NETWORKMODE_CLIENT)
            {
                if(packet_data->update_position.player_id == client_state.local_player_id && !packet_data->update_position.urgent)
                    return false;
                struct network_client* moved = g_hash_table_lookup(network->players, &packet_data->update_position.player_id);
                struct rdm_player* moved_player = (struct rdm_player*)moved->user_data;
                if(moved_player)
                {
                    glm_vec3_copy(packet_data->update_position.position, moved_player->position);
                    glm_vec3_copy(packet_data->update_position.position, moved_player->replicated_position);
                    moved_player->pitch = packet_data->update_position.pitch;
                    moved_player->yaw = packet_data->update_position.yaw;
                }
                else
                    printf("rdm2[%s]: RDM_PACKET_UPDATE_POSITION on unknown player id (%i)\n", net_name_manager(network), packet_data->update_position.player_id);
            }
            return false;    
        case RDM_PACKET_UPDATE_CHUNK:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                map_update_chunk(client_state.map_manager, 
                    packet_data->update_chunk.chunk_x,
                    packet_data->update_chunk.chunk_y,
                    packet_data->update_chunk.chunk_z,

                    packet_data->update_chunk.chunk_data_x,

                    packet_data->update_chunk.chunk_data
                );
            }
            return false;
        case RDM_PACKET_REQUEST_CHUNK:
            if(network->mode == NETWORKMODE_SERVER)
            {
                if(packet_data->request_chunk.chunk_x >= MAP_SIZE)
                    return false;
                if(packet_data->request_chunk.chunk_z >= MAP_SIZE)
                    return false;
                if(packet_data->request_chunk.chunk_x < 0)
                    return false;
                if(packet_data->request_chunk.chunk_z < 0)
                    return false;
                net_send_chunk(network, client, packet_data->request_chunk.chunk_x, 0, packet_data->request_chunk.chunk_z, &server_state.map_server->chunk_x[packet_data->request_chunk.chunk_x].chunk_y[packet_data->request_chunk.chunk_z]);
            }
            return false;
        case RDM_PACKET_UPDATE_GAMEMODE:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                memcpy(&client_state.gamemode, &packet_data->update_gamemode.gm_data, sizeof(struct gamemode_data));
                client_state.gamemode.server = false;
            }
            return false;
        default:
            return true;
    }
}

void net_init(struct world* world)
{
    world->client.receive_packet_callback = net_receive_packet;
    world->client.new_player_callback = net_new_player;
    world->client.del_player_callback = net_del_player;

    world->server.receive_packet_callback = net_receive_packet;
    world->server.new_player_callback = net_new_player;
    world->server.del_player_callback = net_del_player;
}