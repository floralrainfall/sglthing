#ifndef BUILDING_H
#define BUILDING_H
#include <glib-2.0/glib.h>
#include <cglm/cglm.h>
#include <stdbool.h>

enum building_type
{
    BUILDING_HOUSE,
    BUILDING_TREE,
};

struct building
{
    enum building_type type;
    int civ_id;
    int building_id;
    vec2 position;
    int building_progress_points;
    int building_progress_points_max;
    bool building_inprogress;
};

typedef struct building building_t;

building_t* building_construct(GHashTable* building_table, int civ_id, enum building_type type, vec2 position);

#endif