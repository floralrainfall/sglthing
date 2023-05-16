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
}

static void rdm_frame_ui(struct world* world)
{

}

static void rdm_frame_render(struct world* world)
{

}

void sglthing_init_api(struct world* world)
{
    printf("rdm2: hello world\n");
#ifndef HEADLESS
    glfwSetWindowTitle(world->gfx.window, "RDM2");
#endif

    world->world_frame_user = rdm_frame;
    world->world_frame_ui_user = rdm_frame_ui;
    world->world_frame_render_user = rdm_frame_render;

    client_state.local_player = NULL;

    net_init(world);
}