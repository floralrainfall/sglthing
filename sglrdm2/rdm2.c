#include "rdm2.h"
#include <sglthing/keyboard.h>
#include "weapons.h"

struct rdm2_state client_state;
struct rdm2_state server_state;

static void rdm_frame(struct world* world)
{
    if(mouse_state.mouse_button_l)
        weapon_fire_primary();
    else if(mouse_state.mouse_button_r)
        weapon_fire_secondary();

    world->cam.yaw = 45.f;
    world->cam.pitch = -45.f;
}

static void rdm_frame_ui(struct world* world)
{

}

static void rdm_frame_render(struct world* world)
{
    mat4 test_cube;
    glm_mat4_identity(test_cube);
    world_draw_model(world, client_state.map_manager->cube, client_state.object_shader, test_cube, false);
    map_render_chunks(world, client_state.map_manager);
}

void sglthing_init_api(struct world* world)
{
    printf("rdm2: hello world\n");
#ifndef HEADLESS
    glfwSetWindowTitle(world->gfx.window, "RDM2");
#endif

    int f = compile_shader("shaders/fragment.fs", GL_FRAGMENT_SHADER);
    client_state.object_shader = link_program(compile_shader("shaders/normal.vs", GL_VERTEX_SHADER), f);

    world->world_frame_user = rdm_frame;
    world->world_frame_ui_user = rdm_frame_ui;
    world->world_frame_render_user = rdm_frame_render;

    world->cam.position[0] = -8.f;
    world->cam.position[2] = -8.f;
    world->cam.position[1] = 48.f;

    client_state.local_player = NULL;

    client_state.map_manager = (struct map_manager*)malloc(sizeof(struct map_manager));
    map_init(client_state.map_manager);

    net_init(world);
}