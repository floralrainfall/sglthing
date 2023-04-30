#ifndef MAP_H
#define MAP_H

#include "yaal.h"
#include "util.h"
bool determine_tile_collision(struct map_tile_data tile);
void map_render(struct world* world, struct map_tile_data map[MAP_SIZE_MAX_X][MAP_SIZE_MAX_Y]);

#endif