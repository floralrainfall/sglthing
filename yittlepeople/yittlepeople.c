#include "yittlepeople.h"
#include <sglthing/world.h>
#include <sglthing/keyboard.h>
#include "guy.h"

struct yittlepeople_data client_data;
struct yittlepeople_data server_data;

static void __guy_render(gpointer key, gpointer value, gpointer user_data)
{
    int guy_id = *(int*)key;
    guy_t* guy = (guy_t*)value;
    struct world* world = (struct world*)user_data;

    guy_render(world, *guy, client_data.guy_model, client_data.guy_shader, client_data.civs);
}

static void __building_render(gpointer key, gpointer value, gpointer user_data)
{
    int building_id = *(int*)key;
    building_t* building = (guy_t*)value;
    struct world* world = (struct world*)user_data;
    vec3 building_pos = {
        ((float)building->position[0])/100.0,
        0.0,
        ((float)building->position[1])/100.0,
    };

    if(world->gfx.shadow_pass == false)
    {
        char dbginfo[64];
        snprintf(dbginfo,64,"%i, %s",building->building_id,building->building_inprogress ? "building" : "done");
        mat4 txt_mtx;
        glm_mat4_identity(txt_mtx);
        ui_draw_text_3d(world->ui, world->viewport, world->cam.position, world->cam.front, building_pos, world->cam.fov, txt_mtx, world->vp, dbginfo);
    }
}

static void __guy_tick(gpointer key, gpointer value, gpointer user_data)
{
    int guy_id = *(int*)key;
    guy_t* guy = (guy_t*)value;
    struct world* world = (struct world*)user_data;

    guy_ai_tick(guy, client_data.civs, client_data.building_hash, world->delta_time);
}

static void yittle_frame_render(struct world* world)
{
    g_hash_table_foreach(client_data.building_hash, __building_render, world);
    g_hash_table_foreach(client_data.guy_hash, __guy_render, world);
}

static bool build_debounce = false;

static void yittle_frame(struct world* world)
{
    g_hash_table_foreach(client_data.guy_hash, __guy_tick, world);

    if(mouse_state.mouse_button_r)
    {
        if(!build_debounce)
        {
            building_t* new_building = building_construct(client_data.building_hash, 0, BUILDING_HOUSE, (vec2){world->cam.position[0],world->cam.position[2]});
            client_data.civs[0].current_building_id = new_building->building_id;
            build_debounce = true;
        }
    }
    else
        build_debounce = false;

}

void sglthing_init_api(struct world* world)
{
    client_data.guy_hash = g_hash_table_new_full(g_int_hash, g_int_equal, NULL, NULL);
    client_data.building_hash = g_hash_table_new_full(g_int_hash, g_int_equal, NULL, NULL);
    world->world_frame_render_user = yittle_frame_render;
    world->world_frame_user = yittle_frame;

    load_model("yittle/yittleman.obj");
    client_data.guy_model = get_model("yittle/yittleman.obj");

    int f = compile_shader("shaders/fragment.fs", GL_FRAGMENT_SHADER);
    client_data.guy_shader = link_program(compile_shader("shaders/normal.vs", GL_VERTEX_SHADER), f);

    client_data.civs[0].current_building_id = -1;
    for(int i = 0; i < 100; i++)
    {
        guy_t guy;
        guy.civ_id = 0;
        guy.position[0] = g_random_int_range(-5000,5000);
        guy.position[1] = g_random_int_range(-5000,5000);
        guy_new(client_data.guy_hash, guy, client_data.civs);
    }

    world->cam.position[0] = 50.f;
    world->cam.position[1] = 50.f;
    world->cam.position[2] = 50.f;
    world->cam.pitch = -90.f;
    world->gfx.fog_maxdist = 64.f;
    world->gfx.fog_mindist = 50.f;
}