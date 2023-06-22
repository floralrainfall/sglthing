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

void net_player_syncinfo2(struct network* network, struct rdm_player* player)
{

    struct network_packet _pak;
    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

    _pak.meta.packet_type = RDM_PACKET_PLAYER_STATUS;
    _pak.meta.acknowledge = true;
    _pak.meta.packet_size = sizeof(union rdm_packet_data);
    _data->player_status.urgent = false;
    _data->player_status.player_id = player->client->player_id;
    // TODO: This

    network_transmit_packet(network, player->client, &_pak);
}

void net_player_syncinfo(struct network* network, struct rdm_player* player)
{
    struct network_packet _pak;
    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

    _pak.meta.packet_type = RDM_PACKET_PLAYER_STATUS;
    _pak.meta.acknowledge = true;
    _pak.meta.packet_size = sizeof(union rdm_packet_data);
    _data->player_status.urgent = true;
    _data->player_status.health = player->health;
    _data->player_status.player_id = player->client->player_id;
    for(int i = 0; i < __WEAPON_MAX; i++)
        _data->player_status.weapon_ammos[i] = player->weapon_ammos[i];
    _data->player_status.antagonist_data = player->antagonist_data;

    network_transmit_packet(network, player->client, &_pak);
}

void net_player_damage(struct network* network, struct rdm_player* player, enum rdm_damage_type type, int damage, int attacker_id)
{
    float damage_f = (float)damage;

    player->health -= (int)roundf(damage_f);
    if(player->health <= 0)
    {
        struct network_packet _pak;
        union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

        _pak.meta.packet_type = RDM_PACKET_PLAYER_DEATH;
        _pak.meta.acknowledge = true;
        _pak.meta.packet_size = sizeof(union rdm_packet_data);
        _data->player_death.attacker_id = attacker_id;
        _data->player_death.type = type;
        _data->player_death.player_id = player->player_id;

        network_transmit_packet_all(network,  &_pak);

        // die
        player->health = 0;
    }
    net_player_syncinfo(network, player);
}

void net_player_moveto(struct network* network, struct rdm_player* player, vec3 position)
{
    struct network_packet _pak;
    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

    _pak.meta.packet_type = RDM_PACKET_UPDATE_POSITION;
    _pak.meta.acknowledge = false;
    _pak.meta.packet_size = sizeof(union rdm_packet_data);
    glm_vec3_copy(position, _data->update_position.position);
    glm_vec3_copy(position, player->replicated_position);
    glm_quat_copy(player->direction, _data->update_position.direction);
    _data->update_position.urgent = true;
    _data->update_position.player_id = player->client->player_id;

    network_transmit_packet_all(network, &_pak);
}

void net_send_chunk(struct network* network, struct network_client* client, int c_x, int c_y, int c_z, struct map_chunk* chunk)
{
    profiler_event("net_send_chunk");

    for(int x = 0; x < RENDER_CHUNK_SIZE; x++)
    {
        struct pending_packet pending;
        pending.packet = (struct network_packet*)malloc2(sizeof(struct network_packet_header) + sizeof(union rdm_packet_data));
        union rdm_packet_data* _data = (union rdm_packet_data*)&pending.packet->packet.data;

        pending.client = client;

        pending.packet->meta.packet_size = sizeof(_data->update_chunk);
        pending.packet->meta.packet_type = RDM_PACKET_UPDATE_CHUNK;
        pending.packet->meta.acknowledge = true;
        pending.giveup = network->time + 10.0;
        _data->update_chunk.chunk_x = c_x;
        _data->update_chunk.chunk_y = c_y;
        _data->update_chunk.chunk_z = c_z;        
        _data->update_chunk.chunk_data_x = x;

        memcpy(_data->update_chunk.chunk_data, chunk->x[x].y, RENDER_CHUNK_SIZE*RENDER_CHUNK_SIZE);

        g_array_append_val(server_state.chunk_packets_pending, pending);
        //network_transmit_packet(network, client, _pak);
    }
    
    server_state.next_pending = network->time + 0.01f;
    profiler_end();
}

