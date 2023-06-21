#include "weapons.h"
#include "rdm_net.h"
#include "ray.h"
#include <sglthing/sglthing.h>
#include "rdm2.h"

// weapon_fire_x, true = weapon actually went, false = weapon didnt fire

bool weapon_fire_primary(struct network* network, int player_id, bool server)
{                
    struct network_client* client = network_get_player(network, player_id);

    if(!client)
        return false;

    vec3 eye_position;
    struct ray_cast_info ray;
    struct rdm_player* player = (struct rdm_player*)client->user_data;
    enum weapon_type weapon_type = player->active_weapon_type;

    if(server && player)
    {
        if(player->primary_next_fire > network->time)
            return false;

        glm_vec3_copy(player->replicated_position, eye_position);
        eye_position[1] += 0.75;

        switch(weapon_type)
        {
            case WEAPON_HOLSTER:
                break;
            case WEAPON_AK47:
                ray = ray_cast(network, eye_position, player->direction, 100.f, client->player_id, false, false);
                net_play_g_sound(network, "rdm2/sound/ak47_shoot.ogg", player->position);
                if(ray.player)
                {
                    // damage player
                    net_player_damage(network, ray.player, RDM_DAMAGE_AK47, 10, client->player_id);                
                    net_play_g_sound(network, "rdm2/sound/ouch.ogg", player->position);
                }
                else if(ray.object == RAYCAST_VOXEL)
                {
                    net_play_g_sound(network, "rdm2/sound/ricochet_ground.ogg", ray.position);
                }
                player->primary_next_fire = network->time + 0.5;
                break;
            case WEAPON_BLOCK:
                eye_position[1] += 0.50; // magical number
                ray = ray_cast(network, eye_position, player->direction, 16.f, client->player_id, false, true);
                player->primary_next_fire = network->time + 0.5;
                if(ray.object == RAYCAST_VOXEL)
                {
                    int v_x = ray.voxel_x;
                    int v_y = ray.voxel_y;
                    int v_z = ray.voxel_z;

                    int c_x = ray.chunk_x;
                    int c_y = ray.chunk_y;
                    int c_z = ray.chunk_z;
                    
                    if(server_state.map_server->chunk_x[c_x].chunk_y[c_z].x[v_x].y[v_y].z[v_z] != 0)
                        return false;

                    if(ray.chunk_x < 0)
                        break;
                    if(ray.chunk_x > MAP_SIZE)
                        break;
                    if(ray.chunk_y != 0)
                        break;
                    if(ray.chunk_z < 0)
                        break;
                    if(ray.chunk_z > MAP_SIZE)
                        break;

                    server_state.map_server->chunk_x[c_x].chunk_y[c_z].x[v_x].y[v_y].z[v_z] = player->weapon_block_color;

                    struct network_packet _pak;
                    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;                 
                    _pak.meta.packet_type = RDM_PACKET_UPDATE_CHUNK;
                    _pak.meta.acknowledge = false;
                    _pak.meta.packet_size = sizeof(union rdm_packet_data);
                    _data->update_chunk.chunk_x = c_x;
                    _data->update_chunk.chunk_y = c_y;
                    _data->update_chunk.chunk_z = c_z;
                    _data->update_chunk.chunk_data_x = v_x;
                    memcpy(_data->update_chunk.chunk_data, &server_state.map_server->chunk_x[c_x].chunk_y[c_z].x[v_x],RENDER_CHUNK_SIZE*RENDER_CHUNK_SIZE);
                    network_transmit_packet_all(network, &_pak);
                }
                else
                {
                    return false;
                }
                break;
            case WEAPON_SHOVEL:
                eye_position[1] += 0.50; // magical number
                ray = ray_cast(network, eye_position, player->direction, 16.f, client->player_id, false, false);
                if(ray.object == RAYCAST_VOXEL)
                {
                    if(ray.chunk_x < 0)
                        break;
                    if(ray.chunk_x > MAP_SIZE)
                        break;
                    if(ray.chunk_y != 0)
                        break;
                    if(ray.chunk_z < 0)
                        break;
                    if(ray.chunk_z > MAP_SIZE)
                        break;
                    server_state.map_server->chunk_x[ray.chunk_x].chunk_y[ray.chunk_z].x[ray.voxel_x].y[ray.voxel_y].z[ray.voxel_z] = 0;
                    struct network_packet _pak;
                    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;                 
                    _pak.meta.packet_type = RDM_PACKET_UPDATE_CHUNK;
                    _pak.meta.acknowledge = false;
                    _pak.meta.packet_size = sizeof(union rdm_packet_data);
                    _data->update_chunk.chunk_x = ray.chunk_x;
                    _data->update_chunk.chunk_y = ray.chunk_y;
                    _data->update_chunk.chunk_z = ray.chunk_z;
                    _data->update_chunk.chunk_data_x = ray.voxel_x;
                    memcpy(_data->update_chunk.chunk_data, &server_state.map_server->chunk_x[ray.chunk_x].chunk_y[ray.chunk_z].x[ray.voxel_x],RENDER_CHUNK_SIZE*RENDER_CHUNK_SIZE);
                    network_transmit_packet_all(network, &_pak);
                    player->weapon_ammos[WEAPON_BLOCK]++;
                }
                else if(ray.object == RAYCAST_PLAYER)
                {
                    if(ray.distance > 1.5)
                    {
                        net_player_damage(network, ray.player, RDM_DAMAGE_SHOVEL, 25, player->player_id);
                    }
                }
                player->primary_next_fire = network->time + 0.5;
                break;
        }

        return true;
    }
    else if(player)
    {
        if(player->primary_next_fire > network->time)
            return false;
                    

        switch(weapon_type)
        {
            case WEAPON_AK47:

                break;
            default:
                break;
        }

        return true;
    }

    return false;
}

bool weapon_fire_secondary(struct network* network, int player_id, bool server)
{
    return false;
}

void weapon_trigger_fire(struct world* world, bool secondary)
{
    struct network_packet _pak;
    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;
    
    _pak.meta.packet_type = RDM_PACKET_WEAPON_FIRE;
    _pak.meta.acknowledge = false;
    _pak.meta.packet_size = sizeof(union rdm_packet_data);
    _data->weapon_fire.player_id = client_state.local_player_id,
    _data->weapon_fire.secondary = secondary;
    if(client_state.local_player)
        glm_quat_copy(client_state.local_player->direction, _data->weapon_fire.direction);

    network_transmit_packet(&world->client, &world->client.client, &_pak);
}