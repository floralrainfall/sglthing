#include "script.h"
#include "io.h"
#include <stdlib.h>
#include <string.h>
#include "sglthing.h"
#include "world.h"
#include "ui.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// https://www.lua.org/manual/5.3/manual.html
static void *l_alloc (void *ud, void *ptr, size_t osize,
                                        size_t nsize) {
    (void)ud;  (void)osize;  /* not used */
    if (nsize == 0) {
        free(ptr);
        return NULL;
    }
    else
        return realloc(ptr, nsize);
}

struct script_system* script_init(char* file, struct world* world)
{
    struct script_system* new_system = malloc(sizeof(struct script_system));
    char path[255];
    int f = file_get_path(path,255,file);
    if(f != -1)
    {
        struct world* world;
        new_system->enabled = true;
        new_system->world = world;

        new_system->lua = lua_newstate(l_alloc, new_system);
        luaL_openlibs(new_system->lua);
        luaL_dofile(new_system->lua, path); 
    
    }
    else
    {
        printf("sglthing: script %s not found\n", file);
        new_system->enabled = false;
    }
    return new_system;
}

static void __script_push_vec2(struct script_system* system, vec2 vec)
{
    lua_createtable(system->lua, 0, 2);

    lua_pushstring(system->lua, "x");
    lua_pushnumber(system->lua, vec[0]);
    lua_settable(system->lua, -3);

    lua_pushstring(system->lua, "y");
    lua_pushnumber(system->lua, vec[1]);
    lua_settable(system->lua, -3);
}

static void __script_push_vec3(struct script_system* system, vec3 vec)
{
    lua_createtable(system->lua, 0, 3);

    lua_pushstring(system->lua, "x");
    lua_pushnumber(system->lua, vec[0]);
    lua_settable(system->lua, -3);

    lua_pushstring(system->lua, "y");
    lua_pushnumber(system->lua, vec[1]);
    lua_settable(system->lua, -3);

    lua_pushstring(system->lua, "z");
    lua_pushnumber(system->lua, vec[2]);
    lua_settable(system->lua, -3);
}

static void __script_push_vec4(struct script_system* system, vec4 vec)
{
    lua_createtable(system->lua, 0, 4);

    lua_pushstring(system->lua, "x");
    lua_pushnumber(system->lua, vec[0]);
    lua_settable(system->lua, -3);

    lua_pushstring(system->lua, "y");
    lua_pushnumber(system->lua, vec[1]);
    lua_settable(system->lua, -3);

    lua_pushstring(system->lua, "z");
    lua_pushnumber(system->lua, vec[2]);
    lua_settable(system->lua, -3);

    lua_pushstring(system->lua, "w");
    lua_pushnumber(system->lua, vec[3]);
    lua_settable(system->lua, -3);
}


static void __script_push_world(struct script_system* system)
{
    lua_createtable(system->lua, 0, 1);
    lua_pushstring(system->lua, "camera");
    lua_createtable(system->lua, 0, 2);
    lua_pushstring(system->lua, "position");
    __script_push_vec3(system, system->world->cam.position);
    lua_settable(system->lua, -3);
    lua_settable(system->lua, -3);

    lua_createtable(system->lua, 0, 2);


    lua_settable(system->lua, -3);
}

void script_frame(struct script_system* system)
{
    ASSERT2_S(system);
    if(!system->enabled)
        return;

    lua_getglobal(system->lua, "ScriptFrame");
    __script_push_world(system);
    if(lua_pcall(system->lua, 1, 0, 0) == 0)
    {

    }
}

void script_frame_render(struct script_system* system, bool xtra_pass)
{
    ASSERT2_S(system);    
    if(!system->enabled)
        return;

    lua_getglobal(system->lua, "ScriptFrameRender");
    if(lua_pcall(system->lua, 0, 0, 0) == 0)
    {
        
    }
}

void script_frame_ui(struct script_system* system)
{
    ASSERT2_S(system);    
    if(!system->enabled)
        return;

    lua_getglobal(system->lua, "ScriptFrameUI");
    if(lua_pcall(system->lua, 0, 0, 0) == 0)
    {
        
    }
}