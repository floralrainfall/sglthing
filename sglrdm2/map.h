#ifndef MAP_H
#define MAP_H

#define RENDER_CHUNK_SIZE 16
#define MAP_SIZE 64
#define CUBE_SIZE 1
// [0-3],[0-3],[0-3]
#define MAP_PAL(r,g,b) r * 16 + g * 4 + b

#include <cglm/cglm.h>
#include <sglthing/world.h>
#include <sglthing/model.h>
#include "perlin.h"
#include <sglthing/prof.h>

struct map_chunk
{
    struct {
        struct {
            unsigned char z[RENDER_CHUNK_SIZE];
        } y[RENDER_CHUNK_SIZE];
    } x[RENDER_CHUNK_SIZE];
};

struct map_manager
{
    int map_x; // in center
    int map_y;
    int map_request_range;
    
    GArray* chunk_list;

    struct model* cube;
    int cube_program;
    int cube_program_light;
    int cube_texture;

    double next_map_rq;
};


struct map_server
{
    struct 
    {
        struct map_chunk chunk_y[MAP_SIZE];
    } chunk_x[MAP_SIZE];
    int map_seed;
    bool map_ready;
};

void map_server_init(struct map_server* map);

void map_init(struct map_manager* map);
void map_render_chunks(struct world* world, struct map_manager* map);
void map_update_chunks(struct map_manager* map, struct world* world);
void map_update_chunk(struct map_manager* map, int c_x, int c_y, int c_z, int d_x, unsigned char* chunk_data);
void map_color_to_rgb(unsigned char color_id, vec3 output);

bool map_determine_collision_client(struct map_manager* map, vec3 position);
bool map_determine_collision_server(struct map_server* map, vec3 position);

#endif