#ifndef GUY_H
#define GUY_H

#include <cglm/cglm.h>
#include <glib-2.0/glib.h>
#include <sglthing/model.h>
#include <sglthing/world.h>
#include "civ.h"

enum __attribute__((packed)) job
{
    JOB_UNEMPLOYED,
    JOB_BUILDER,
};

enum __attribute__((packed)) guy_state
{
    GUY_STATE_IDLE,
    GUY_STATE_MOVING,
    GUY_STATE_ASSIST_BUILD,
};

struct __attribute__((packed)) guy
{
    char civ_id;
    enum guy_state state;
    int guy_id;
    vec2 position;
    vec2 wanted_position;
};
typedef struct guy guy_t;


void guy_render(struct world* world, guy_t guy, struct model* guy_mdl, int guy_prg, struct civ_info* civs);
void guy_ai_tick(guy_t* guy, struct civ_info* civs, GHashTable* building_table, float delta_time);
guy_t* guy_new(GHashTable* guy_table, guy_t guy, struct civ_info* civs);

#endif