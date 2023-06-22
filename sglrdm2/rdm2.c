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
    if(world->state == WORLD_STATE_GAME)
    {
#ifndef HEADLESS
        if(client_state.local_player)
        {
            world->gfx.fog_maxdist = client_state.map_manager->map_render_range * RENDER_CHUNK_SIZE * CUBE_SIZE;
            world->gfx.fog_mindist = (float)(RENDER_CHUNK_SIZE * CUBE_SIZE)/2.f;

            vec3 player_move = {0};
            vec3 expected_position;
            vec3 ground_test;
            glm_vec3_sub(client_state.local_player->position, (vec3){0.f,0.1f,0.f}, ground_test);

            client_state.player_grounded = map_determine_collision_client(client_state.map_manager, ground_test);
            vec3 velocity_frame, velocity_frame_p;
            glm_vec3_copy(client_state.player_velocity, velocity_frame_p);
            glm_vec3_mul(velocity_frame_p, (vec3){world->delta_time, world->delta_time, world->delta_time}, velocity_frame);
            glm_vec3_sub(velocity_frame, player_move, player_move);

            float rooftest_height = 1.5f;
            vec3 player_move_x = {0};
            vec3 player_move_x_rooftest = {0};
            vec3 player_move_y = {0};
            vec3 player_move_y_rooftest = {0};
            vec3 player_move_z = {0};
            vec3 player_move_z_rooftest = {0};
            player_move_x[0] = player_move[0];
            player_move_x_rooftest[0] = player_move[0];
            player_move_x_rooftest[1] = rooftest_height;
            player_move_y[1] = player_move[1];
            player_move_y_rooftest[1] = player_move[1] + rooftest_height;
            player_move_z[2] = player_move[2];
            player_move_z_rooftest[2] = player_move[2];
            player_move_z_rooftest[1] = rooftest_height;

            glm_vec3_add(player_move_x, client_state.local_player->position, expected_position);
            glm_vec3_add(player_move_x_rooftest, client_state.local_player->position, ground_test);
            if(map_determine_collision_client(client_state.map_manager, expected_position) ||
            map_determine_collision_client(client_state.map_manager, ground_test))
            {
                client_state.player_velocity[0] = 0.f;      
            }
            else
                glm_vec3_copy(expected_position, client_state.local_player->position);

            glm_vec3_add(player_move_z, client_state.local_player->position, expected_position);
            glm_vec3_add(player_move_z_rooftest, client_state.local_player->position, ground_test);
            if(map_determine_collision_client(client_state.map_manager, expected_position) ||
            map_determine_collision_client(client_state.map_manager, ground_test))
            {
                client_state.player_velocity[2] = 0.f;        
            }
            else
                glm_vec3_copy(expected_position, client_state.local_player->position);

            glm_vec3_add(player_move_y, client_state.local_player->position, expected_position);
            glm_vec3_add(player_move_y_rooftest, client_state.local_player->position, ground_test);
            if(map_determine_collision_client(client_state.map_manager, expected_position) ||
            map_determine_collision_client(client_state.map_manager, ground_test))
            {
                client_state.player_velocity[1] = 0.f;        
            }
            else
                glm_vec3_copy(expected_position, client_state.local_player->position);

            vec3 vel_loss;
            float vel_loss_scalar = client_state.player_grounded ? 5.f : 1.f, vel_loss_scalar_y = 0.1f;
            glm_vec3_mul(client_state.player_velocity, (vec3){vel_loss_scalar * world->delta_time, vel_loss_scalar_y * world->delta_time, vel_loss_scalar * world->delta_time}, vel_loss);
            glm_vec3_sub(client_state.player_velocity, vel_loss, client_state.player_velocity);
            float vel_min = 0.01f;
            if(fabsf(client_state.player_velocity[0]) < vel_min)
                client_state.player_velocity[0] = 0.f;
            if(fabsf(client_state.player_velocity[2]) < vel_min)
                client_state.player_velocity[2] = 0.f;

            if(!client_state.player_grounded)
            {
                if(client_state.player_velocity[1] > -5.0)
                    client_state.player_velocity[1] -= world->delta_time * 5.f;
            }
            else
            {
                client_state.player_velocity[1] = 0.f;

                bool player_submerged = map_determine_collision_client(client_state.map_manager, client_state.local_player->position);
                while(player_submerged)
                {
                    client_state.local_player->position[1] += 0.1f * world->delta_time;
                    player_submerged = map_determine_collision_client(client_state.map_manager, client_state.local_player->position);
                }

                if(get_focus() && keys_down[GLFW_KEY_SPACE])
                {
                    glm_vec3_add(client_state.player_velocity, (vec3){0.f,3.5f,0.f}, client_state.player_velocity);         
                    glm_vec3_add(client_state.local_player->position, (vec3){0.f,0.1f,0.f}, client_state.local_player->position);
                }            
            }
            vec3 eye_position;
            glm_vec3_copy(client_state.local_player->position, eye_position);
            eye_position[1] += 0.75f;
            client_state.mouse_block_v = false;
            client_state.mouse_player = 0;
            struct ray_cast_info ray = ray_cast(&world->client, eye_position, client_state.local_player->direction, 16.f, client_state.local_player_id, true, client_state.local_player->active_weapon_type == WEAPON_BLOCK);
            if(ray.object == RAYCAST_VOXEL)
            {
                client_state.mouse_block_x = ray.real_voxel_x;
                client_state.mouse_block_y = ray.real_voxel_y;
                client_state.mouse_block_z = ray.real_voxel_z;
                client_state.mouse_block_v = true;
            }
            else if(ray.player == RAYCAST_PLAYER)
            {
                client_state.mouse_player = ray.player;
            }            

            if(get_focus())
            {
                if(client_state.local_player->active_weapon_type == WEAPON_BLOCK)
                {
                    if(floorf(mouse_state.scroll_y) != 0)
                    {
                        struct network_packet _pak; 
                        union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

                        _pak.meta.packet_type = RDM_PACKET_UPDATE_WEAPON;
                        _pak.meta.acknowledge = false;
                        _pak.meta.packet_size = sizeof(union rdm_packet_data);
                        _data->update_weapon.hotbar_id = client_state.local_player->active_hotbar_id;
                        _data->update_weapon.block_color = client_state.local_player->weapon_block_color + floorf(mouse_state.scroll_y);

                        network_transmit_packet(&world->client, &world->client.client, &_pak);
                    }
                }

                if(mouse_state.mouse_button_l)
                    weapon_trigger_fire(world, false);
                else if(mouse_state.mouse_button_r)
                    weapon_trigger_fire(world, true);

                float cam_yaw = mouse_position[0] * world->delta_time * g_key_file_get_double(world->config.key_file, "rdm2", "mouse_sensitivity", NULL);
                float cam_pitch = mouse_position[1] * world->delta_time * g_key_file_get_double(world->config.key_file, "rdm2", "mouse_sensitivity", NULL);

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

                float move_speed = client_state.player_grounded ? 25.f : 7.f;

                client_state.player_velocity[0] += cosf(world->cam.yaw*M_PI_180f) * get_input("z_axis") * world->delta_time * move_speed;
                client_state.player_velocity[2] += sinf(world->cam.yaw*M_PI_180f) * get_input("z_axis") * world->delta_time * move_speed;

                client_state.player_velocity[0] += cosf(world->cam.yaw*M_PI_180f+M_PI_2f) * get_input("x_axis") * world->delta_time * move_speed;
                client_state.player_velocity[2] += sinf(world->cam.yaw*M_PI_180f+M_PI_2f) * get_input("x_axis") * world->delta_time * move_speed;
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
                    _pak.meta.packet_size = sizeof(union rdm_packet_data);
                    glm_vec3_copy(client_state.local_player->position, _data->update_position.position);
                    glm_quat_copy(client_state.local_player->direction, _data->update_position.direction);
                    _data->update_position.player_id = client_state.local_player_id;

                    network_transmit_packet(&world->client, &world->client.client, &_pak);

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
#endif
    }
    else
    {
        world->cam.yaw = -35.f;
        world->cam.pitch = 35.f;
    }

    if(world->client_on)
    {
        if(client_state.gamemode.started)
            map_update_chunks(client_state.map_manager, world);

        gamemode_frame(&client_state.gamemode, world);
    }
}

static int __pvc_v_id = 0;
static void __player_voice_chat_ui(gpointer key, gpointer value, gpointer user_data)
{
    struct world* world = (struct world*)user_data;
    struct network_client* client = (struct network_client*)value;
    struct rdm_player* player = (struct rdm_player*)client->user_data;

    if(world->time - client->last_voice_packet < 0.1f)
    {
        vec4 oldbg_kys;
        glm_vec4_copy(world->ui->panel_background_color, oldbg_kys);
        glm_vec4_mix((vec4){0.25f,0.f,0.f,0.25f},(vec4){0.f,0.25f,0.f,0.25f},client->last_voice_packet_avg,world->ui->panel_background_color);
        ui_draw_panel(world->ui, world->gfx.screen_width - 128.f - 8.f, 255.f - (__pvc_v_id * 24.f), 128.f, 24.f, 1.f);
        ui_font2_text(world->ui, 8.f, (24.f/2.f) + (client_state.normal_font2->size_y/2.f), client_state.normal_font2, client->client_name, 0.5f);
        ui_end_panel(world->ui);
        glm_vec4_copy(oldbg_kys, world->ui->panel_background_color);

        __pvc_v_id++;
    }
}

static void rdm_frame_ui(struct world* world)
{
#ifndef HEADLESS
    if(world->client_on)
    {
        if(world->client.client.lag > 1)
            ui_font2_text(world->ui, world->gfx.screen_width/1.5, world->gfx.screen_height-24, client_state.big_font, "LAG", 1.f);
    }

    ui_font2_text(world->ui, 0.f, 0.f, client_state.big_font, "RDM2 ALPHA", 1.f);

    char dbginfo[64] = { 0 };
    if(client_state.local_player && world->state != WORLD_STATE_MAINMENU)
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

        snprintf(dbginfo,64,"%i player(s) online",g_hash_table_size(world->client.players));
        ui_font2_text(world->ui, 128.f, 0.f, client_state.normal_font, dbginfo, 1.f);

        snprintf(dbginfo,64,"%fx%fx%f %fx%fx%f",
            client_state.local_player->position[0], client_state.local_player->position[1], client_state.local_player->position[2],
            client_state.player_velocity[0], client_state.player_velocity[1], client_state.player_velocity[2]);
    }
    else if(world->state != WORLD_STATE_MAINMENU)
        snprintf(dbginfo,64,"waiting for player");
    
    ui_draw_text(world->ui, 0.f, 24.f, dbginfo, 1.f);


    if(client_state.local_player && !client_state.server_motd_dismissed)
    {
        set_focus(world->gfx.window, false);

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

        if(client_state.local_player->antagonist_data.type)
        {
            world->ui->foreground_color[1] = 0.f;
            world->ui->foreground_color[2] = 0.f;
            snprintf(dbginfo,64,"You are the %s", antagonist_name(client_state.local_player->antagonist_data.type));
            ui_font2_text(world->ui, 8.f, world->ui->current_panel->size_y / 2.f, client_state.big_font, dbginfo, 0.2f);
            world->ui->foreground_color[1] = 1.f;
            world->ui->foreground_color[2] = 1.f;
            for(int i = 0; i < sizeof(client_state.local_player->antagonist_data.objectives) / sizeof(struct objective); i++)
            {
                struct objective obj = client_state.local_player->antagonist_data.objectives[i];
                char obj_desc[512];
                objective_description(obj_desc, 512, obj);
                ui_font2_text(world->ui, 8.f, (world->ui->current_panel->size_y / 2.f) + 24.f + (i * 12.f), client_state.normal_font, obj_desc, 0.2f);
            }
        }

        bool ok_button = ui_draw_button(world->ui, world->gfx.screen_width - 64.f, world->gfx.screen_height - 56.f, 40.f, 24.f, world->gfx.white_texture, 1.0f);
        ui_font2_text(world->ui, world->gfx.screen_width - 64.f, world->gfx.screen_height - 32.f, client_state.big_font, "Ok", 0.2f);

        if(ok_button)
        {
            client_state.server_motd_dismissed = true;
            set_focus(world->gfx.window, true);
        }

        ui_end_panel(world->ui);
    }
    else if(world->state != WORLD_STATE_MAINMENU)
    {
        if(client_state.gamemode.started && client_state.local_player)
        {
            float health_pctg = (float)client_state.local_player->health / (float)client_state.local_player->max_health;
            float health_max_sz = world->gfx.screen_width / 4.f;
            float health_sz = health_pctg * health_max_sz;

            vec4 oldbg_kys;
            glm_vec4_copy(world->ui->panel_background_color, oldbg_kys);
            world->ui->panel_background_color[0] = 0.25f;
            world->ui->panel_background_color[1] = 0.f;
            world->ui->panel_background_color[2] = 0.f;
            world->ui->panel_background_color[3] = 1.f;

            ui_draw_panel(world->ui, 64.f, 88.f, health_max_sz, 24.f, 1.1f);

            world->ui->panel_background_color[0] = 0.f;
            world->ui->panel_background_color[1] = 0.25f;
            world->ui->panel_background_color[2] = 0.f;
            world->ui->panel_background_color[3] = 1.f;
            ui_draw_panel(world->ui, 0.f, 0.f, health_sz, 24.f, 1.f);
            snprintf(dbginfo,64,"%0.0f%%", health_pctg * 100.f);
            ui_font2_text(world->ui, health_sz / 2.f - ui_font2_text_len(client_state.normal_font, dbginfo) / 2.f, 16.f, client_state.normal_font, dbginfo, 0.7f);
            ui_end_panel(world->ui);            
            ui_end_panel(world->ui);

            glm_vec4_copy(oldbg_kys, world->ui->panel_background_color);

            int img_size = 32;
            if(client_state.local_player->weapon_ammos[client_state.local_player->active_weapon_type] != -1)
            {
                snprintf(dbginfo,64,"%i", client_state.local_player->weapon_ammos[client_state.local_player->active_weapon_type]);
                ui_font2_text(world->ui, world->gfx.screen_width - 64.f - ui_font2_text_len(client_state.big_font, dbginfo), 64.f + img_size, client_state.big_font, dbginfo, 1.f);
            }

            if(client_state.local_player->active_weapon_type == WEAPON_BLOCK)
            {
                world->ui->panel_background_color[3] = 1.f;
                unsigned char c = client_state.local_player->weapon_block_color;
                map_color_to_rgb(c, world->ui->panel_background_color);
                ui_draw_panel(world->ui, world->gfx.screen_width - 32, 32, 32, 32, 1.f);
                char col_tx[16];
                snprintf(col_tx,16,"%u",c);
                ui_font2_text(world->ui, 0.f, 12.f, client_state.normal_font, col_tx, 0.5f);
                ui_end_panel(world->ui);
                glm_vec4_copy(oldbg_kys, world->ui->panel_background_color);
            }
            
            glm_vec4_copy(oldbg_kys, world->ui->panel_background_color);

            for(int i = 0; i < 9; i++)
            {   
                if(client_state.local_player->weapon_hotbar[i] == -1)
                    continue;             
                enum weapon_type weapon_type = client_state.local_player->inventory[client_state.local_player->weapon_hotbar[i]];
                if(weapon_type == -1)
                    continue;
                if(keys_down[GLFW_KEY_0 + i + 1])
                {
                    struct network_packet _pak; 
                    union rdm_packet_data* _data = (union rdm_packet_data*)&_pak.packet.data;

                    _pak.meta.packet_type = RDM_PACKET_UPDATE_WEAPON;
                    _pak.meta.acknowledge = false;
                    _pak.meta.packet_size = sizeof(union rdm_packet_data);
                    _data->update_weapon.hotbar_id = i;
                    _data->update_weapon.block_color = client_state.local_player->weapon_block_color;

                    network_transmit_packet(&world->client, &world->client.client, &_pak);
                }
                float pos_off = 0.f;
                float pos_sep = 0.f;
                if(client_state.local_player->active_hotbar_id == i)
                    pos_off += sinf(world->time * 4.f) * 4.f;
                ui_draw_image(world->ui, world->gfx.screen_width - 64.f - (9 * img_size) + (i * (img_size + pos_sep)), 64.f + img_size + pos_off, img_size, img_size, client_state.weapon_icon_textures[weapon_type], 1.f);
                snprintf(dbginfo,64,"%i", i + 1);
                ui_font2_text(world->ui, world->gfx.screen_width - 64.f - (9 * img_size) + (i * (img_size + pos_sep)) + (img_size - 12.f), 64.f + img_size + pos_off - 12.f, client_state.normal_font, dbginfo, 0.8f);
                if(client_state.local_player->weapon_ammos[weapon_type] != -1)
                {
                    snprintf(dbginfo,64,"%i", client_state.local_player->weapon_ammos[weapon_type]);
                    ui_font2_text(world->ui, world->gfx.screen_width - 64.f - (9 * img_size) + (i * img_size), 64.f + pos_off, client_state.normal_font, dbginfo, 0.8f);
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
        {            
            ui_draw_panel(world->ui, world->gfx.screen_width / 2 - 128.f, world->gfx.screen_height / 2 + 150.f, 255.f, 300.f, 1.f);
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
                snprintf(dbginfo,64,"%i - %s, ping %fmsec, pd %p", i, client->client_name, client->lag * 1000.0, player);
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

    if(world->state == WORLD_STATE_MAINMENU)
    {
        if(client_state.logo_title > world->time)
        {
            float old_alpha = world->ui->foreground_color[3];
            world->ui->foreground_color[3] = fabsf(world->time - client_state.logo_title);
            ui_draw_image(world->ui, 0.f, world->gfx.screen_height, world->gfx.screen_width, world->gfx.screen_height, client_state.logo_title_tex, 0.5f);
            world->ui->foreground_color[3] = old_alpha;
        }
        else
        {            
            float old_alpha = world->ui->foreground_color[3];
            world->ui->foreground_color[3] = MIN(1,fabsf(client_state.logo_title - world->time));
            if(client_state.server_selection_panel)
            {            
                ui_draw_panel(world->ui, 8.f, world->gfx.screen_height-8.f, world->gfx.screen_width-16.f, world->gfx.screen_height-16.f, 0.4f);
                ui_font2_text(world->ui, 8.f, 24.f, client_state.big_font, "Multiplayer Servers", 0.2f);
                snprintf(dbginfo, 64, "%s", world->client.http_client.motd);
                ui_font2_text(world->ui, ui_font2_text_len(client_state.big_font, "Multiplayer Servers") + 8.f + 16.f, 12.f, client_state.normal_font, dbginfo, 0.2f);
                snprintf(dbginfo, 64, "%i servers online", client_state.server_list->len);
                ui_font2_text(world->ui, ui_font2_text_len(client_state.big_font, "Multiplayer Servers") + 8.f + 16.f, 24.f, client_state.normal_font, dbginfo, 0.2f);

                bool ok_button = ui_draw_button(world->ui, world->gfx.screen_width - 88.f, world->gfx.screen_height - 56.f, 60.f, 24.f, world->gfx.white_texture, 1.0f);
                ui_font2_text(world->ui, world->gfx.screen_width - 88.f, world->gfx.screen_height - 32.f, client_state.big_font, "Close", 0.2f);

                if(ok_button)
                    client_state.server_selection_panel = false;

                bool last_button = ui_draw_button(world->ui, world->gfx.screen_width - 88.f, world->gfx.screen_height - 72.f, 60.f, 24.f, world->gfx.white_texture, 1.0f);
                ui_font2_text(world->ui, world->gfx.screen_width - 88.f, world->gfx.screen_height - 48.f, client_state.big_font, "Last", 0.2f);

                if(last_button)
                {
                    client_state.server_selection_panel = false;
                    network_connect(&world->client,
                        g_key_file_get_string(world->config.key_file, "rdm2", "last_server_ip", NULL),
                        g_key_file_get_integer(world->config.key_file, "rdm2", "last_server_port", NULL)
                    );

                    world_start_game(world);

                    if(client_state.playing_music)
                        stop_snd(client_state.playing_music);
                    client_state.playing_music = 0;
                }

                bool local_button = ui_draw_button(world->ui, world->gfx.screen_width - 88.f, world->gfx.screen_height - 88.f, 60.f, 24.f, world->gfx.white_texture, 1.0f);
                ui_font2_text(world->ui, world->gfx.screen_width - 88.f, world->gfx.screen_height - 64.f, client_state.big_font, "Local", 0.2f);

                if(local_button)
                {
                    client_state.server_selection_panel = false;
                    network_connect(&world->client,
                        "127.0.0.1",
                        g_key_file_get_integer(world->config.key_file, "sglthing", "network_port", NULL)
                    );

                    world_start_game(world);

                    if(client_state.playing_music)
                        stop_snd(client_state.playing_music);
                    client_state.playing_music = 0;
                }

                bool refresh_button = ui_draw_button(world->ui, world->gfx.screen_width - 88.f, world->gfx.screen_height - 104.f, 60.f, 24.f, world->gfx.white_texture, 1.0f);
                ui_font2_text(world->ui, world->gfx.screen_width - 88.f, world->gfx.screen_height - 80.f, client_state.big_font, "Refresh", 0.2f);

                if(refresh_button)
                {
                    http_get_servers(&world->client.http_client, "rdm2-ng", client_state.server_list);
                }

                vec4 oldbg_kys;
                glm_vec4_copy(world->ui->panel_background_color, oldbg_kys);
                world->ui->panel_background_color[3] = 1.0f;
                ui_draw_panel(world->ui, 16.f, 32.f, world->ui->current_panel->size_x - 32.f, 12.f, 0.2f);
                float half = world->ui->current_panel->size_x / 3.0f;
                ui_font2_text(world->ui, 0.f, 12.f, client_state.normal_font, "Name", 0.1f);
                ui_font2_text(world->ui, half, 12.f, client_state.normal_font, "Message of the Day", 0.1f);
                ui_end_panel(world->ui);

                world->ui->panel_background_color[0] = 0.2f;
                world->ui->panel_background_color[1] = 0.2f;
                world->ui->panel_background_color[2] = 0.2f;

                for(int i = 0; i < client_state.server_list->len; i++)
                {
                    struct http_server server = g_array_index(client_state.server_list, struct http_server, i);
                    ui_draw_panel(world->ui, 16.f, (i * 13.f) + 44.f, world->ui->current_panel->size_x - 32.f, 12.f, 0.2f);
                    ui_font2_text(world->ui, 0.f, 12.f, client_state.normal_font, server.name, 0.1f);
                    ui_font2_text(world->ui, half, 12.f, client_state.normal_font, server.desc, 0.1f);
                    bool connect_button = ui_draw_button(world->ui, world->ui->current_panel->size_x - 64.f, 0.f, 64.f, 12.f, world->gfx.white_texture, 1.f);
                    ui_font2_text(world->ui, world->ui->current_panel->size_x - 64.f, 12.f, client_state.normal_font, "Connect", 0.1f);
                    ui_end_panel(world->ui);

                    if(connect_button)
                    {
                        network_connect(&world->client,
                            server.ip,
                            server.port
                        );

                        world_start_game(world);

                        if(client_state.playing_music)
                            stop_snd(client_state.playing_music);
                        client_state.playing_music = 0;
                    }
                }

                glm_vec4_copy(oldbg_kys, world->ui->panel_background_color);

                ui_end_panel(world->ui);
            }
            else
            {
                float mm_y = world->gfx.screen_height / 2.f;
        #define MAIN_MENU_ENTRY(f,e,c)                                                                                  \
                {                                                                                                       \
                    float t_len = ui_font2_text_len(f, e);                                                              \
                    bool b = ui_draw_button(world->ui, 45.f, mm_y, t_len, f->size_y, world->gfx.alpha_texture, 1.f);    \
                    ui_font2_text(world->ui, 45.f, mm_y - f->size_y, f, e, 0.5f);                                       \
                    mm_y -= f->size_y;                                                                                  \
                    if(b) c;                                                                                            \
                }
                MAIN_MENU_ENTRY(client_state.big_font, "RDM2", {});
                mm_y -= client_state.normal_font2->size_y;
                MAIN_MENU_ENTRY(client_state.normal_font2, "Multiplayer", {
                    http_get_servers(&world->client.http_client, "rdm2-ng", client_state.server_list);
                    http_update_motd(&world->client.http_client);
                    client_state.server_selection_panel = true;
                });
                MAIN_MENU_ENTRY(client_state.normal_font2, "Quit", glfwSetWindowShouldClose(world->gfx.window, 1));

                if(!world->client.http_client.login)
                {                    
                    ui_font2_text(world->ui, world->gfx.screen_width / 2.f, 60.f, client_state.normal_font, "You are not logged in. Sign up at https://sgl.endoh.ca/", 1.f);
                }
                else
                {
                    ui_draw_panel(world->ui, world->gfx.screen_width / 2.f, world->gfx.screen_height / 3.f, world->gfx.screen_width / 2.f - 8.f, world->gfx.screen_height / 3.f - 8.f, 1.f);
                    snprintf(dbginfo, 64, "Welcome to RDM2, %s.", client_state.local_http_user.user_username);
                    ui_font2_text(world->ui, 8.f, 24.f, client_state.normal_font2, dbginfo, 0.5f);
                    snprintf(dbginfo, 64, "%i coins", client_state.local_http_user.money);
                    world->ui->foreground_color[2] = 0.f;
                    ui_font2_text(world->ui, 8.f, 24.f + client_state.normal_font2->size_y, client_state.normal_font, dbginfo, 0.5f);
                    world->ui->foreground_color[2] = 1.f;
                    ui_end_panel(world->ui);
                }
            }            
            world->ui->foreground_color[3] = old_alpha;
        }
    }

    if(client_state.mouse_player)
    {
        char player_hover[255];
        float health_pctg = (float)client_state.mouse_player->health / (float)client_state.mouse_player->max_health;
        snprintf(player_hover, 255, "%s (%.0f%%)", client_state.mouse_player->client->client_name, health_pctg * 100.f);
        ui_font2_text(world->ui, (world->gfx.screen_width / 2.f) - (ui_font2_text_len(client_state.normal_font2, player_hover) / 2.f), (world->gfx.screen_height / 2.f) - 32.f, client_state.normal_font2, player_hover, 1.f);
    }

    m2_draw_dbg((void*)world);

    __pvc_v_id = 0;
    g_hash_table_foreach(world->client.players, __player_voice_chat_ui, world);
#endif
}

static void __player_frame(gpointer key, gpointer value, gpointer user_data)
{
    struct world* world = (struct world*)user_data;
    struct network_client* client = (struct network_client*)value;
    struct rdm_player* player = (struct rdm_player*)client->user_data;

}

static void __player_render(gpointer key, gpointer value, gpointer user_data)
{
#ifndef HEADLESS
    struct world* world = (struct world*)user_data;
    struct network_client* client = (struct network_client*)value;
    struct rdm_player* player = (struct rdm_player*)client->user_data;
    bool local_player = client->player_id == client_state.local_player_id;

    vec4 team_colors[__TEAM_MAX] = {
        { 0.5f, 0.5f, 0.5f, 0.5f }, // neutral
        { 1.0f, 0.2f, 0.2f, 0.5f }, // red
        { 0.2f, 0.2f, 1.0f, 0.5f }, // blue
        { 0.5f, 0.5f, 0.5f, 0.5f }, // n/a
        { 0.2f, 0.2f, 0.2f, 0.5f }, // spectator
    };

    mat4 player_matrix, player_rotated_matrix;
    glm_mat4_identity(player_matrix);
    glm_translate(player_matrix, player->position);
    glm_translate(player_matrix, (vec3){0.f,-1.f,0.f});
    glm_quat_rotate(player_matrix, player->direction, player_rotated_matrix);

    if(player->health == 0)
        glm_rotate_x(player_rotated_matrix, 1.f, player_rotated_matrix);

    if(client->player_id != client_state.local_player_id || world->gfx.shadow_pass)
    {
        if(client_state.client_stencil)
        {
            vec4* team_color = &team_colors[player->team];
            glm_scale(player_rotated_matrix, (vec3){1.1f,1.1f,1.1f});
            glUniform4fv(glGetUniformLocation(client_state.stencil_shader,"stencil_color"), 1, *team_color);
            glGetError(); // pop possible error
            world_draw_model(world, client_state.rdm_guy, client_state.stencil_shader, player_rotated_matrix, true);            
        }
        else
        {
            world_draw_model(world, client_state.rdm_guy, client_state.object_shader, player_rotated_matrix, true);
        }
    }
    struct model* weapon_model = client_state.weapons[player->active_weapon_type];
    if(weapon_model)
    {
        mat4 weapon_matrix;
        glm_mat4_identity(weapon_matrix);
        glm_translate(weapon_matrix, player->position);
        glm_quat_rotate(weapon_matrix, player->direction, weapon_matrix);
        glm_translate(weapon_matrix, (vec3){-0.3f,0.5f,0.1f});

        if(client_state.client_stencil)
        {
            vec4* team_color = &team_colors[player->team];
            glm_scale(weapon_matrix, (vec3){1.1f,1.1f,1.1f});
            glUniform4fv(glGetUniformLocation(client_state.stencil_shader,"stencil_color"), 1, *team_color);
            glGetError(); // pop possible error
            world_draw_model(world, weapon_model, client_state.stencil_shader, weapon_matrix, true);            
        }
        else
        {
            if(!world->gfx.shadow_pass && player->active_weapon_type == WEAPON_BLOCK)
            {
                vec3 block_color;
                map_color_to_rgb(player->weapon_block_color, block_color);
                sglc(glUseProgram(client_state.object_shader));
                sglc(glUniform4f(glGetUniformLocation(client_state.object_shader,"color"), block_color[0], block_color[1], block_color[2], 1.0f));
            }
            world_draw_model(world, weapon_model, client_state.object_shader, weapon_matrix, true);
            if(!world->gfx.shadow_pass)
                sglc(glUniform4f(glGetUniformLocation(client_state.object_shader,"color"), 1.0f, 1.0f, 1.0f, 1.0f));
        }
    }

    if(world->time - client->last_voice_packet < 0.1f)
    {
        mat4 vc_bubble_mat;
        glm_mat4_identity(vc_bubble_mat);
        glm_translate(vc_bubble_mat, player->position);
        glm_translate_y(vc_bubble_mat, 2.5f);
        glm_scale(vc_bubble_mat, (vec3){0.5f,0.5f,0.5f});
        glm_rotate_y(vc_bubble_mat, world->time, vc_bubble_mat);
        glm_translate_x(vc_bubble_mat, 0.7f);
        world_draw_model(world, client_state.vc_bubble, client_state.object_shader, vc_bubble_mat, true);
    }
#endif
}

static void rdm_frame_render(struct world* world)
{
#ifndef HEADLESS
    if(!world->gfx.shadow_pass)
    {
        sglc(glDepthMask(GL_FALSE));  

        mat4 skybox_mat;
        glm_mat4_identity(skybox_mat);
        glm_translate(skybox_mat, world->cam.position);
        //world_draw_model(world, client_state.skybox, client_state.skybox_shader, skybox_mat, true);

        glm_translate_y(skybox_mat, 0.2);
        glm_scale(skybox_mat, (vec3){15.f,1.f,15.f});
        world_draw_model(world, client_state.cloud_layer, client_state.cloud_layer_shader, skybox_mat, true);

        sglc(glDepthMask(GL_LESS));  
    }

    if(world->state == WORLD_STATE_MAINMENU)
    {
        mat4 menu_player_mat;
        glm_mat4_identity(menu_player_mat);
        glm_translate(menu_player_mat, (vec3){2.5f, 0.f, -1.0f});
        glm_rotate_y(menu_player_mat, world->time / 2.f, menu_player_mat);
        world_draw_model(world, client_state.rdm_guy, client_state.object_shader, menu_player_mat, true);
    }

    // mat4 test_cube;
    // glm_mat4_identity(test_cube);
    // glm_translate(test_cube,server_state.last_position);
    // world_draw_model(world, client_state.map_manager->cube, client_state.object_shader, test_cube, false);

    map_render_chunks(world, client_state.map_manager);

    g_hash_table_foreach(world->client.players, __player_render, world);

    if(!world->gfx.shadow_pass)
    {
        if(client_state.mouse_block_v)
        {
            mat4 block_selection_matrix;
            glm_mat4_identity(block_selection_matrix);
            glm_translate(block_selection_matrix, (vec3){
                client_state.mouse_block_x,
                client_state.mouse_block_y,
                client_state.mouse_block_z,
            });
            if(client_state.local_player->active_weapon_type == WEAPON_SHOVEL)
            {
                glUniform4f(glGetUniformLocation(client_state.object_shader,"color"), 0.f, 0.f, 0.f, 0.5f);
                glm_scale(block_selection_matrix, (vec3){1.1f,1.1f,1.1f});
            }
            else
            {
                vec3 block_color;
                map_color_to_rgb(client_state.local_player->weapon_block_color, block_color);
                glUniform4f(glGetUniformLocation(client_state.object_shader,"color"), block_color[0], block_color[1], block_color[2], fabsf(sinf(world->time*5.f)));
            }
            world_draw_model(world, client_state.map_manager->cube, client_state.object_shader, block_selection_matrix, false);
            glUniform4f(glGetUniformLocation(client_state.object_shader,"color"), 1.f, 1.f, 1.f, 1.f);
        }
    }
#endif
}

void sglthing_init_api(struct world* world)
{
    printf("rdm2: hello world\n");
#ifndef HEADLESS
    glfwSetWindowTitle(world->gfx.window, "RDM2");

    if(world->client_on)
    {
        if(world->state == WORLD_STATE_MAINMENU)
        {
            world->client.http_client.server = false;
            http_create(&world->client.http_client, SGLAPI_BASE);
            client_state.local_http_user = http_get_userdata(&world->client.http_client, world->client.http_client.sessionkey);
        }

        client_state.server_list = g_array_new(true, true, sizeof(struct http_server));

        load_snd("rdm2/sound/ak47_shoot.ogg");
        load_snd("rdm2/sound/ricochet_ground.ogg");
        load_snd("rdm2/sound/ouch.ogg");

        world->ui->ui_font = ui_load_font("uiassets/font2.png",8,16,32,16);

        client_state.big_font = ui_load_font2(world->ui, "uiassets/impact.ttf",0,24);
        client_state.normal_font = ui_load_font2(world->ui, "uiassets/arial.ttf",0,12);
        client_state.normal_font2 = ui_load_font2(world->ui, "uiassets/arialbd.ttf",0,18);

        load_texture("rdm2/intro_logo.png");
        client_state.logo_title_tex = get_texture("rdm2/intro_logo.png");

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

        load_model("vcbubble.obj");
        client_state.vc_bubble = get_model("vcbubble.obj");

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

        v = compile_shader("shaders/normal.vs", GL_VERTEX_SHADER);
        f = compile_shader("rdm2/shaders/stencil.fs", GL_FRAGMENT_SHADER);
        client_state.stencil_shader = link_program(v, f);

        client_state.lobby_music = load_snd("rdm2/music/lobby0.mp3");
        client_state.lobby_music->loop = true;
        client_state.lobby_music->multiplier = 0.1f;
        if(world->state == WORLD_STATE_MAINMENU)
            client_state.playing_music = play_snd2(client_state.lobby_music);
        else
            client_state.playing_music = 0;
        client_state.roundstart_sound = load_snd("rdm2/sound/round_start.ogg");
        client_state.roundstart_sound->loop = false;
    }
#endif 

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
    world->cam.position[1] = 0.f;
    world->cam.lock = true;
    world->gfx.far_boundary = 1000.f;
    world->gfx.fog_mindist = RENDER_CHUNK_SIZE * CUBE_SIZE;

    client_state.logo_title = world->time + 5.f;

    glm_vec3_zero(client_state.player_velocity);

    input_lock_tab = true;

    if(world->client_on)
    {
        client_state.server_motd_dismissed = false;
        client_state.local_player = NULL;

        client_state.map_manager = (struct map_manager*)malloc2(sizeof(struct map_manager));
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