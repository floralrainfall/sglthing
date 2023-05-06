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

#include "lua/l_functions.h"
#include "lua/l_vector.h"

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
        new_system->enabled = true;
        new_system->world = world;

        new_system->lua = lua_newstate(l_alloc, new_system);
        luaL_openlibs(new_system->lua);
        l_functions_register(new_system->lua);
    
        luaL_dofile(new_system->lua, path);
    }
    else
    {
        printf("sglthing: script %s not found\n", file);
        new_system->enabled = false;
    }
    return new_system;
}

void script_frame(struct script_system* system)
{
    ASSERT2_S(system);
    if(!system->enabled)
        return;

    lua_getglobal(system->lua, "ScriptFrame");
    lua_pushlightuserdata(system->lua, system->world);
    if(lua_pcall(system->lua, 1, 0, 0) != 0)
    {
        printf("lua: error running ScriptFrame: %s\n",
            lua_tostring(system->lua, -1));
    }
}

void script_frame_render(struct script_system* system, bool xtra_pass)
{
    ASSERT2_S(system);    
    if(!system->enabled)
        return;

    lua_getglobal(system->lua, "ScriptFrameRender");
    lua_pushlightuserdata(system->lua, system->world);
    lua_pushboolean(system->lua, xtra_pass);
    if(lua_pcall(system->lua, 2, 0, 0) != 0)
    {
        printf("lua: error running ScriptFrameRender: %s\n",
            lua_tostring(system->lua, -1));
    }
}

void script_frame_ui(struct script_system* system)
{
    ASSERT2_S(system);    
    if(!system->enabled)
        return;

    lua_getglobal(system->lua, "ScriptFrameUI");
    lua_pushlightuserdata(system->lua, system->world);
    lua_pushlightuserdata(system->lua, system->world->ui);
    if(lua_pcall(system->lua, 2, 0, 0) != 0)
    {
        printf("lua: error running ScriptFrameUI: %s\n",
            lua_tostring(system->lua, -1));
    }
}