#ifndef RAY_H
#define RAY_H

#include <cglm/cglm.h>
#include <sglthing/net.h>

enum cast_object
{
    RAYCAST_NOTHING,
    RAYCAST_VOXEL,
    RAYCAST_PLAYER,
};

struct ray_cast_info
{
    enum cast_object object;

    int voxel_x; 
    int voxel_y; 
    int voxel_z; 
    
    int chunk_x;
    int chunk_y;
    int chunk_z;

    int real_voxel_x;
    int real_voxel_y;
    int real_voxel_z;

    enum {
        RAYCAST_HIT_TOP,
        RAYCAST_HIT_BOTTOM,
        RAYCAST_HIT_LEFT,
        RAYCAST_HIT_RIGHT,
        RAYCAST_HIT_FORWARD,
        RAYCAST_HIT_BACK,
    } voxel_hit_side;

    vec3 position;
    double distance;

    struct rdm_player* player;
    int player_ignore;
};

struct ray_cast_info ray_cast(struct network* network, vec3 starting_position, versor direction, double length, int ignore, bool client, bool voxel_side);

#endif