void net_play_sound(struct network* network, struct rdm_player* player, char* snd_name, vec3 position)
{
    struct network_packet _pak;
    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

    _pak.meta.packet_type = RDM_PACKET_REPLICATE_SOUND;
    _pak.meta.packet_size = sizeof(union rdm_packet_data);
    _pak.meta.acknowledge = false;
    strncpy(_data->replicate_sound.sound_name, snd_name, 128);
    glm_vec3_copy(position, _data->replicate_sound.position);

    network_transmit_packet(network, player->client, &_pak);
}

void net_play_g_sound(struct network* network, char* snd_name, vec3 position)
{
    for(int i = 0; i < network->server_clients->len; i++)
        net_play_sound(network, (struct rdm_player*)g_array_index(network->server_clients, struct network_client, i).user_data, snd_name, position);
}

void net_sync_gamemode(struct network* network, struct gamemode_data* gamemode)
{
    struct network_packet _pak;
    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

    _pak.meta.packet_type = RDM_PACKET_UPDATE_GAMEMODE;
    _pak.meta.acknowledge = true;
    _pak.meta.packet_size = sizeof(union rdm_packet_data);  

    memcpy(&_data->update_gamemode.gm_data, gamemode, sizeof(struct gamemode_data));

    network_transmit_packet_all(network, &_pak);
}

static void net_info_player(struct network* network, struct network_client* new_client, struct network_client* old_client)
{
    struct rdm_player* old_player = (struct rdm_player*)old_client->user_data;
    if(old_player)
    {
        struct network_packet _pak;
        union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

        _pak.meta.packet_type = RDM_PACKET_UPDATE_POSITION;
        _pak.meta.acknowledge = false;
        _pak.meta.packet_size = sizeof(union rdm_packet_data);  
        glm_vec3_copy(old_player->replicated_position, _data->update_position.position);
        glm_quat_copy(old_player->direction, _data->update_position.direction);
        _data->update_position.player_id = old_client->player_id;

        network_transmit_packet(network, new_client, &_pak);
    }
}

static void net_del_player(struct network* network, struct network_client* client)
{
    free2(client->user_data);
}

