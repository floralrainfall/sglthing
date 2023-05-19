#include "rdm2.h"
#include <sglthing/keyboard.h>
#include <sglthing/sglthing.h>
#include "weapons.h"

struct rdm2_state client_state;
struct rdm2_state server_state;

static void rdm_frame(struct world* world)
{
    if(mouse_state.mouse_button_l)
        weapon_fire_primary();
    else if(mouse_state.mouse_button_r)
        weapon_fire_secondary();

    if(get_focus() && client_state.local_player)
    {
        world->cam.yaw += mouse_position[0] * world->delta_time * 4.f;
        world->cam.pitch -= mouse_position[1] * world->delta_time * 4.f;

        vec2 player_move = {0,0};

        player_move[0] += cosf(world->cam.yaw*M_PI_180f) * get_input("z_axis") * world->delta_time * 16.f;
        player_move[1] += sinf(world->cam.yaw*M_PI_180f) * get_input("z_axis") * world->delta_time * 16.f;

        player_move[0] += cosf(world->cam.yaw*M_PI_180f+M_PI_2f) * get_input("x_axis") * world->delta_time * 16.f;
        player_move[1] += sinf(world->cam.yaw*M_PI_180f+M_PI_2f) * get_input("x_axis") * world->delta_time * 16.f;

        glm_vec3_add((vec3){player_move[0],0.f,player_move[1]}, client_state.local_player->position, client_state.local_player->position);
    }

    if(client_state.local_player)
    {
        glm_vec3_copy(client_state.local_player->position, world->cam.position);
        world->cam.position[1] += 2.75f;
    }

    //world->cam.position[2] = sinf(world->time/2.f)*64.f;
    //world->cam.position[0] = cosf(world->time/2.f)*64.f;

    map_update_chunks(client_state.map_manager, world);
}

static void rdm_frame_ui(struct world* world)
{
    char dbginfo[64];
    snprintf(dbginfo,64,"%i players online",g_hash_table_size(world->client.players));
    ui_draw_text(world->ui, 0.f, 16.f, dbginfo, 1.f);

    ui_font2_text(world->ui, 16.f, 64.f, client_state.normal_font, "Hello World", 1.f);
    ui_font2_text(world->ui, 16.f, 64.f+(24.f), client_state.big_font, "Lmao Lmao", 1.f);
}

static void __player_render(gpointer key, gpointer value, gpointer user_data)
{
    struct world* world = (struct world*)user_data;
    struct network_client* client = (struct network_client*)value;
    struct rdm_player* player = (struct rdm_player*)client->user_data;

    
}

static void rdm_frame_render(struct world* world)
{
    mat4 test_cube;
    glm_mat4_identity(test_cube);
    world_draw_model(world, client_state.map_manager->cube, client_state.object_shader, test_cube, false);
    map_render_chunks(world, client_state.map_manager);

    g_hash_table_foreach(world->client.players, __player_render, world);
}

void sglthing_init_api(struct world* world)
{
    printf("rdm2: hello world\n");
#ifndef HEADLESS
    glfwSetWindowTitle(world->gfx.window, "RDM2");
#endif



    if(world->client_on)
    {
        world->ui->ui_font = ui_load_font("uiassets/font2.png",8,16,32,16);

        client_state.big_font = ui_load_font2(world->ui, "uiassets/arialbd.ttf",0,24);
        client_state.normal_font = ui_load_font2(world->ui, "uiassets/arial.ttf",0,12);

        load_model("rdm2/rdmguy.obj");
        client_state.rdm_guy = get_model("rdm2/rdmguy.obj");

        int f = compile_shader("shaders/fragment.fs", GL_FRAGMENT_SHADER);
        client_state.object_shader = link_program(compile_shader("shaders/normal.vs", GL_VERTEX_SHADER), f);
    }

    world->world_frame_user = rdm_frame;
    world->world_frame_ui_user = rdm_frame_ui;
    world->world_frame_render_user = rdm_frame_render;

    world->gfx.clear_color[0] = 0.375f;
    world->gfx.clear_color[1] = 0.649f;
    world->gfx.clear_color[2] = 0.932f;
    glm_vec4_copy(world->gfx.clear_color, world->gfx.fog_color);

    world->gfx.ambient[0] = 0.2f;
    world->gfx.ambient[1] = 0.2f;
    world->gfx.ambient[2] = 0.2f;

    world->gfx.diffuse[0] = 0.8f;
    world->gfx.diffuse[1] = 0.9f;
    world->gfx.diffuse[2] = 1.0f;

    world->gfx.specular[0] = 0.9f;
    world->gfx.specular[1] = 0.9f;
    world->gfx.specular[2] = 1.0f;

    world->cam.position[0] = 0.f;
    world->cam.position[2] = 0.f;
    world->cam.position[1] = 32.f;
    world->cam.lock = false;
    world->gfx.fog_maxdist = 64.f;

    if(world->client_on)
    {
        client_state.local_player = NULL;

        client_state.map_manager = (struct map_manager*)malloc(sizeof(struct map_manager));
        map_init(client_state.map_manager);
    }

    if(world->server_on)
    {
        server_state.map_server = (struct map_server*)malloc(sizeof(struct map_server));
        map_server_init(server_state.map_server);
    }

    net_init(world);
}