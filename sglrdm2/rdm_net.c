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

static void net_send_chunk(struct network* network, struct network_client* client, int c_x, int c_y, int c_z, struct map_chunk* chunk)
{
    struct network_packet _pak;
    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

    _pak.meta.packet_type = RDM_PACKET_UPDATE_CHUNK;
    _pak.meta.acknowledge = true;
    _data->update_chunk.chunk_x = c_x;
    _data->update_chunk.chunk_y = c_y;
    _data->update_chunk.chunk_z = c_z;

    for(int x = 0; x < RENDER_CHUNK_SIZE; x++)
        for(int y = 0; y < RENDER_CHUNK_SIZE; y++)
        {
            _data->update_chunk.chunk_data_x = x;
            _data->update_chunk.chunk_data_y = y;

            memcpy(_data->update_chunk.chunk_data, chunk->x[x].y[y].z, RENDER_CHUNK_SIZE);

            network_transmit_packet(network, client, _pak);
        }
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
    player->position[1] = 10.f;
    glm_vec3_zero(player->replicated_position);
    player->replicated_position[1] = 10.f;
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

            }
            else if(network->mode == NETWORKMODE_CLIENT)
            {

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
                    packet_data->update_chunk.chunk_data_y,

                    packet_data->update_chunk.chunk_data
                );
            }
            return false;
        case RDM_PACKET_REQUEST_CHUNK:
            if(network->mode == NETWORKMODE_SERVER)
            {
                if(packet_data->update_chunk.chunk_x >= MAP_SIZE)
                    return false;
                if(packet_data->update_chunk.chunk_y >= MAP_SIZE)
                    return false;
                if(packet_data->update_chunk.chunk_x < 0)
                    return false;
                if(packet_data->update_chunk.chunk_y < 0)
                    return false;
                net_send_chunk(network, client, packet_data->update_chunk.chunk_x, 0, packet_data->update_chunk.chunk_z, &server_state.map_server->chunk_x[packet_data->update_chunk.chunk_x].chunk_y[packet_data->update_chunk.chunk_z]);
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