static void net_new_player(struct network* network, struct network_client* client)
{
    printf("rdm2[%s]: new player %s\n", net_name_manager(network), client->client_name);
    client->user_data = (struct rdm_player*)malloc2(sizeof(struct rdm_player));
    struct rdm_player* player = (struct rdm_player*)client->user_data;
    player->client = client;

    glm_vec3_zero(player->position);
    player->position[0] = 1.f;
    player->position[1] = 16.f;
    player->position[2] = 1.f;
    glm_vec3_copy(player->position, player->replicated_position);
    glm_quat_identity(player->direction);
    for(int i = 0; i < 9; i++)
        player->weapon_hotbar[i] = -1;
    for(int i = 0; i < sizeof(player->inventory) / sizeof(int); i++)
        player->inventory[i] = -1;
    player->active_hotbar_id = 0;
    player->active_weapon_type = WEAPON_SHOVEL;
    player->inventory[0] = WEAPON_SHOVEL;
    player->inventory[1] = WEAPON_BLOCK;
    player->inventory[2] = WEAPON_AK47;
    player->weapon_hotbar[0] = 0;
    player->weapon_hotbar[1] = 1;
    player->weapon_hotbar[2] = 2;
    player->weapon_hotbar[3] = 2;
    player->weapon_hotbar[4] = 2;
    player->weapon_hotbar[5] = 2;
    player->weapon_hotbar[6] = 2;
    player->team = TEAM_NEUTRAL;
    player->health = 2;
    player->max_health = 3;
    player->primary_next_fire = 0.f;
    player->secondary_next_fire = 0.f;
    for(int i = 0; i < __WEAPON_MAX; i++)
        player->weapon_ammos[i] = -1;
    player->weapon_ammos[WEAPON_AK47] = 200;
    player->weapon_ammos[WEAPON_SHOVEL] = -1;
    player->weapon_ammos[WEAPON_BLOCK] = 60;
    player->weapon_block_color = MAP_PAL(2,2,2);
    player->antagonist_data.type = RDM_ANTAGONIST_NEUTRAL;

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
        net_player_syncinfo(network, player);
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
        case PACKETTYPE_SERVERINFO:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                client_state.local_player_id = packet->packet.serverinfo.player_id;
                printf("rdm2[%s]: local player id = %i\n", net_name_manager(network), client_state.local_player_id);
            }
            return true;
        case RDM_PACKET_UPDATE_POSITION:
            if(network->mode == NETWORKMODE_SERVER)
            {
                bool collides = map_determine_collision_server(server_state.map_server, packet_data->update_position.position);
                if(!collides)
                {
                    float distance = glm_vec3_distance(remote_player->replicated_position, packet_data->update_position.position);
                    if(distance > 5.f)
                    {
                        struct network_packet _pak; // snap back
                        union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

                        _pak.meta.packet_type = RDM_PACKET_UPDATE_POSITION;
                        _pak.meta.acknowledge = false;
                        _pak.meta.packet_size = sizeof(union rdm_packet_data);  
                        glm_vec3_copy(remote_player->replicated_position, _data->update_position.position);
                        glm_quat_copy(remote_player->direction, _data->update_position.direction);
                        _data->update_position.urgent = true;
                        _data->update_position.player_id = client->player_id;

                        printf("rdm2[%s]: player %s going too fast (%fu in 1 packet)\n", net_name_manager(network), client->client_name, distance);

                        network_transmit_packet_all(network, &_pak);
                    }
                    else
                    {
                        glm_vec3_copy(packet_data->update_position.position, remote_player->replicated_position);       
                        glm_quat_copy(packet_data->update_position.direction, remote_player->direction);       

                        struct network_packet _pak; // snap back
                        union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;               
                        _pak.meta.packet_size = sizeof(union rdm_packet_data);  
                        _pak.meta.packet_type = RDM_PACKET_UPDATE_POSITION;
                        _pak.meta.acknowledge = false;
                        glm_vec3_copy(remote_player->replicated_position, _data->update_position.position);
                        glm_quat_copy(remote_player->direction, _data->update_position.direction);
                        _data->update_position.urgent = false;
                        _data->update_position.player_id = client->player_id;

                        network_transmit_packet_all(network, &_pak);
                    }
                }
                else
                {
                    struct network_packet _pak;
                    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

                    printf("rdm2[%s]: player %s colliding\n", net_name_manager(network), client->client_name);

                    _pak.meta.packet_type = RDM_PACKET_UPDATE_POSITION;
                    _pak.meta.acknowledge = false;
                    _pak.meta.packet_size = sizeof(union rdm_packet_data);
                    glm_vec3_copy(remote_player->replicated_position, _data->update_position.position);
                    glm_quat_copy(remote_player->direction, _data->update_position.direction);
                    _data->update_position.urgent = true;
                    _data->update_position.player_id = client->player_id;

                    network_transmit_packet_all(network, &_pak);
                }
            }
            else if(network->mode == NETWORKMODE_CLIENT)
            {
                struct network_client* moved = g_hash_table_lookup(network->players, &packet_data->update_position.player_id);
                if(!moved)
                {                   
                    printf("rdm2[%s]: RDM_PACKET_UPDATE_POSITION on unknown player id (%i)\n", net_name_manager(network), packet_data->update_position.player_id);
                    return false;
                }
                struct rdm_player* moved_player = (struct rdm_player*)moved->user_data;
                if(moved_player)
                {
                    glm_vec3_copy(packet_data->update_position.position, moved_player->replicated_position);
                    if(packet_data->update_position.player_id == client_state.local_player_id && !packet_data->update_position.urgent)
                        return false;
                    glm_vec3_copy(packet_data->update_position.position, moved_player->position);
                    glm_quat_copy(packet_data->update_position.direction, moved_player->direction);
                }
                else
                    printf("rdm2[%s]: RDM_PACKET_UPDATE_POSITION on unreg player id (%i)\n", net_name_manager(network), packet_data->update_position.player_id);
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
                if(server_state.map_server->map_ready == false)
                    return false;
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
        case RDM_PACKET_DESTROY_CHUNK:
            if(network->mode == NETWORKMODE_CLIENT)
            {

            }
            return false;
        case RDM_PACKET_UPDATE_GAMEMODE:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                memcpy(&client_state.gamemode, &packet_data->update_gamemode.gm_data, sizeof(struct gamemode_data));
                client_state.gamemode.server = false;
            }
            return false;
        case RDM_PACKET_GAME_START:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                printf("rdm2[%s]: mission started\n", net_name_manager(network));
                if(client_state.playing_music)
                    stop_snd(client_state.playing_music);
                client_state.playing_music = 0;
                play_snd2(client_state.roundstart_sound);

                map_client_clear(client_state.map_manager);
            }
            return false;
        case RDM_PACKET_WEAPON_FIRE:
            if(network->mode == NETWORKMODE_SERVER)
            {
                if(remote_player->weapon_ammos[net_player_get_weapon(remote_player)] == 0)
                    return false;
                bool fired = false;
                glm_quat_copy(packet_data->weapon_fire.direction, remote_player->direction);
                if(packet_data->weapon_fire.secondary)
                    fired = weapon_fire_secondary(network, client->player_id, true);
                else
                    fired = weapon_fire_primary(network, client->player_id, true);
                if(fired)
                {
                    if(remote_player->weapon_ammos[net_player_get_weapon(remote_player)] != -1)
                        remote_player->weapon_ammos[net_player_get_weapon(remote_player)]--;

                    net_player_syncinfo(network, remote_player);

                    struct network_packet _pak;
                    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;
                    
                    _pak.meta.packet_type = RDM_PACKET_WEAPON_FIRE;
                    _pak.meta.acknowledge = false;
                    _pak.meta.packet_size = sizeof(union rdm_packet_data);
                    _data->weapon_fire.player_id = client->player_id,
                    _data->weapon_fire.secondary = packet_data->weapon_fire.secondary;
                    glm_quat_copy(packet_data->weapon_fire.direction, _data->weapon_fire.direction);

                    network_transmit_packet_all(network, &_pak);
                }
            }
            else if(network->mode == NETWORKMODE_CLIENT)
            {
                if(packet_data->weapon_fire.secondary)
                    weapon_fire_secondary(network, client->player_id, false);
                else
                    weapon_fire_primary(network, client->player_id, false);
            }
            return false;
        case RDM_PACKET_PLAYER_STATUS:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                struct network_client* moved = g_hash_table_lookup(network->players, &packet_data->player_status.player_id);
                if(!moved)
                {                   
                    printf("rdm2[%s]: RDM_PACKET_PLAYER_STATUS on unknown player id (%i)\n", net_name_manager(network), packet_data->update_position.player_id);
                    return false;
                }
                struct rdm_player* moved_player = (struct rdm_player*)moved->user_data;
                if(moved_player)
                {
                    if(moved->player_id == client_state.local_player_id && !packet_data->player_status.urgent)
                    {
                        return false;
                    }
                    moved_player->health = packet_data->player_status.health;
                    for(int i = 0; i < __WEAPON_MAX; i++)
                        moved_player->weapon_ammos[i] = packet_data->player_status.weapon_ammos[i];                    
                    if(moved->player_id == client_state.local_player_id)
                    {
                        enum antagonist_type old_type = moved_player->antagonist_data.type;
                        moved_player->antagonist_data = packet_data->player_status.antagonist_data;
                        if(moved_player->antagonist_data.type != old_type)
                            client_state.server_motd_dismissed = false;
                    }
                }
                else
                    printf("rdm2[%s]: RDM_PACKET_PLAYER_STATUS on unreg player id (%i)\n", net_name_manager(network), packet_data->update_position.player_id);
            }
            return false;
        case RDM_PACKET_UPDATE_WEAPON:
            if(network->mode == NETWORKMODE_SERVER)
            {
                struct network_packet _pak;
                union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

                remote_player->active_hotbar_id = packet_data->update_weapon.hotbar_id;
                enum weapon_type weapon_type = net_player_get_weapon(remote_player);
                remote_player->active_weapon_type = weapon_type;
                
                _pak.meta.packet_type = RDM_PACKET_UPDATE_WEAPON;
                _pak.meta.acknowledge = false;
                _pak.meta.packet_size = sizeof(union rdm_packet_data);
                _data->update_weapon.player_id = client->player_id,
                _data->update_weapon.weapon = weapon_type;
                _data->update_weapon.hotbar_id = packet_data->update_weapon.hotbar_id;
                _data->update_weapon.block_color = packet_data->update_weapon.block_color;
                remote_player->weapon_block_color = packet_data->update_weapon.block_color;

                network_transmit_packet_all(network, &_pak);
            }
            else if(network->mode == NETWORKMODE_CLIENT)
            {
                struct network_client* moved = g_hash_table_lookup(network->players, &packet_data->update_weapon.player_id);
                if(!moved)
                {                   
                    printf("rdm2[%s]: RDM_PACKET_UPDATE_WEAPON on unknown player id (%i)\n", net_name_manager(network), packet_data->update_position.player_id);
                    return false;
                }
                struct rdm_player* moved_player = (struct rdm_player*)moved->user_data;
                if(moved_player)
                {
                    moved_player->active_hotbar_id = packet_data->update_weapon.hotbar_id;
                    moved_player->active_weapon_type = packet_data->update_weapon.weapon;
                    moved_player->weapon_block_color = packet_data->update_weapon.block_color;
                }
                else
                    printf("rdm2[%s]: RDM_PACKET_UPDATE_WEAPON on unreg player id (%i)\n", net_name_manager(network), packet_data->update_position.player_id);
            }
            return false;
