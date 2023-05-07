#include "l_functions.h"
#include "../world.h"
#include "../ui.h"
#include "../script.h"
#include "../shader.h"

// _G.world

static int l_world_time(lua_State* l)
{
    struct world* world = l_get_world(l, 1);
    lua_pushnumber(l, world->time);
    return 1;
}

static int l_world_delta(lua_State* l)
{
    struct world* world = l_get_world(l, 1);
    lua_pushnumber(l, world->delta_time);
    return 1;
}

static int l_world_render_model(lua_State* l)
{
    struct world* world = l_get_world(l, 1);
    struct model* model = l_get_model(l, 2);
}

// _G.ui

static int l_ui_text(lua_State* l)
{
    struct ui_data* ui = l_get_ui_data(l, 1);

    float px = lua_tonumber(l, 2);
    float py = lua_tonumber(l, 3);
    float depth = lua_tonumber(l, 4);
    char* text = lua_tostring(l, 5);

    ui_draw_text(ui, px, py, text, depth);
    return 0;
}

static int l_ui_image(lua_State* l)
{
    struct ui_data* ui = l_get_ui_data(l, 1);

    float px = lua_tonumber(l, 2);
    float py = lua_tonumber(l, 3);
    float sx = lua_tonumber(l, 4);
    float sy = lua_tonumber(l, 5);
    float depth = lua_tonumber(l, 6);
    int image = lua_tointeger(l, 7);

    ui_draw_image(ui, px, py, sx, sy, image, depth);
    return 0;
}

static int l_ui_font(lua_State* l)
{
    struct ui_data* ui = l_get_ui_data(l, 1);
    int argc = lua_gettop(l);
    if(argc == 2)
    {
        ui->ui_font = lua_topointer(l, 2);
        return 0;
    }
    else
    {
        lua_pushlightuserdata(l, ui->ui_font);
        return 1;
    }
}

// _G.asset

static int l_asset_texture(lua_State* l)
{
    char* path = lua_tostring(l, 1);
    load_texture(path);
    lua_pushinteger(l, get_texture(path));
    return 1;
}

static int l_asset_model(lua_State* l)
{
    char* path = lua_tostring(l, 1);
    load_model(path);
    lua_pushlightuserdata(l, get_model(path));
    return 1;
}

static int l_asset_font(lua_State* l)
{
    char* path = lua_tostring(l, 1);
    int cx = lua_tointeger(l, 2);
    int cy = lua_tointeger(l, 3);
    int cw = lua_tointeger(l, 4);
    int ch = lua_tointeger(l, 5);
    lua_pushlightuserdata(l, ui_load_font(path, cx, cy, cw, ch));
    return 1;
}

static int l_asset_shader(lua_State* l)
{
    char* path = lua_tostring(l, 1);
    int type = lua_tointeger(l, 2);
    int shader = compile_shader(path, type);
    lua_pushinteger(l, shader);
    return 1;
}

static int l_asset_program(lua_State* l)
{
    int program = create_program();
    int argc = lua_gettop(l);
    for(int i = 1; i <= argc; i++)
        attach_program_shader(program, lua_tointeger(l, i));
    link_programv(program);
    lua_pushinteger(l, program);
    return 1;
}

// _G.matrix

struct __matrix_data
{
    mat4 data;
};

static int l_matrix_identity(lua_State* l)
{
    struct __matrix_data* mat = (struct __matrix_data*)lua_topointer(l, 1);    
    glm_mat4_identity(mat->data);
    return 0;
}

static int l_matrix_new(lua_State* l)
{
    struct __matrix_data* new_mat = (struct __matrix_data*)malloc(sizeof(struct __matrix_data));
    glm_mat4_zero(new_mat->data);
    lua_pushlightuserdata(l, new_mat);
    return 1;
}

// _G.gl

static int l_gl_render_model(lua_State* l)
{
    struct world* world = l_get_world(l, 1);
    struct model* model = l_get_model(l, 2);
    struct __matrix_data* mat = (struct __matrix_data*)lua_topointer(l, 3);
    int shader = lua_tointeger(l, 4);
    world_draw_model(world, model, shader, mat->data, true);
    return 0;
}

