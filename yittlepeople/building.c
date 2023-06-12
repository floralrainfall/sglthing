#include "building.h"
#include <sglthing/memory.h>

static int last_building_id = 0;

building_t* building_construct(GHashTable* building_table, int civ_id, enum building_type type, vec2 position)
{
    building_t* building_ref = (building_t*)malloc2(sizeof(building_t));
    int building_id = last_building_id++;
    building_ref->civ_id = civ_id;
    building_ref->position[0] = position[0];
    building_ref->position[1] = position[1];
    building_ref->building_id = building_id;
    building_ref->building_progress_points_max = 100;
    building_ref->building_progress_points = 0;
    building_ref->building_inprogress = true;
    building_ref->type = type;
    g_hash_table_insert(building_table, &building_id, building_ref);
    return building_ref;
}