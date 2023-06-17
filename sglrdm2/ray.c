#include "ray.h"
#include "map.h"
#include "rdm_net.h"
#include "rdm2.h"
#include <sglthing/sglthing.h>

#define RAY_FRACTION 0.25f

struct ray_cast_info ray_cast(struct network* network, vec3 starting_position, versor direction, double length, int ignore, bool client, bool side)
{
    struct ray_cast_info info;
    profiler_event("ray_cast");

    vec3 ray_position;
    vec4 forward = {0.0, 0.0, 0.0};

    mat4 rotation;
    vec4 front = {0.0,0.0,1.0,1.0};
    vec3 _forward;

    glm_vec3_copy(starting_position, ray_position);
    glm_quat_mat4(direction, rotation);
    glm_mat4_mulv(rotation, front, forward);

    ray_position[1] += client ? 0.5f : 0.0f;

    glm_vec3_mul(forward, (vec3){RAY_FRACTION,RAY_FRACTION,RAY_FRACTION}, _forward);

    double distance_traversed = 0.f;
    info.object = RAYCAST_NOTHING;
    info.player = 0;
    while(distance_traversed < length)
    {
        if(client ? map_determine_collision_client(client_state.map_manager, ray_position) : 
                    map_determine_collision_server(server_state.map_server, ray_position))
        {
            info.real_voxel_x = floorf((ray_position[0]+0.5f) / CUBE_SIZE);
            info.real_voxel_y = floorf((ray_position[1]) / CUBE_SIZE);
            info.real_voxel_z = floorf((ray_position[2]+0.5f) / CUBE_SIZE);

            info.chunk_x = floorf((float)info.real_voxel_x / RENDER_CHUNK_SIZE);
            info.chunk_y = floorf((float)info.real_voxel_y / RENDER_CHUNK_SIZE);
            info.chunk_z = floorf((float)info.real_voxel_z / RENDER_CHUNK_SIZE);

            info.voxel_x = info.real_voxel_x - (info.chunk_x * RENDER_CHUNK_SIZE);
            info.voxel_y = info.real_voxel_y - (info.chunk_y * RENDER_CHUNK_SIZE);
            info.voxel_z = info.real_voxel_z - (info.chunk_z * RENDER_CHUNK_SIZE);

            if(forward[0] > 0.5)
                info.voxel_hit_side = RAYCAST_HIT_RIGHT;
            else if(forward[0] < -0.5)
                info.voxel_hit_side = RAYCAST_HIT_LEFT;
            else if(forward[1] > 0.5)
                info.voxel_hit_side = RAYCAST_HIT_BOTTOM;
            else if(forward[1] < -0.5)
                info.voxel_hit_side = RAYCAST_HIT_TOP;
            else if(forward[2] > 0.5)
                info.voxel_hit_side = RAYCAST_HIT_FORWARD;
            else if(forward[2] < -0.5)
                info.voxel_hit_side = RAYCAST_HIT_BACK;

            if(side)
            {
                    switch(info.voxel_hit_side)
                    {
                        case RAYCAST_HIT_TOP:
                            info.voxel_y++;
                            info.real_voxel_y++;
                            break;
                        case RAYCAST_HIT_BOTTOM:
                            info.voxel_y--;
                            info.real_voxel_y--;
                            break;
                        case RAYCAST_HIT_BACK:
                            info.voxel_z++;
                            info.real_voxel_y++;
                            break;
                        case RAYCAST_HIT_FORWARD:
                            info.voxel_z--;
                            info.real_voxel_z--;
                            break;
                        case RAYCAST_HIT_LEFT:
                            info.voxel_x++;
                            info.real_voxel_x++;
                            break;
                        case RAYCAST_HIT_RIGHT:
                            info.voxel_x--;
                            info.real_voxel_x--;
                            break;
                    }



#define C_SZ(x)                                       \
    if(info.voxel_ ## x == -1) { info.chunk_ ## x--; info.voxel_ ## x = RENDER_CHUNK_SIZE-1; } \
    if(info.voxel_ ## x == RENDER_CHUNK_SIZE) { info.chunk_ ## x++; info.voxel_ ## x = 0; }

                    C_SZ(x);
                    C_SZ(y);
                    C_SZ(z);
            }

            info.object = RAYCAST_VOXEL;
            break;
        }
        else {
            if(!client)
            {
                for(int i = 0; i < network->server_clients->len; i++)
                {
                    struct network_client* client = &g_array_index(network->server_clients, struct network_client, i);
                    struct rdm_player* player = (struct rdm_player*)client->user_data;
                    if(client && player && client->player_id != ignore)
                    {
                        // FIXME: distance based, should be a cuboid around player/hitboxes on model
                        if(glm_vec3_distance(ray_position, player->replicated_position) < 1.f)
                        {
                            info.object = RAYCAST_PLAYER;
                            info.player = player;
                            break;
                        }
                    }
                }
            }
        }
        if(info.object != RAYCAST_NOTHING)
            break;

        glm_vec3_add(ray_position, _forward, ray_position);
        distance_traversed += RAY_FRACTION;
    }

    info.distance = distance_traversed;
    glm_vec3_copy(ray_position, info.position);

    profiler_end();
    return info;
}