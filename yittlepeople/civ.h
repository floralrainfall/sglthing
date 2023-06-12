#ifndef CIV_H
#define CIV_H
#include <glib-2.0/glib.h>
#include "building.h"

struct civ_info
{
    int civ_population;
    int current_building_id;
};

struct civ_demands
{
    int food;
};

void civ_get_demands(int civ_id, struct civ_info* civs, GHashTable* guy_table, GHashTable* building_table);

#endif