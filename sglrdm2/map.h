#ifndef MAP_H
#define MAP_H

#define MAP_MAX_TILES_Y 32
#define MAP_MAX_TILES_X 32

#include <cglm/cglm.h>

struct map_data
{
    int map_tile_id;
    int map_tile_path;
};

struct map_row
{
    struct map_data map_column[MAP_MAX_TILES_Y];
};

struct map
{
    struct map_row map_rows[MAP_MAX_TILES_X];
};

void map_generate(struct map* map);

#endif