static int l_gl_viewport(lua_State* l)
{
    struct world* world = l_get_world(l, 1);
    int argc = lua_gettop(l);
    if(argc == 1)
    {
        lua_newtable(l);
        lua_pushstring(l, "x");
        lua_pushinteger(l, world->gfx.screen_width);
        lua_settable(l, -3);
        lua_pushstring(l, "y");
        lua_pushinteger(l, world->gfx.screen_height);
        lua_settable(l, -3);
        return 1;
    }
    else if(argc == 2)
    {
        lua_pushstring(l, "x");
        lua_gettable(l, 2);
        double x = lua_tonumber(l, 3);
        lua_pop(l,1);

        lua_pushstring(l, "y");
        lua_gettable(l, 2);
        double y = lua_tonumber(l, 3);
        lua_pop(l,1);
        
        printf("%d,%d\n",x,y);
    }
}

void l_functions_register(lua_State* l)
{
    // _G.world

    lua_newtable(l);

    lua_pushstring(l, "time");
    lua_pushcfunction(l, l_world_time);
    lua_settable(l, -3);

    lua_pushstring(l, "delta");
    lua_pushcfunction(l, l_world_delta);
    lua_settable(l, -3);

    lua_setglobal(l, "world");

    // _G.ui

    lua_newtable(l);

    lua_pushstring(l, "text");
    lua_pushcfunction(l, l_ui_text);
    lua_settable(l, -3);

    lua_pushstring(l, "image");
    lua_pushcfunction(l, l_ui_image);
    lua_settable(l, -3);

    lua_pushstring(l, "font");
    lua_pushcfunction(l, l_ui_font);
    lua_settable(l, -3);

    lua_setglobal(l, "ui");

    // _G.asset

    lua_newtable(l);

    lua_pushstring(l, "texture");
    lua_pushcfunction(l, l_asset_texture);
    lua_settable(l, -3);

    lua_pushstring(l, "model");
    lua_pushcfunction(l, l_asset_model);
    lua_settable(l, -3);

    lua_pushstring(l, "font");
    lua_pushcfunction(l, l_asset_font);
    lua_settable(l, -3);

    lua_pushstring(l, "shader");
    lua_pushcfunction(l, l_asset_shader);
    lua_settable(l, -3);

    lua_pushstring(l, "program");
    lua_pushcfunction(l, l_asset_program);
    lua_settable(l, -3);

    lua_setglobal(l, "asset");

    // _G.matrix

    lua_newtable(l);

    lua_pushstring(l, "identity");
    lua_pushcfunction(l, l_matrix_identity);
    lua_settable(l, -3);

    lua_pushstring(l, "new");
    lua_pushcfunction(l, l_matrix_new);
    lua_settable(l, -3);

    lua_setglobal(l, "matrix");

    // _G.gl

    lua_newtable(l);

    lua_pushstring(l, "fragment_shader");
    lua_pushinteger(l, GL_FRAGMENT_SHADER);
    lua_settable(l, -3);
    
    lua_pushstring(l, "vertex_shader");
    lua_pushinteger(l, GL_VERTEX_SHADER);
    lua_settable(l, -3);

    lua_pushstring(l, "render_model");
    lua_pushcfunction(l, l_gl_render_model);
    lua_settable(l, -3);

    lua_pushstring(l, "viewport");
    lua_pushcfunction(l, l_gl_viewport);
    lua_settable(l, -3);

    lua_setglobal(l, "gl");
}

struct world* l_get_world(lua_State* l, int i)
{
    if(!lua_islightuserdata(l, i))
    {
        lua_pushliteral(l, "not userdata");
        lua_error(l);
    }
    struct world* world = lua_topointer(l, i);
    return world;
}

struct ui_data* l_get_ui_data(lua_State* l, int i)
{
    if(!lua_islightuserdata(l, i))
    {
        lua_pushliteral(l, "not userdata");
        lua_error(l);
    }
    struct ui_data* ui = lua_topointer(l, i);
    return ui;
}

struct model* l_get_model(lua_State* l, int i)
{
    if(!lua_islightuserdata(l, i))
    {
        lua_pushliteral(l, "not userdata");
        lua_error(l);
    }
    struct model* mdl = lua_topointer(l, i);
    return mdl;
}
