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
    struct transform player_transform;
    struct world* world;
    int player_transform_gc;
    char script_name[64];
    struct filesystem_archive* archive;

    s7_pointer s7_transform_ptr;
    int s7_transform_gc;
    s7_pointer s7_world_ptr;
    int s7_world_gc;
    s7_pointer s7_uidata_ptr;
    int s7_uidata_gc;

    bool enabled;
};

struct script_system* script_init(char* file, void* world)
{
#ifdef S7_DEBUGGING
    printf("sglthing: s7 is poisoned (S7_DEBUGGING)\n");
#endif

#ifndef S7_DISABLE
    struct world* _world = (struct world*)world;
    char rsc_path[64] = {0};
    char rsc_dir[64] = {0};
    struct script_system* system = malloc(sizeof(struct script_system));
    if(file_get_path(rsc_path,64,file)!=-1 && _world->enable_script)
    {
        char* last = strrchr(rsc_path, '/');
        if(last != NULL)
        {
            int len = strlen(rsc_path) - strlen(last);
            strncpy(rsc_dir, rsc_path, MIN(len,64));
        }

        printf("sglthing: starting script system on file %s (env: %s)\n", rsc_path, rsc_dir);
        system->scheme = s7_init();
        system->world = _world;
        strncpy(system->script_name, rsc_path, 64);

        s7_add_to_load_path(system->scheme, rsc_dir);
        sgls7_add_functions(system->scheme);
        
        system->player_transform.px = 0.f;
        system->player_transform.py = 0.f;
        system->player_transform.pz = 0.f;
        system->player_transform.rx = 0.f;
        system->player_transform.ry = 0.f;
        system->player_transform.rz = 0.f;
        system->player_transform.rw = 0.f;
        system->player_transform.sx = 0.f;
        system->player_transform.sy = 0.f;
        system->player_transform.sz = 0.f;

        system->s7_transform_ptr = s7_make_c_object(system->scheme, sgls7_transform_type(), &system->player_transform);
        system->s7_transform_gc = s7_gc_protect(system->scheme, system->s7_transform_ptr);
        s7_define_variable(system->scheme, "game-camera", system->s7_transform_ptr);

        s7_mark(system->s7_transform_ptr);

        system->s7_uidata_ptr = s7_make_c_pointer(system->scheme, system->world->ui);
        system->s7_uidata_gc = s7_gc_protect(system->scheme, system->s7_uidata_ptr);
        s7_define_variable(system->scheme, "game-ui", system->s7_uidata_ptr);

        s7_mark(system->s7_uidata_ptr);

        system->s7_world_ptr = s7_make_c_pointer(system->scheme, system->world);
        system->s7_world_gc = s7_gc_protect(system->scheme, system->s7_world_ptr);
        s7_define_variable(system->scheme, "game-world", system->s7_world_ptr);

        s7_mark(system->s7_world_ptr);

        s7_pointer code = s7_load(system->scheme, rsc_path);
        s7_pointer response;
        s7_eval(system->scheme, code, s7_rootlet(system->scheme));

        system->enabled = true;

        return system;
    }
    else
#endif
    {
        printf("sglthing: script %s not found\n", file);
        system->enabled = false;
        return system;
    }
}

void script_frame(struct script_system* system)
{
    ASSERT(system);
    if(!system->enabled)
        return;

    system->player_transform.px = system->world->cam.position[0];
    system->player_transform.py = system->world->cam.position[1];
    system->player_transform.pz = system->world->cam.position[2];

    system->player_transform.rx = system->world->cam.pitch;
    system->player_transform.ry = system->world->cam.yaw;

    s7_call(system->scheme, s7_name_to_value(system->scheme, "script-frame"), s7_nil(system->scheme));
    
    system->world->cam.position[0] = system->player_transform.px;
    system->world->cam.position[1] = system->player_transform.py;
    system->world->cam.position[2] = system->player_transform.pz;

    system->world->cam.pitch = system->player_transform.rx;
    system->world->cam.yaw = system->player_transform.ry;
}

void script_frame_render(struct script_system* system, bool xtra_pass)
{
    ASSERT(system);    
    if(!system->enabled)
        return;
        

    s7_call(system->scheme, s7_name_to_value(system->scheme, "script-frame-render"), s7_cons(system->scheme, s7_make_boolean(system->scheme, xtra_pass), s7_nil(system->scheme)));
}

void script_frame_ui(struct script_system* system)
{
    ASSERT(system);    
    if(!system->enabled)
        return;

    char sfui_dbg[256];
    snprintf(sfui_dbg,256,"s7 running script %s",system->script_name);
    ui_draw_text(system->world->ui, system->world->viewport[2]/3.f, 0.f, sfui_dbg, 1.f);
    s7_call(system->scheme, s7_name_to_value(system->scheme, "script-frame-ui"), s7_nil(system->scheme));
}

void* script_s7(struct script_system* system)
{
    return (void*)system->scheme;
}

bool script_enabled(struct script_system* system)
{
    return system->enabled;
}