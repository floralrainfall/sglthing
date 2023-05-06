#ifndef SCRIPT_H
#define SCRIPT_H
#include <stdbool.h>
#include <lua.h>

struct script_system
{
    lua_State* lua;

    bool enabled;

    struct world* world;
};

struct script_system* script_init(char* file, struct world* world);
void script_frame(struct script_system* system);
void script_frame_render(struct script_system* system, bool xtra_pass);
void script_frame_ui(struct script_system* system);

#endif