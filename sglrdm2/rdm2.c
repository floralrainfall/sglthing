#include "rdm2.h"
#include <sglthing/keyboard.h>
#include <sglthing/sglthing.h>
#include "weapons.h"

struct rdm2_state client_state;
struct rdm2_state server_state;

static void rdm_frame(struct world* world)
{
    world->gfx.fog_maxdist = 15.f + world->cam.position[1];
    if(mouse_state.mouse_button_l)
        weapon_fire_primary();
    else if(mouse_state.mouse_button_r)
        weapon_fire_secondary();

    if(client_state.local_player)
    {
        vec3 player_move = {0};
        vec3 expected_position;
        vec3 ground_test;
        glm_vec3_sub(client_state.local_player->position, (vec3){0.f,0.2f,0.f}, ground_test);

        client_state.player_grounded = map_determine_collision_client(client_state.map_manager, ground_test);
        if(!client_state.player_grounded)
        {
            vec3 velocity_frame, velocity_frame_p;
            glm_vec3_copy(client_state.player_velocity, velocity_frame_p);
            glm_vec3_mul(velocity_frame_p, (vec3){world->delta_time, world->delta_time, world->delta_time}, velocity_frame);
            glm_vec3_add(velocity_frame, player_move, player_move);
            if(client_state.player_velocity[1] > -5.0)
                client_state.player_velocity[1] -= world->delta_time * 5.f;

            glm_vec3_add(player_move, client_state.local_player->position, expected_position);
            if(!map_determine_collision_client(client_state.map_manager, expected_position) )
                glm_vec3_copy(expected_position, client_state.local_player->position);
        }
        else
        {
            glm_vec3_zero(client_state.player_velocity);

            bool player_submerged = map_determine_collision_client(client_state.map_manager, client_state.local_player->position);
            while(player_submerged)
            {
                client_state.local_player->position[1] += 0.1f * world->delta_time;
                player_submerged = map_determine_collision_client(client_state.map_manager, client_state.local_player->position);
            }

            if(get_focus() && keys_down[GLFW_KEY_SPACE])
            {
                glm_vec3_add(client_state.player_velocity, (vec3){0.f,3.0f,0.f}, client_state.player_velocity);         
                glm_vec3_add(client_state.local_player->position, (vec3){0.f,0.1f,0.f}, client_state.local_player->position);
            }            
        }

        if(get_focus())
        {
            world->cam.yaw += mouse_position[0] * world->delta_time * 4.f;
            world->cam.pitch -= mouse_position[1] * world->delta_time * 4.f;

            client_state.local_player->yaw = world->cam.yaw;
            client_state.local_player->pitch = world->cam.pitch;

            glm_vec3_zero(player_move); // fall

            player_move[0] += cosf(world->cam.yaw*M_PI_180f) * get_input("z_axis") * world->delta_time * 5.f;
            player_move[2] += sinf(world->cam.yaw*M_PI_180f) * get_input("z_axis") * world->delta_time * 5.f;

            player_move[0] += cosf(world->cam.yaw*M_PI_180f+M_PI_2f) * get_input("x_axis") * world->delta_time * 5.f;
            player_move[2] += sinf(world->cam.yaw*M_PI_180f+M_PI_2f) * get_input("x_axis") * world->delta_time * 5.f;

            glm_vec3_add(player_move, client_state.local_player->position, expected_position);
            if(!map_determine_collision_client(client_state.map_manager, expected_position))
            {
                glm_vec3_copy(expected_position, client_state.local_player->position);
            }
        }

        glm_vec3_copy(client_state.local_player->position, world->cam.position);
        glm_vec3_copy(client_state.local_player->position, world->gfx.sun_position);
        world->cam.position[1] += 1.75f;
    }

    //world->cam.position[2] = sinf(world->time/2.f)*64.f;
    //world->cam.position[0] = cosf(world->time/2.f)*64.f;

    if(world->client_on)
    {
        map_update_chunks(client_state.map_manager, world);

        gamemode_frame(&client_state.gamemode, world);
    }
    if(world->server_on)
        gamemode_frame(&server_state.gamemode, world);
}

