#ifndef MAP_H
#define MAP_H

#define RENDER_CHUNK_SIZE 8
#define RENDER_CHUNK_SIZE_Y 8
#define MAP_SIZE 14
#define CUBE_SIZE 1
// [0-3],[0-3],[0-3]
#define MAP_PAL(r,g,b) r * 16 + g * 4 + b

#include <cglm/cglm.h>
#include <sglthing/world.h>
#include <sglthing/model.h>
#include "perlin.h"
#include <sglthing/prof.h>

enum chunk_attr
{
    CHUNK_RED_SPAWN,
    CHUNK_BLUE_SPAWN,
    CHUNK_NEUTRAL_SPAWN,
    CHUNK_CITY,
    CHUNK_HIGHWAY,
    CHUNK_TERRAIN,
    CHUNK_FLAT,
};

enum block_type
{
    BLOCK_AIR,
    BLOCK_CONCRETE,
    BLOCK_BRICKS,
    BLOCK_SKYSCRAPERWALL,
    BLOCK_REDSPAWNBLOCK,
    BLOCK_BLUESPAWNBLOCK,
    BLOCK_NEUTRALSPAWNBLOCK,
    BLOCK_DIRT,
    BLOCK_GRASS,
    BLOCK_GLASS,
} __attribute__((packed));

struct map_chunk
{
    struct {
        struct {
            enum block_type z[RENDER_CHUNK_SIZE];
        } y[RENDER_CHUNK_SIZE_Y];
    } x[RENDER_CHUNK_SIZE];
    enum chunk_attr attr;
};

struct map_manager
{
    int map_x; // in center
    int map_y;
    int map_request_range;
    int map_render_range;
    int map_dealloc_range;
    int map_texture_atlas;

    GArray* chunk_list;

    struct model* cube;
    int cube_program;
    int cube_program_light;
    int cube_texture;

    double next_map_rq;
    int map_data_wanted;
    int chunk_limit;
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
void map_update_chunk(struct map_manager* map, int c_x, int c_y, int c_z, int d_x, enum block_type* chunk_data);
void map_color_to_rgb(unsigned char color_id, vec3 output);

void map_client_clear(struct map_manager* map);

bool map_determine_collision_client(struct map_manager* map, vec3 position);
bool map_determine_collision_server(struct map_server* map, vec3 position);

#endif