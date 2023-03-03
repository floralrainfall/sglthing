#include "script.h"
#include "io.h"
#include "s7/s7.h"
#include <stdlib.h>
#include <string.h>
#include "sglthing.h"
#include "s7/script_functions.h"
#include "s7/transform.h"
#include "world.h"
#include "ui.h"

struct script_system
{
    s7_scheme* scheme;
    struct transform* player_transform;
    int player_transform_gc;
    char script_name[64];
};

struct script_system* script_init(char* file)
{
    char rsc_path[64] = {0};
    char rsc_dir[64] = {0};
    if(file_get_path(rsc_path,64,file))
    {
        char* last = strrchr(rsc_path, '/');
        if(last != NULL)
        {
            int len = strlen(rsc_path) - strlen(last);
            strncpy(rsc_dir, rsc_path, MIN(len,64));
        }

        printf("sglthing: starting script system on file %s (env: %s)\n", rsc_path, rsc_dir);
        struct script_system* system = malloc(sizeof(struct script_system));
        system->scheme = s7_init();
        strncpy(system->script_name, rsc_path, 64);

        s7_add_to_load_path(system->scheme, rsc_dir);
        sgls7_add_functions(system->scheme);
        
        system->player_transform = malloc(sizeof(struct transform));
        system->player_transform->px = 0.f;
        system->player_transform->py = 0.f;
        system->player_transform->pz = 0.f;
        system->player_transform->rx = 0.f;
        system->player_transform->ry = 0.f;
        system->player_transform->rz = 0.f;
        system->player_transform->rw = 0.f;
        system->player_transform->sx = 0.f;
        system->player_transform->sy = 0.f;
        system->player_transform->sz = 0.f;

        s7_pointer code = s7_load(system->scheme, rsc_path);
        s7_pointer response;
        s7_eval(system->scheme, code, s7_rootlet(system->scheme));

        return system;
    }
    else
    {
        printf("sglthing: script %s not found\n", file);
        return NULL;
    }
}

void script_frame(void* world, struct script_system* system)
{
    ASSERT(system);
    s7_gc_on(system->scheme, false);
    struct world* actual_world = (struct world*)world;

    system->player_transform->px = actual_world->cam.position[0];
    system->player_transform->py = actual_world->cam.position[1];
    system->player_transform->pz = actual_world->cam.position[2];

    system->player_transform->rx = actual_world->cam.pitch;
    system->player_transform->ry = actual_world->cam.yaw;

    struct transform* new_transform = (struct transform*)malloc(sizeof(struct transform));
    memcpy(new_transform, system->player_transform, sizeof(struct transform));
    s7_pointer transform = s7_make_c_object(system->scheme, sgls7_transform_type(), new_transform);     
    s7_int gc_i = s7_gc_protect(system->scheme,transform);   
    s7_call(system->scheme, s7_name_to_value(system->scheme, "script-frame"),   s7_cons(system->scheme, s7_make_c_pointer(system->scheme, world), 
                                                                                s7_cons(system->scheme, transform, s7_nil(system->scheme))));
    s7_gc_unprotect_at(system->scheme,gc_i);
    

    actual_world->cam.position[0] = new_transform->px;
    actual_world->cam.position[1] = new_transform->py;
    actual_world->cam.position[2] = new_transform->pz;

    actual_world->cam.pitch = new_transform->rx;
    actual_world->cam.yaw = new_transform->ry;
    s7_gc_on(system->scheme, true);
}

void script_frame_render(void* world, struct script_system* system, bool xtra_pass)
{
    ASSERT(system);
    s7_gc_on(system->scheme, false);
    s7_call(system->scheme, s7_name_to_value(system->scheme, "script-frame-render"), s7_cons(system->scheme, s7_make_c_pointer(system->scheme, world), s7_cons(system->scheme, s7_make_boolean(system->scheme, xtra_pass), s7_nil(system->scheme))));
    s7_gc_on(system->scheme, true);
}

void script_frame_ui(void* world, struct script_system* system)
{
    ASSERT(system);
    struct world* actual_world = (struct world*)world;
    char sfui_dbg[256];
    snprintf(sfui_dbg,256,"s7 running script %s",system->script_name);
    ui_draw_text(actual_world->ui, actual_world->viewport[2]/3.f, 0.f, sfui_dbg, 1.f);
    s7_call(system->scheme, s7_name_to_value(system->scheme, "script-frame-ui"), s7_cons(system->scheme, s7_make_c_pointer(system->scheme, world), s7_nil(system->scheme)));
}