static void rdm_frame_ui(struct world* world)
{
    char gamemode_txt[64];
    char gamemode2_txt[64] = { 0 };
    double time_left = client_state.gamemode.gamemode_end - client_state.gamemode.network_time;
    snprintf(gamemode_txt,64,"%01i:%02i", 
        (int)floor(time_left / 60.0),
        (int)fmod(time_left, 60.0)
    );
    if(!client_state.gamemode.started)
        snprintf(gamemode2_txt,64,"The round will start in");
    float _text_len =  ui_font2_text_len(client_state.normal_font, gamemode2_txt);
    float text_len = ui_font2_text_len(client_state.big_font, gamemode_txt) + _text_len;
    ui_font2_text(world->ui, world->gfx.screen_width / 2.0 - (text_len / 2.0), world->gfx.screen_height - 24, client_state.normal_font, gamemode2_txt, 1.f);
    ui_font2_text(world->ui, world->gfx.screen_width / 2.0 - ((text_len) / 2.0) + _text_len + 8, world->gfx.screen_height - 24, client_state.big_font, gamemode_txt, 1.f);

    snprintf(gamemode_txt,64,"The gamemode is: %s", 
        gamemode_name(client_state.gamemode.gamemode)
    );
    ui_font2_text(world->ui, world->gfx.screen_width / 2.0 - (ui_font2_text_len(client_state.normal_font, gamemode_txt) / 2.0), world->gfx.screen_height - 42, client_state.normal_font, gamemode_txt, 1.f);

    char dbginfo[64];
    snprintf(dbginfo,64,"%i player(s) online",g_hash_table_size(world->client.players));
    ui_font2_text(world->ui, 0.f, 0.f, client_state.big_font, "RDM2 ALPHA", 1.f);
    ui_font2_text(world->ui, 128.f, 0.f, client_state.normal_font, dbginfo, 1.f);

    if(client_state.local_player)
        snprintf(dbginfo,64,"%fx%fx%f %fx%fx%f",
            client_state.local_player->position[0], client_state.local_player->position[1], client_state.local_player->position[2],
            client_state.player_velocity[0], client_state.player_velocity[1], client_state.player_velocity[2]);
    else
        snprintf(dbginfo,64,"waiting for player");
    
    ui_draw_text(world->ui, 0.f, 24.f, dbginfo, 1.f);
}

static void __player_frame(gpointer key, gpointer value, gpointer user_data)
{
    struct world* world = (struct world*)user_data;
    struct network_client* client = (struct network_client*)value;
    struct rdm_player* player = (struct rdm_player*)client->user_data;

}

static void __player_render(gpointer key, gpointer value, gpointer user_data)
{
    struct world* world = (struct world*)user_data;
    struct network_client* client = (struct network_client*)value;
    struct rdm_player* player = (struct rdm_player*)client->user_data;

    mat4 player_matrix, rotation_matrix;
    glm_mat4_identity(player_matrix);
    glm_translate(player_matrix, player->position);
    glm_euler((vec3){0.f, -player->yaw * M_PI_180f + M_PI_2f,0.f},rotation_matrix);
    glm_mat4_mul(player_matrix, rotation_matrix, player_matrix);

    if(client->player_id != client_state.local_player_id || world->gfx.shadow_pass)
        world_draw_model(world, client_state.rdm_guy, client_state.object_shader, player_matrix, true);
}

static void rdm_frame_render(struct world* world)
{
    if(!world->gfx.shadow_pass)
    {
        glDepthMask(GL_FALSE);  

        mat4 skybox_mat;
        glm_mat4_identity(skybox_mat);
        glm_translate(skybox_mat, world->cam.position);
        //world_draw_model(world, client_state.skybox, client_state.skybox_shader, skybox_mat, true);

        glm_translate_y(skybox_mat, 0.2);
        glm_scale(skybox_mat, (vec3){15.f,1.f,15.f});
        world_draw_model(world, client_state.cloud_layer, client_state.cloud_layer_shader, skybox_mat, true);

        glDepthMask(GL_LESS);  
    }

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

        client_state.big_font = ui_load_font2(world->ui, "uiassets/impact.ttf",0,24);
        client_state.normal_font = ui_load_font2(world->ui, "uiassets/arial.ttf",0,12);

        load_model("rdm2/cloud.obj");
        client_state.cloud_layer = get_model("rdm2/cloud.obj");

        load_model("rdm2/rdmguy.obj");
        client_state.rdm_guy = get_model("rdm2/rdmguy.obj");

        load_model("rdm2/sun.obj");
        client_state.sun = get_model("rdm2/sun.obj");

        //load_model("rdm2/skybox.obj");
        //client_state.skybox = get_model("rdm2/skybox.obj");

        int f = compile_shader("shaders/fragment.fs", GL_FRAGMENT_SHADER);
        client_state.object_shader = link_program(compile_shader("shaders/normal.vs", GL_VERTEX_SHADER), f);

        int v = compile_shader("rdm2/shaders/sky.vs", GL_VERTEX_SHADER);
        f = compile_shader("rdm2/shaders/sky.fs", GL_FRAGMENT_SHADER);
        client_state.skybox_shader = link_program(v, f);

        v = compile_shader("rdm2/shaders/cloud.vs", GL_VERTEX_SHADER);
        f = compile_shader("rdm2/shaders/cloud.fs", GL_FRAGMENT_SHADER);
        client_state.cloud_layer_shader = link_program(v, f);

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

    world->gfx.sun_direction[0] = -0.5f;
    world->gfx.sun_direction[1] = 0.5f;
    world->gfx.sun_direction[2] = 0.5f;

    world->cam.position[0] = 0.f;
    world->cam.position[2] = 0.f;
    world->cam.position[1] = 32.f;
    world->cam.lock = false;
    world->gfx.fog_maxdist = 32.f;
    world->gfx.fog_mindist = 15.f;

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

    if(world->client_on)
        gamemode_init(&client_state.gamemode, false);
    if(world->server_on)
        gamemode_init(&server_state.gamemode, true);
}