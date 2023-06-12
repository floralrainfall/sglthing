#ifndef YITTLEPEOPLE_H
#define YITTLEPEOPLE_H

#include <sglthing/model.h>
#include <sglthing/texture.h>
#include <glib-2.0/glib.h>
#include "civ.h"

struct yittlepeople_data
{
    GHashTable* guy_hash;
    GHashTable* building_hash;
    
    struct model* guy_model;
    int guy_shader;

    struct civ_info civs[255];
};

extern struct yittlepeople_data client_data;
extern struct yittlepeople_data server_data;

#endif