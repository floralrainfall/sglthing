#include "rdm2.h"
#include <sglthing/keyboard.h>
#include <sglthing/sglthing.h>
#include <sglthing/texture.h>
#include "weapons.h"
#include "ray.h"

struct rdm2_state client_state;
struct rdm2_state server_state;

static void rdm_frame(struct world* world)
{
    world->gfx.fog_maxdist = 24.f + world->cam.position[1];

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
            if(mouse_state.mouse_button_l)
                weapon_trigger_fire(world, false);
            else if(mouse_state.mouse_button_r)
                weapon_trigger_fire(world, true);

            float cam_yaw = mouse_position[0] * world->delta_time * 4.f;
            float cam_pitch = mouse_position[1] * world->delta_time * 4.f;

            world->cam.yaw += cam_yaw;
            world->cam.pitch -= cam_pitch;
            client_state.mouse_update_diff += fabsf(cam_yaw);
            client_state.mouse_update_diff += fabsf(cam_pitch);

            mat4 rot, rot1, rot2;
            glm_euler((vec3){
                0.f,
                glm_rad(-world->cam.yaw) + M_PI_2f,
                0.f
            }, rot1);
            glm_euler((vec3){
                glm_rad(-world->cam.pitch),
                0.f,
                0.f
            }, rot2);
            glm_mat4_mul(rot1, rot2, rot);
            
            glm_mat4_quat(rot, client_state.local_player->direction);

            glm_vec3_zero(player_move); // fall

            player_move[0] += cosf(world->cam.yaw*M_PI_180f) * get_input("z_axis") * world->delta_time * 5.f;
            player_move[2] += sinf(world->cam.yaw*M_PI_180f) * get_input("z_axis") * world->delta_time * 5.f;

            player_move[0] += cosf(world->cam.yaw*M_PI_180f+M_PI_2f) * get_input("x_axis") * world->delta_time * 5.f;
            player_move[2] += sinf(world->cam.yaw*M_PI_180f+M_PI_2f) * get_input("x_axis") * world->delta_time * 5.f;

            vec3 eye_position;
            glm_vec3_add(player_move, client_state.local_player->position, expected_position);
            glm_vec3_add(expected_position, (vec3){0.f,1.25f,0.f}, eye_position);
            if(!map_determine_collision_client(client_state.map_manager, expected_position) && !map_determine_collision_client(client_state.map_manager, eye_position))
            {
                glm_vec3_copy(expected_position, client_state.local_player->position);
            }
        }

        if(world->client.pung)
        {
            float distance = glm_vec3_distance(client_state.local_player->position, client_state.local_player->replicated_position);
            if(distance > 0.5f || client_state.mouse_update_diff > 1.f)
            {
                struct network_packet _pak; 
                union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

                _pak.meta.packet_type = RDM_PACKET_UPDATE_POSITION;
                _pak.meta.acknowledge = false;
                glm_vec3_copy(client_state.local_player->position, _data->update_position.position);
                glm_quat_copy(client_state.local_player->direction, _data->update_position.direction);
                _data->update_position.player_id = client_state.local_player_id;

                network_transmit_packet(&world->client, &world->client.client, _pak);

                client_state.mouse_update_diff = 0.f;
            }
            world->client.pung = false;
        }

        glm_vec3_copy(client_state.local_player->position, world->cam.position);
        glm_vec3_copy(client_state.local_player->position, world->gfx.sun_position);
        world->cam.position[1] += 0.75f;
    }

    //world->cam.position[2] = sinf(world->time/2.f)*64.f;
    //world->cam.position[0] = cosf(world->time/2.f)*64.f;

    if(world->client_on)
    {
        map_update_chunks(client_state.map_manager, world);

        gamemode_frame(&client_state.gamemode, world);
    }
    if(world->server_on)
    {
        gamemode_frame(&server_state.gamemode, world);

        if(server_state.next_pending < world->time && server_state.chunk_packets_pending->len != 0)
        {
            profiler_event("net: send queued packets");
            struct pending_packet packet = g_array_index(server_state.chunk_packets_pending, struct pending_packet, 0);
            if(packet.giveup < world->time)
            {
                g_array_remove_index(server_state.chunk_packets_pending, 0);
            }
            else
            {
                network_transmit_packet(&world->server, packet.client, packet.packet);
                g_array_remove_index(server_state.chunk_packets_pending, 0);
                server_state.next_pending = world->time + 0.001f;
            }
            profiler_end();
        }
    }
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

    if(world->client_on)
    {
        if(world->client.client.lag > 1)
            ui_font2_text(world->ui, world->gfx.screen_width/1.5, world->gfx.screen_height-24, client_state.big_font, "LAG", 1.f);
    }


    if(client_state.local_player && !client_state.server_motd_dismissed)
    {
        ui_draw_panel(world->ui, 8.f, world->gfx.screen_height-8.f, world->gfx.screen_width-16.f, world->gfx.screen_height-16.f, 0.4f);
        ui_font2_text(world->ui, 8.f, 24.f, client_state.big_font, "Welcome to RDM2", 0.2f);
        snprintf(dbginfo,64,"You are playing on '%s'.", world->client.server_name);
        ui_font2_text(world->ui, 8.f, 24.f + 24.f, client_state.normal_font, dbginfo, 0.2f);
        snprintf(dbginfo,64,"Message of the Day: '%s'", world->client.server_motd);
        ui_font2_text(world->ui, 8.f, 24.f + 24.f + 12.f, client_state.normal_font, dbginfo, 0.2f);
        snprintf(dbginfo,64,"SGLSite Message of the Day: '%s'", world->client.http_client.motd);
        ui_font2_text(world->ui, 8.f, 24.f + 24.f + 24.f, client_state.normal_font, dbginfo, 0.2f);
        if(!client_state.local_player->client->verified)
            ui_font2_text(world->ui, 8.f, 24.f + 24.f + 48.f, client_state.normal_font, "You are not logged in, and connected as a guest.", 0.2f);

        bool ok_button = ui_draw_button(world->ui, world->gfx.screen_width - 64.f, world->gfx.screen_height - 56.f, 40.f, 24.f, world->gfx.white_texture, 1.0f);
        ui_font2_text(world->ui, world->gfx.screen_width - 64.f, world->gfx.screen_height - 32.f, client_state.big_font, "Ok", 0.2f);

        if(ok_button)
        {
            client_state.server_motd_dismissed = true;
            set_focus(world->gfx.window, true);
        }

        ui_end_panel(world->ui);
    }
    else
    {
        if(client_state.local_player)
        {
            snprintf(dbginfo,64,"%i/100", client_state.local_player->health);
            ui_font2_text(world->ui, 64.f, 64.f, client_state.big_font, dbginfo, 1.f);

            int img_size = 32;
            if(client_state.local_player->weapon_ammos[client_state.local_player->active_weapon] != -1)
            {
                snprintf(dbginfo,64,"%i", client_state.local_player->weapon_ammos[client_state.local_player->active_weapon]);
                ui_font2_text(world->ui, world->gfx.screen_width - 64.f - ui_font2_text_len(client_state.big_font, dbginfo), 64.f + img_size, client_state.big_font, dbginfo, 1.f);
            }

            for(int i = 1; i < __WEAPON_MAX; i++)
            {
                if(keys_down[GLFW_KEY_0 + i])
                {
                    struct network_packet _pak; 
                    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

                    _pak.meta.packet_type = RDM_PACKET_UPDATE_WEAPON;
                    _pak.meta.acknowledge = false;
                    _data->update_weapon.weapon = i;

                    network_transmit_packet(&world->client, &world->client.client, _pak);
                }
                float pos_off = 0.f;
                if(client_state.local_player->active_weapon == i)
                    pos_off += sinf(world->time * 4.f) * 4.f;
                ui_draw_image(world->ui, world->gfx.screen_width - 64.f - (__WEAPON_MAX * img_size) + (i * img_size), 64.f + img_size + pos_off, img_size, img_size, client_state.weapon_icon_textures[i], 1.f);
                if(client_state.local_player->weapon_ammos[i] != -1)
                {
                    snprintf(dbginfo,64,"%i", client_state.local_player->weapon_ammos[i]);
                    ui_font2_text(world->ui, world->gfx.screen_width - 64.f - (__WEAPON_MAX * img_size) + (i * img_size), 64.f + pos_off, client_state.normal_font, dbginfo, 0.8f);
                }
            }
        }

        if(get_focus() && mouse_state.mouse_button_m && client_state.server_motd_dismissed && !client_state.context_mode)
        {
            client_state.context_mode = true;
            set_focus(world->gfx.window, false);
        }

        char dbg_stuff[256];
        snprintf(dbg_stuff,256,"RDM2 Debug\nServer Unack: %i, Client Unack: %i\nS Tx: %i, C Tx: %i\nS Rx: %i, C Rx: %i\n",
            world->server.packet_unacknowledged->len,
            world->client.packet_unacknowledged->len,
            world->server.packet_tx_numbers[world->server.packet_time],
            world->client.packet_tx_numbers[world->client.packet_time],
            world->server.packet_rx_numbers[world->server.packet_time],
            world->client.packet_rx_numbers[world->client.packet_time]);
        ui_font2_text(world->ui, 0.f, world->gfx.screen_height-48.f, client_state.big_font, dbg_stuff, 1.f);

        if(client_state.context_mode)
        {            ui_draw_panel(world->ui, world->gfx.screen_width / 2 - 128.f, world->gfx.screen_height / 2 + 150.f, 255.f, 300.f, 1.f);
            ui_font2_text(world->ui, 8.f, 24.f, client_state.big_font, "Context Menu", 0.7f);

            bool button = ui_draw_button(world->ui, 0.f, world->ui->current_panel->size_y - 12.f, world->ui->current_panel->size_x, 12.f, world->gfx.white_texture, 1.2f);
            ui_font2_text(world->ui, 0.f, world->ui->current_panel->size_y, client_state.normal_font, "Cancel", 0.2f);

            if(button)
            {
                client_state.context_mode = false;
                set_focus(world->gfx.window, true);
            }

            button = ui_draw_button(world->ui, 0.f, world->ui->current_panel->size_y - 24.f, world->ui->current_panel->size_x, 12.f, world->gfx.white_texture, 1.2f);
            ui_font2_text(world->ui, 0.f, world->ui->current_panel->size_y - 12.f, client_state.normal_font, "Disconnect", 0.2f);

            if(button)
            {
                network_disconnect_player(&world->client, true, "Disconnect by player", &world->client.client);
            }

            button = ui_draw_button(world->ui, 0.f, world->ui->current_panel->size_y - 36.f, world->ui->current_panel->size_x, 12.f, world->gfx.white_texture, 1.2f);
            ui_font2_text(world->ui, 0.f, world->ui->current_panel->size_y - 24.f, client_state.normal_font, "Server MOTD", 0.2f);

            if(button)
            {
                client_state.server_motd_dismissed = false;
                client_state.context_mode = false;
            }

            if(world->server.status == NETWORKSTATUS_CONNECTED)
            {
                button = ui_draw_button(world->ui, 0.f, world->ui->current_panel->size_y - 48.f, world->ui->current_panel->size_x, 12.f, world->gfx.white_texture, 1.2f);
                ui_font2_text(world->ui, 0.f, world->ui->current_panel->size_y - 36.f, client_state.normal_font, "Server: Information Panel", 0.2f);

                if(button)
                {
                    client_state.server_information_panel = true;
                    client_state.context_mode = false;
                }
            }

            ui_end_panel(world->ui);
        }        

        if(client_state.server_information_panel)
        {
            ui_draw_panel(world->ui, 8.f, world->gfx.screen_height-8.f, world->gfx.screen_width-16.f, world->gfx.screen_height-16.f, 0.4f);
            ui_font2_text(world->ui, 8.f, 24.f, client_state.big_font, "Server", 0.2f);
            snprintf(dbginfo,64,"%i clients connected. %i packets pending", world->server.server_clients->len, server_state.chunk_packets_pending->len);
            ui_font2_text(world->ui, 8.f, 24.f + 24.f, client_state.normal_font, dbginfo, 0.2f);

            char dbginfo2[64];
            for(int i = 0; i < world->server.server_clients->len; i++)
            {
                struct network_client* client = &g_array_index(world->server.server_clients, struct network_client, i);
                struct rdm_player* player = (struct rdm_player*)client->user_data;
                snprintf(dbginfo,64,"%i - %s, ping %f, pd %p", i, client->client_name, client->lag, player);
                ui_font2_text(world->ui, 8.f, 24.f + 48.f + (i * 12.f), client_state.normal_font, dbginfo, 0.2f);
                if(player)
                {
                    snprintf(dbginfo2,64,"%f,%f,%f %i", player->replicated_position[0], player->replicated_position[1], player->replicated_position[2], player->team);
                    ui_font2_text(world->ui, 8.f + ui_font2_text_len(client_state.normal_font, dbginfo) + 12.f, 24.f + 48.f + (i * 12.f), client_state.normal_font, dbginfo2, 0.2f);                    
                }
            }

            bool ok_button = ui_draw_button(world->ui, world->gfx.screen_width - 64.f, world->gfx.screen_height - 56.f, 40.f, 24.f, world->gfx.white_texture, 1.0f);
            ui_font2_text(world->ui, world->gfx.screen_width - 64.f, world->gfx.screen_height - 32.f, client_state.big_font, "Ok", 0.2f);

            if(ok_button)
            {
                client_state.server_information_panel = false;
                set_focus(world->gfx.window, true);
            }

            bool prof_button = ui_draw_button(world->ui, world->gfx.screen_width - 64.f, world->gfx.screen_height - 80.f, 40.f, 24.f, world->gfx.white_texture, 1.0f);
            ui_font2_text(world->ui, world->gfx.screen_width - 64.f, world->gfx.screen_height - 48.f, client_state.big_font, "Prof", 0.2f);

            if(prof_button)
            {
                world->profiler_on = true;
                client_state.server_information_panel = false;
                set_focus(world->gfx.window, true);
            }

            ui_end_panel(world->ui);
        }
    }
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

    mat4 player_matrix, player_rotated_matrix;
    glm_mat4_identity(player_matrix);
    glm_translate(player_matrix, player->position);
    glm_translate(player_matrix, (vec3){0.f,-1.f,0.f});
    glm_quat_rotate(player_matrix, player->direction, player_rotated_matrix);

    if(client->player_id != client_state.local_player_id || world->gfx.shadow_pass)
        world_draw_model(world, client_state.rdm_guy, client_state.object_shader, player_rotated_matrix, true);
    struct model* weapon_model = client_state.weapons[player->active_weapon];
    if(weapon_model)
    {
        mat4 weapon_matrix;
        glm_mat4_identity(weapon_matrix);
        glm_translate(weapon_matrix, player->position);
        glm_quat_rotate(weapon_matrix, player->direction, weapon_matrix);
        glm_translate(weapon_matrix, (vec3){-0.3f,0.5f,0.1f});

        world_draw_model(world, weapon_model, client_state.object_shader, weapon_matrix, true);
    }
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
    glm_translate(test_cube,server_state.last_position);
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
        load_snd("rdm2/sound/ak47_shoot.ogg");
        load_snd("rdm2/sound/ricochet_ground.ogg");
        load_snd("rdm2/sound/ouch.ogg");

        world->ui->ui_font = ui_load_font("uiassets/font2.png",8,16,32,16);

        client_state.big_font = ui_load_font2(world->ui, "uiassets/impact.ttf",0,24);
        client_state.normal_font = ui_load_font2(world->ui, "uiassets/arial.ttf",0,12);

        char mdlname[64];
        for(int i = 0; i < __WEAPON_MAX; i++)
        {
            snprintf(mdlname, 64, "rdm2/weapon/weapon%i.obj", i);
            load_model(mdlname);
            client_state.weapons[i] = get_model(mdlname);
            snprintf(mdlname, 64, "rdm2/weapon/weapon%i_render.png", i);
            load_texture(mdlname);
            client_state.weapon_icon_textures[i] = get_texture(mdlname);
        }

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

        client_state.lobby_music = load_snd("rdm2/music/lobby0.mp3");
        client_state.lobby_music->loop = false;
        client_state.lobby_music->multiplier = 0.1f;
        client_state.playing_music = play_snd2(client_state.lobby_music);
        client_state.roundstart_sound = load_snd("rdm2/sound/round_start.ogg");
        client_state.roundstart_sound->loop = false;
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
    world->cam.lock = true;
    world->gfx.fog_maxdist = 32.f;
    world->gfx.fog_mindist = 15.f;

    input_lock_tab = true;

    if(world->client_on)
    {
        client_state.server_motd_dismissed = false;
        client_state.local_player = NULL;

        client_state.map_manager = (struct map_manager*)malloc(sizeof(struct map_manager));
        map_init(client_state.map_manager);
    }

    if(world->server_on)
    {
        server_state.map_server = (struct map_server*)malloc(sizeof(struct map_server));
        server_state.map_server->map_ready = false;
        server_state.chunk_packets_pending = g_array_new(true, true, sizeof(struct pending_packet));
    }

    net_init(world);

    if(world->client_on)
        gamemode_init(&client_state.gamemode, false);
    if(world->server_on)
        gamemode_init(&server_state.gamemode, true);
}