#ifndef HEADLESS
        case RDM_PACKET_REPLICATE_SOUND:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                struct snd* new_snd = play_snd(packet_data->replicate_sound.sound_name);
                if(new_snd)
                {
                    new_snd->auto_dealloc = true;
                    glm_vec3_copy(packet_data->replicate_sound.position, new_snd->sound_position);
                    new_snd->sound_3d = packet_data->replicate_sound.sound_3d;
                    new_snd->camera_position = &client_state.local_player->position;
                }
                else
                    printf("rdm2[%s]: RDM_PACKET_REPLICATE_SOUND on non existent song (%s)\n", net_name_manager(network), packet_data->replicate_sound.sound_name);
            }
            return false;
#endif
        case RDM_PACKET_PLAYER_DEATH:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                
            }
            return false;
        default:
            return true;
    }
}

static void net_tick(struct network* network)
{
    struct world* world = network->client.user_data;
    if(server_state.chunk_packets_pending->len != 0)
    {
        profiler_event("net: send queued packets");
        struct pending_packet packet = g_array_index(server_state.chunk_packets_pending, struct pending_packet, 0);
        if(packet.giveup < network->time)
        {
            g_array_remove_index(server_state.chunk_packets_pending, 0);
        }
        else
        {
            network_transmit_packet(network, packet.client, packet.packet);
            g_array_remove_index(server_state.chunk_packets_pending, 0);
            free2(packet.packet);
        }
        profiler_end();
    }

    // packet limit
    if(server_state.chunk_packets_pending->len > 600) 
    {
        printf("rdm2[server]: clearing chunk packet queue (%i packets in queue)\n",server_state.chunk_packets_pending->len);
        while(server_state.chunk_packets_pending->len != 0)
        {
            struct pending_packet packet = g_array_index(server_state.chunk_packets_pending, struct pending_packet, 0);
            free2(packet.packet);
            g_array_remove_index(server_state.chunk_packets_pending, 0);
        }
    }
        
    gamemode_frame(&server_state.gamemode, world);
}

void net_init(struct world* world)
{
    world->client.receive_packet_callback = net_receive_packet;
    world->client.new_player_callback = net_new_player;
    world->client.del_player_callback = net_del_player;

    world->server.receive_packet_callback = net_receive_packet;
    world->server.new_player_callback = net_new_player;
    world->server.del_player_callback = net_del_player;
    world->server.old_player_add_callback = net_info_player;
    world->server.network_tick_callback = net_tick;
    world->server.client.user_data = (void*)world;
}

enum weapon_type net_player_get_weapon(struct rdm_player* player)
{
    return player->inventory[player->weapon_hotbar[player->active_hotbar_id]];
}