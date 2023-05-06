#ifndef L_FUNCTIONS_H
#define L_FUNCTIONS_H
#include <lua.h>

void l_functions_register(lua_State* l);
struct world* l_get_world(lua_State* l, int i);
struct ui_data* l_get_ui_data(lua_State* l, int i);

#endif