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

    if(server && player)
    {
        if(player->primary_next_fire > network->time)
            return false;

        glm_vec3_copy(player->replicated_position, eye_position);
        eye_position[1] += 0.75;

        switch(player->active_weapon)
        {
            case WEAPON_HOLSTER:
                break;
            case WEAPON_AK47:
                ray = ray_cast(network, eye_position, player->direction, 100.f, client->player_id);
                net_play_g_sound(network, "rdm2/sound/ak47_shoot.ogg", player->position);
                if(ray.player)
                {
                    // damage player
                    net_player_damage(network, ray.player, 10, client->player_id);                
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
                ray = ray_cast(network, eye_position, player->direction, 16.f, client->player_id);
                player->primary_next_fire = network->time + 0.5;
                if(ray.object == RAYCAST_VOXEL)
                {
                    int v_x = ray.voxel_x;
                    int v_y = ray.voxel_y;
                    int v_z = ray.voxel_z;

                    int c_x = ray.chunk_x;
                    int c_y = ray.chunk_y;
                    int c_z = ray.chunk_z;
                    switch(ray.voxel_hit_side)
                    {
                        case RAYCAST_HIT_TOP:
                            v_y++;
                            break;
                        case RAYCAST_HIT_BOTTOM:
                            v_y--;
                            break;
                        case RAYCAST_HIT_BACK:
                            v_z++;
                            break;
                        case RAYCAST_HIT_FORWARD:
                            v_z--;
                            break;
                        case RAYCAST_HIT_LEFT:
                            v_x++;
                            break;
                        case RAYCAST_HIT_RIGHT:
                            v_x--;
                            break;
                    }

#define C_SZ(x)                                       \
    if(v_ ## x == -1) { c_ ## x--; v_ ## x = RENDER_CHUNK_SIZE-1; } \
    if(v_ ## x == RENDER_CHUNK_SIZE) { c_ ## x++; v_ ## x = 0; }

                    C_SZ(x);
                    C_SZ(y);
                    C_SZ(z);
                    
                    if(server_state.map_server->chunk_x[c_x].chunk_y[c_z].x[v_x].y[v_y].z[v_z] != 0)
                        return false;

                    switch(player->team)
                    {
                        case TEAM_NEUTRAL:
                            server_state.map_server->chunk_x[c_x].chunk_y[c_z].x[v_x].y[v_y].z[v_z] = 1;
                            break;
                        case TEAM_NA:
                            server_state.map_server->chunk_x[c_x].chunk_y[c_z].x[v_x].y[v_y].z[v_z] = 3;
                            break;
                        case TEAM_RED:
                            server_state.map_server->chunk_x[c_x].chunk_y[c_z].x[v_x].y[v_y].z[v_z] = 7;
                            break;
                        case TEAM_BLUE:
                            server_state.map_server->chunk_x[c_x].chunk_y[c_z].x[v_x].y[v_y].z[v_z] = 8;
                            break;
                        default:
                            break;
                    }

                    struct network_packet _pak;
                    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;                 
                    _pak.meta.packet_type = RDM_PACKET_UPDATE_CHUNK;
                    _pak.meta.acknowledge = false;
                    _data->update_chunk.chunk_x = c_x;
                    _data->update_chunk.chunk_y = c_y;
                    _data->update_chunk.chunk_z = c_z;
                    _data->update_chunk.chunk_data_x = v_x;
                    memcpy(_data->update_chunk.chunk_data, &server_state.map_server->chunk_x[c_x].chunk_y[c_z].x[v_x],RENDER_CHUNK_SIZE*RENDER_CHUNK_SIZE);
                    network_transmit_packet_all(network, _pak);
                }
                else
                {
                    return false;
                }
                break;
            case WEAPON_SHOVEL:
                eye_position[1] += 0.50; // magical number
                ray = ray_cast(network, eye_position, player->direction, 16.f, client->player_id);
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
                    _data->update_chunk.chunk_x = ray.chunk_x;
                    _data->update_chunk.chunk_y = ray.chunk_y;
                    _data->update_chunk.chunk_z = ray.chunk_z;
                    _data->update_chunk.chunk_data_x = ray.voxel_x;
                    memcpy(_data->update_chunk.chunk_data, &server_state.map_server->chunk_x[ray.chunk_x].chunk_y[ray.chunk_z].x[ray.voxel_x],RENDER_CHUNK_SIZE*RENDER_CHUNK_SIZE);
                    network_transmit_packet_all(network, _pak);
                    player->weapon_ammos[WEAPON_BLOCK]++;
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
                    

        switch(player->active_weapon)
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
    _data->weapon_fire.player_id = client_state.local_player_id,
    _data->weapon_fire.secondary = secondary;

    network_transmit_packet(&world->client, &world->client.client, _pak);
}