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

    if(network->mode == NETWORKMODE_CLIENT)
    {
        if(client->player_id == client_state.local_player_id)
        {   
            printf("rdm2[%s]: found local player\n", net_name_manager(network));
            client_state.local_player = player;
        }
    }
}

static bool net_receive_packet(struct network* network, struct network_client* client, struct network_packet packet)
{
    struct rdm_player* remote_player = (struct rdm_player*)client->user_data;
    union rdm_packet_data* packet_data = (union rdm_packet_data*)&packet.packet;
    if(!remote_player) // ignore from players without remote player
        return true;
    switch(packet.meta.packet_type)
    {
        case PACKETTYPE_SERVERINFO:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                client_state.local_player_id = packet.packet.serverinfo.player_id;
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