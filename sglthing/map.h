#ifndef MAP_H
#define MAP_H

#define MAP_SIZE 16
#define MAP_MESHES 7

#include "light.h"

struct world;

struct map {
    int map_data[MAP_SIZE][MAP_SIZE];
    int map_textures[MAP_SIZE][MAP_SIZE];

    struct model* map_meshes[MAP_MESHES];
    struct light_area* area;
};

void new_map(struct world* world, struct map* map);
void draw_map(struct world* world, struct map* map, int shader);

#endif