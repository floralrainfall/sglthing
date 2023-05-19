#ifndef MAP_H
#define MAP_H

#define MAP_CHUNK_SIZE 8

#define MAP_MAX_TILES_Y 8
#define MAP_MAX_TILES_X 8

#define MAP_STORAGE_SIZE 3

#include <cglm/cglm.h>
#include <sglthing/world.h>
#include <sglthing/model.h>

struct map_manager
{
    int map_x; // in center
    int map_y;
    
    struct model* cube;
    int cube_program;
    int cube_texture;
};

void map_init(struct map_manager* map);
void map_render_chunks(struct world* world, struct map_manager* map);

#endif