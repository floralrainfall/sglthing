#include <sglthing/sglthing.h>
#include <sglthing/world.h>
#include <sglthing/net.h>
#include <sglthing/model.h>
#include <sglthing/texture.h>
#include <sglthing/shader.h>
#include <sglthing/keyboard.h>
#include <sglthing/animator.h>
#include <sglthing/light.h>
#include <sglthing/io.h>
#ifndef HEADLESS
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif
#include <stdlib.h>
#include "yaal.h"
#include "util.h"
#include "netmgr.h"
#include "map.h"

struct yaal_state yaal_state;
struct yaal_state server_state;

struct model* map_model;

static void __player_iter(gpointer key, gpointer value, gpointer user)
{
    struct network* network = (struct network*)user;
    struct network_client* client = NULL;
    if(network->mode == NETWORKMODE_SERVER)
        client = network_get_player(network, client);
    else
        client = (struct network_client*)value;
    if(!client)
        return;
    struct player* player = (struct player*)client->user_data;
    if(network->mode == NETWORKMODE_CLIENT)
    {
        if(player)
        {
            animator_update(&player->animator, network->world->delta_time);
        }
    }
    else
    {

    }
}

static void __sglthing_frame(struct world* world)
{
    if(world->client.status == NETWORKSTATUS_CONNECTED)
    {
        if(!yaal_state.current_player && yaal_state.player_id != -1)
        {
            yaal_state.current_player = g_hash_table_lookup(world->client.players, &yaal_state.player_id);
            if(yaal_state.current_player)
                printf("yaal: ok we are %p %i\n", yaal_state.current_player, yaal_state.player_id);
        }
        if(yaal_state.mode == YAAL_STATE_GAME)
        {
            g_hash_table_foreach(world->client.players, __player_iter, (gpointer)&world->client);

            if(yaal_state.current_player)
            {
                mat4 m, mvp;
                vec3 dm;
                glm_mat4_identity(m);
                glm_mat4_mul(world->vp, m, mvp);
                glm_project(yaal_state.current_player->old_position, mvp, world->viewport, dm);
                glm_unproject((vec3){mouse_position[0],world->gfx.screen_height-mouse_position[1],dm[2]}, world->vp, world->viewport, yaal_state.aiming_arrow_position);
                yaal_state.aiming_arrow_position[1] = 0.f;
                
                yaal_state.current_player->map_tile_x = (int)roundf(yaal_state.current_player->old_position[0] / (MAP_TILE_SIZE));
                yaal_state.current_player->map_tile_y = (int)roundf(yaal_state.current_player->old_position[2] / (MAP_TILE_SIZE));

                light_update(world->render_area, yaal_state.current_player->old_position);

                glm_vec3_copy(yaal_state.current_player->old_position, world->gfx.sun_position);
                if(yaal_state.current_player->combat_mode)
                {
                    world->cam.position[0] = yaal_state.current_player->old_position[0];
                    world->cam.position[1] = yaal_state.current_player->old_position[1] + 20.f;
                    world->cam.position[2] = yaal_state.current_player->old_position[2] - 2.f;
                    world->cam.yaw = 90.f;
                    world->cam.pitch = -90.f;
                }
                else
                {
                    world->cam.position[0] = yaal_state.current_player->old_position[0] - 15.f;
                    world->cam.position[1] = yaal_state.current_player->old_position[1] + 20.f;
                    world->cam.position[2] = yaal_state.current_player->old_position[2] - 15.f;
                    world->cam.yaw = 45.f;
                    world->cam.pitch = -45.f;
                }
                
                vec3 pot_move, new_vec;
                pot_move[0] = -get_input("x_axis") * world->delta_time * 3.f;
                pot_move[1] = 0.f;
                pot_move[2] = get_input("z_axis") * world->delta_time * 3.f;
                glm_vec3_add(pot_move, yaal_state.current_player->player_position, new_vec);

                int map_tile_x = (int)roundf(new_vec[0] / (MAP_TILE_SIZE));
                int map_tile_y = (int)roundf(new_vec[2] / (MAP_TILE_SIZE));
                bool collide = determine_tile_collision(yaal_state.map_data[map_tile_x][map_tile_y]);
                if(!collide)
                    glm_vec3_copy(new_vec, yaal_state.current_player->player_position);
            }


            world->gfx.sun_direction[0] = -0.5f;
            world->gfx.sun_direction[1] = 0.5f;
            world->gfx.sun_direction[2] = 0.5f;

            world->gfx.clear_color[0] = 0.000f;
            world->gfx.clear_color[1] = 0.000f;
            world->gfx.clear_color[2] = 0.003f;

            world->gfx.fog_color[0] = world->gfx.clear_color[0];
            world->gfx.fog_color[1] = world->gfx.clear_color[1];
            world->gfx.fog_color[2] = world->gfx.clear_color[2];
        }
        else if(yaal_state.mode == YAAL_STATE_MENU)
        {
            world->gfx.clear_color[0] = 0.375f;
            world->gfx.clear_color[1] = 0.649f;
            world->gfx.clear_color[2] = 0.932f;

            world->gfx.fog_color[0] = world->gfx.clear_color[0];
            world->gfx.fog_color[1] = world->gfx.clear_color[1];
            world->gfx.fog_color[2] = world->gfx.clear_color[2];


            world->gfx.sun_direction[0] = -0.2f;
            world->gfx.sun_direction[1] = 0.9f;
            world->gfx.sun_direction[2] = 0.4f;
        }
    }

    if(world->server.status == NETWORKSTATUS_CONNECTED)
    {
        g_hash_table_foreach(world->server.players, __player_iter, (gpointer)&world->server);        
    }

    tick_fx(world, &yaal_state.fx_manager);
    tick_fx(world, &server_state.fx_manager);
}

static void __player_ui_list_render(gpointer key, gpointer value, gpointer user)
{
    struct world* world = (struct world*)user;
    struct network_client* client = (struct network_client*)value;
    struct player* player = (struct player*)client->user_data;

    if(player)
    {
        char tx[256];
        snprintf(tx,256,"%-64s %4.1fms", client->client_name, client->lag*1000.0);
        ui_draw_text(world->ui, 0.f, world->gfx.screen_height - (yaal_state.last_player_list_id * 64.f) - 96.f, tx, 0.1f);
        yaal_state.last_player_list_id++;
    }
}

static void __player_render(gpointer key, gpointer value, gpointer user)
{
    struct world* world = (struct world*)user;
    struct network_client* client = (struct network_client*)value;
    struct player* player = (struct player*)client->user_data;

    if(player)
    {
        if(player->level_id != yaal_state.current_level_id)
            return;

        char nameplate_txt[256];
        snprintf(nameplate_txt, 256, "%s", client->client_name, player->animator.current_time);
        if(yaal_state.player_model)
        {
            mat4 render_matrix;
            glm_mat4_copy(player->model_matrix, render_matrix);
            glm_scale(render_matrix, (vec3){0.01f,0.01f,0.01f}); 
            if(player == yaal_state.current_player)
            {
                vec3 direction;
                glm_vec3_sub(player->old_position, yaal_state.aiming_arrow_position,  direction);
                glm_quat_for(direction, (vec3){0.f,1.f,0.f}, player->rotation_versor);
                glm_quat_rotate(render_matrix, player->rotation_versor, render_matrix);     
            }
            else
            {
                glm_quat_rotate(render_matrix, player->rotation_versor, render_matrix);    
            }
            sglc(glUseProgram(yaal_state.player_shader));
            sglc(glUniform4f(glGetUniformLocation(yaal_state.player_shader,"color"), client->player_color_r, client->player_color_g, client->player_color_b, 1.0));
            animator_set_bone_uniform_matrices(&player->animator, world->gfx.shadow_pass?world->gfx.lighting_shader:yaal_state.player_shader);
            world_draw_model(world, yaal_state.player_model, yaal_state.player_shader, render_matrix, true);
        }

        world->ui->foreground_color[0] = player->client->player_color_r;
        world->ui->foreground_color[1] = player->client->player_color_g;
        world->ui->foreground_color[2] = player->client->player_color_b;
        world->ui->background_color[3] = 0.f;
        world->ui->persist = true;
        if(!world->gfx.shadow_pass)
            ui_draw_text_3d(world->ui, world->viewport, world->cam.position, world->cam.front, (vec3){0.f,3.f,0.f}, world->cam.fov, player->model_matrix, world->vp, nameplate_txt);
        chat_render_player(world, yaal_state.chat, player, player->model_matrix);
        world->ui->persist = false;
    }
}

static void __sglthing_frame_render(struct world* world)
{
    world->render_area = yaal_state.area;

    if(yaal_state.mode == YAAL_STATE_GAME)
    {
        map_render(world, yaal_state.map_data);

        vec4 oldfg, oldbg;
        glm_vec4_copy(world->ui->foreground_color, oldfg);
        glm_vec4_copy(world->ui->background_color, oldbg);
        g_hash_table_foreach(world->client.players, __player_render, (gpointer)world);        
        glm_vec4_copy(oldfg, world->ui->foreground_color);
        glm_vec4_copy(oldbg, world->ui->background_color);
        
        if(yaal_state.current_aiming_action != -1)
        {
            mat4 arrow_matrix;
            glm_mat4_identity(arrow_matrix);
            glm_translate(arrow_matrix, yaal_state.aiming_arrow_position);
            if(!world->gfx.shadow_pass)
                sglc(glDisable(GL_DEPTH_TEST));
            sglc(glUseProgram(yaal_state.object_shader));
            sglc(glActiveTexture(GL_TEXTURE0));
            sglc(glBindTexture(GL_TEXTURE_2D, world->gfx.white_texture));                
            sglc(glUniform4f(glGetUniformLocation(yaal_state.object_shader,"color"), 0.0, 1.0, 0.0, 0.5));
            world_draw_model(world, yaal_state.arrow_model, yaal_state.object_shader, arrow_matrix, false);
            if(!world->gfx.shadow_pass)
            {
                particles_add(&yaal_state.particles, (struct particle){
                    .position = {yaal_state.aiming_arrow_position[0], yaal_state.aiming_arrow_position[1]+0.5f, yaal_state.aiming_arrow_position[2]}, 
                    .velocity = {g_random_double_range(-2.5f,2.5f),g_random_double_range(-15.1f,1.5f),g_random_double_range(-2.5f,2.5f)}, 
                    .life = 7.5f, 
                    .color = {0,1,0,1}, 
                    .texture = yaal_state.select_particle_tex, 
                    .dim = 2.5f
                });
                sglc(glEnable(GL_DEPTH_TEST));
            }
        }

        render_fx(world,&yaal_state.fx_manager);

        if(!world->gfx.shadow_pass)
        {
            particles_render(world, &yaal_state.particles);
        }
    }
    else if(yaal_state.mode == YAAL_STATE_MENU)
    {
        vec3 position = {5.f,1.f,-0.5f};
        mat4 logo_matrix;
        glm_mat4_identity(logo_matrix);
        glm_translate(logo_matrix, position);
        versor rotation;
        vec3 direction;
        glm_vec3_sub(position, (vec3){sinf(world->time)*4.f,sinf(world->time),cosf(world->time)*4.f},  direction);
        glm_quat_for(direction, (vec3){0.f,1.f,0.f}, rotation);
        glm_quat_rotate(logo_matrix, rotation, logo_matrix);
        world_draw_model(world, yaal_state.menu_yaal_model, yaal_state.object_shader, logo_matrix, true);

        mat4 ground_matrix;
        glm_mat4_identity(ground_matrix);
        glm_scale(ground_matrix,(vec3){1000.f,1.f,1000.f});
        glm_translate(ground_matrix,(vec3){0.f,-2.f,0.f});

        world_draw_model(world, yaal_state.map_tiles[1], yaal_state.object_shader, ground_matrix, true);

        mat4 city_matrix, city_rotation_matrix;
        glm_mat4_identity(city_matrix);
        glm_translate(city_matrix,(vec3){30.f,-1.f,1.f});
        glm_euler((vec3){0.f,2.5f,0.f},city_rotation_matrix);
        glm_mat4_mul(city_matrix, city_rotation_matrix, city_matrix);

        world_draw_model(world, yaal_state.menu_city_model, yaal_state.object_shader, city_matrix, true);
    }
}

static void __sglthing_frame_ui(struct world* world)
{
    struct ui_data* ui = world->ui;

    switch(yaal_state.mode)
    {
        case YAAL_STATE_MENU:
            ui_draw_panel(world->ui, 0.f, 48.f, world->gfx.screen_width, 48.f, 2.f);
            {
                if(ui_draw_button(world->ui, world->gfx.screen_width-80, 8.f, 64.f, 32.f, yaal_state.player_play_button_tex, 1.f))
                {
                    yaal_state.mode = YAAL_STATE_GAME;
                }

                ui_draw_text(world->ui, 8.f, 16.f, "Welcome to YaalOnline", 1.f);
                char server_info[64];
                int player_count = g_hash_table_size(world->client.players);
                snprintf(server_info, 64, "%i players online", player_count);
                ui_draw_text(world->ui, 8.f, 32.f, server_info, 1.f);
                ui_draw_text(world->ui, 8.f, 48.f, world->client.server_motd, 1.f);
                if(ui_draw_button(world->ui, world->gfx.screen_width-80.f, -8.f, 64.f, 8.f, yaal_state.player_menu_button_tex, 1.f))
                {
                    yaal_state.player_menu_open = true;
                }
            }
            ui_end_panel(world->ui);
            break; 
        case YAAL_STATE_GAME:
            char txinfo[256];
            if(yaal_state.current_player)
            {
                if(world->debug_mode)
                    snprintf(txinfo,256,"P=(%f,%f,%f), O=(%f,%f,%f)\nplayer id = %i, M=(%i,%i), %s\n",
                        yaal_state.current_player->player_position[0],
                        yaal_state.current_player->player_position[1],
                        yaal_state.current_player->player_position[2],
                        yaal_state.current_player->old_position[0],
                        yaal_state.current_player->old_position[1],
                        yaal_state.current_player->old_position[2],
                        yaal_state.current_player->player_id,
                        yaal_state.current_player->map_tile_x,
                        yaal_state.current_player->map_tile_y,
                        yaal_state.map_downloading?"map is downloading":"no map is being downloaded");
            }
            else
            {
                snprintf(txinfo,256,"Waiting for player (%i)", yaal_state.player_id);
            }

        #ifndef HEADLESS
            if(!keys_down[GLFW_KEY_EQUAL])
                chat_render(world, yaal_state.chat);
        #endif

            vec4 oldfg, oldbg;
            glm_vec4_copy(world->ui->foreground_color, oldfg);
            glm_vec4_copy(world->ui->background_color, oldbg);

            if(yaal_state.show_rpg_message)
            {
                int s = strlen(yaal_state.rpg_message) + 4;
                int pos = (world->gfx.screen_width/2)-((s*8)/2);
                ui_draw_text(world->ui, pos, 16*8.f, yaal_state.rpg_message, 1.f);
                if(ui_draw_button(world->ui, pos+8.f*strlen(yaal_state.rpg_message), 16*9.f, 32.f, 16.f, yaal_state.player_ok_button_tex, 1.f))
                {
                    yaal_state.show_rpg_message = false;
                }
            }

            render_ui_fx(world, &yaal_state.fx_manager);

            glm_vec4_copy(oldfg, world->ui->foreground_color);
            glm_vec4_copy(oldbg, world->ui->background_color);

            world->ui->background_color[0] = 0.f;
            world->ui->background_color[1] = 0.f;
            world->ui->background_color[2] = 0.f;
            world->ui->background_color[3] = 0.f;

            vec2 action_bar_base = { 0, 64+16.f };
            for(int i = 0; i < sizeof(yaal_state.player_actions)/sizeof(struct player_action); i++)
            {
                struct player_action* action = &yaal_state.player_actions[i];
                if(!action->visible || yaal_state.player_action_disable)
                {
                    ui_draw_image(world->ui, action_bar_base[0] + (64.f * i), action_bar_base[1], 64.f, 64.f, yaal_state.player_action_disabled_tex, 1.0f);
                    continue;
                }
                char act_name[64];
                snprintf(act_name, 64, "%i", i+1);
                ui_draw_text(world->ui, action_bar_base[0] + (64.f * i), action_bar_base[1] - 16.f, act_name, 0.9f);
                ui_draw_text(world->ui, action_bar_base[0] + (64.f * i), action_bar_base[1] - (4 * 16.f), action->action_name, 0.9f);
                bool press = ui_draw_button(world->ui, action_bar_base[0] + (64.f * i), action_bar_base[1], 64.f, 64.f, yaal_state.player_action_images[action->action_picture_id], 1.2f);

                if(action->combat_mode && !yaal_state.current_player->combat_mode)
                {
                    ui_draw_image(world->ui, action_bar_base[0] + (64.f * i), action_bar_base[1], 64.f, 64.f, yaal_state.player_action_disabled_tex, 0.9f);
                    ui_draw_image(world->ui, action_bar_base[0] + (64.f * i) + 32.f, action_bar_base[1] - 32.f, 32.f, 32.f, yaal_state.player_combat_mode_tex, 0.8f);
                }
                bool disable = false;
                if(action->action_count != -1)
                {
                    snprintf(act_name, 64, "%i", action->action_count);
                    ui_draw_text(world->ui, action_bar_base[0] + (64.f * i), action_bar_base[1], act_name, 0.9f);
                    if(action->action_count == 0)
                    {
                        ui_draw_image(world->ui, action_bar_base[0] + (64.f * i), action_bar_base[1], 64.f, 64.f, yaal_state.player_action_disabled_tex, 0.9f);
                        disable = true;
                    }
                }
                if(action->combat_mode && !yaal_state.current_player->combat_mode)
                    continue;
        #ifndef HEADLESS
                if((press || keys_down[GLFW_KEY_1 + i]) && !disable)
                    if(!yaal_state.player_action_debounce[i])
                    {
                        yaal_state.player_action_debounce[i] = true;
                        if(action->action_aim)
                        {
                            yaal_state.current_aiming_action = i;
                            yaal_state.player_action_disable = true;
                        }
                        else
                        {
                            struct network_packet upd_pak;
                            struct xtra_packet_data* x_data2 = (struct xtra_packet_data*)&upd_pak.packet.data;
                            upd_pak.meta.packet_type = YAAL_PLAYER_ACTION;
                            x_data2->packet.player_action.action_id = action->action_id;
                            network_transmit_packet(yaal_state.current_player->client->owner, &yaal_state.current_player->client->owner->client, upd_pak);
                        }
                    }
                if(yaal_state.player_action_debounce[i] && !(press || keys_down[GLFW_KEY_1 + i]))
                    yaal_state.player_action_debounce[i] = false; 
        #endif
            }    

            if(yaal_state.current_player)
            {
                if(glm_vec3_distance2(yaal_state.current_player->player_position,yaal_state.current_player->old_position)>1.0f)
                    ui_draw_image(world->ui, world->gfx.screen_width/2, world->gfx.screen_height, 32, 16, yaal_state.player_lag_tex, 1.f);
                float player_hearts = yaal_state.current_player->player_health/2.f;
                int player_full_hearts = floorf(player_hearts);
                vec2 offset = {0.f, 16.f};
                world->ui->foreground_color[0] = yaal_state.current_player->client->player_color_r;
                world->ui->foreground_color[1] = yaal_state.current_player->client->player_color_g;
                world->ui->foreground_color[2] = yaal_state.current_player->client->player_color_b;
                ui_draw_image(world->ui, offset[0], offset[1], world->gfx.screen_width, 16.f, yaal_state.player_hotbar_bar_tex, 1.1f);
                glm_vec4_copy(oldfg, world->ui->foreground_color);
                world->ui->shadow = false;
                for(int i = 0; i < player_full_hearts; i++)
                {
                    ui_draw_image(world->ui, offset[0], offset[1], 16.f, 16.f, yaal_state.player_heart_full_tex, 1.f);
                    offset[0] += 16.f;
                }
                int less_heart = 0;
                if(player_hearts - player_full_hearts > 0)
                {
                    ui_draw_image(world->ui, offset[0], offset[1], 16.f, 16.f, yaal_state.player_heart_half_tex, 1.f);
                    offset[0] += 16.f;
                    less_heart = 1;
                }
                int player_empty_hearts = yaal_state.current_player->player_max_health/2 - player_full_hearts - less_heart;
                for(int i = 0; i < player_empty_hearts; i++)
                {
                    ui_draw_image(world->ui, offset[0], offset[1], 16.f, 16.f, yaal_state.player_heart_none_tex, 1.f);
                    offset[0] += 16.f;
                }
                char player_stat_info[255];
                snprintf(player_stat_info, 255, "%02i/%02iHP", yaal_state.current_player->player_health, yaal_state.current_player->player_max_health);
                ui_draw_text(world->ui, offset[0], offset[1]-16.f, player_stat_info, 1.f);
                float player_heart_timer = yaal_state.player_heart_end - world->time;
                if(player_heart_timer > 0)
                {
                    float abs_timer = fabsf(player_heart_timer);
                    if(yaal_state.player_heart_amount > 0)
                    {
                        world->ui->foreground_color[0] = 0.016f;
                        world->ui->foreground_color[1] = 0.475f;
                        world->ui->foreground_color[2] = 0.039f;
                    }
                    else
                    {
                        world->ui->foreground_color[0] = 0.502f;
                        world->ui->foreground_color[1] = 0.0f;
                        world->ui->foreground_color[2] = 0.0f;
                    }
                    char chg[8];
                    snprintf(chg,8,"%i",yaal_state.player_heart_amount);
                    float yoff = sinf(abs_timer)*16.f;
                    float xoff = cosf(abs_timer*10.0)*2.f;        
                    world->ui->shadow = true;
                    ui_draw_text(world->ui, 4.f+xoff+(strlen(chg)*8.f), 125.f+yoff, chg, 0.7f);
                    world->ui->shadow = false;
                    glm_vec4_copy(oldfg, world->ui->foreground_color);
                }
                offset[0] += 16.f;
                offset[0] += strlen(player_stat_info) * 8.f;
                float player_manas = yaal_state.current_player->player_mana/2.f;
                int player_full_manas = floorf(player_manas);
                for(int i = 0; i < player_full_manas; i++)
                {
                    ui_draw_image(world->ui, offset[0], offset[1], 16.f, 16.f, yaal_state.player_mana_full_tex, 1.f);
                    offset[0] += 16.f;
                }
                if(player_manas - player_full_manas > 0)
                {
                    ui_draw_image(world->ui, offset[0], offset[1], 16.f, 16.f, yaal_state.player_mana_half_tex, 1.f);
                    offset[0] += 16.f;
                }
                int player_empty_manas = yaal_state.current_player->player_max_mana/2 - player_full_manas;
                for(int i = 0; i < player_empty_manas; i++)
                {
                    ui_draw_image(world->ui, offset[0], offset[1], 16.f, 16.f, yaal_state.player_mana_none_tex, 1.f);
                    offset[0] += 16.f;
                }
                snprintf(player_stat_info, 255, "%02i/%02iMP", yaal_state.current_player->player_mana, yaal_state.current_player->player_max_mana);
                ui_draw_text(world->ui, offset[0], offset[1]-16.f, player_stat_info, 1.f);
                offset[0] += strlen(player_stat_info) * 8.f;

                offset[0] = world->gfx.screen_width;
                int player_count = g_hash_table_size(world->client.players);
                snprintf(player_stat_info, 255, player_count == 1 ? "%i player online" : "%i players online", player_count);
                offset[0] -= strlen(player_stat_info) * 8.f;
                ui_draw_text(world->ui, offset[0], offset[1]-16.f, player_stat_info, 1.f);
                offset[0] -= 16.f;
                ui_draw_image(world->ui, offset[0], offset[1], 16.f, 16.f, yaal_state.friends_tex, 1.f);
                offset[0] -= 16.f;

                snprintf(player_stat_info, 255, "%i$", yaal_state.current_player->player_coins);
                offset[0] -= strlen(player_stat_info) * 8.f;
                ui_draw_text(world->ui, offset[0], offset[1]-16.f, player_stat_info, 1.f);
                offset[0] -= 16.f;
                ui_draw_image(world->ui, offset[0], offset[1], 16.f, 16.f, yaal_state.player_coins_tex, 1.f);
                world->ui->shadow = true;

                bool cmb_button = ui_draw_button(world->ui, (9*64.f), 16.f+64.f, 32.f, 64.f, yaal_state.current_player->combat_mode ? yaal_state.player_combat_on_tex : yaal_state.player_combat_off_tex, 1.f);
                bool chat_button = ui_draw_button(world->ui, (9*64.f)+32.f, 16.f+64.f, 32.f, 64.f, yaal_state.player_chat_mode ? yaal_state.player_chat_on_tex : yaal_state.player_chat_off_tex, 1.f);
                bool menu_button = ui_draw_button(world->ui, (9*64.f), 16.f+8.f+64.f, 64.f, 8.f, yaal_state.player_menu_button_tex, 1.f);
                if(cmb_button)
                {
                    struct network_packet upd_pak;
                    struct xtra_packet_data* x_data2 = (struct xtra_packet_data*)&upd_pak.packet.data;
                    upd_pak.meta.packet_type = YAAL_UPDATE_COMBAT_MODE;
                    x_data2->packet.update_combat_mode.player_id = -1;
                    x_data2->packet.update_combat_mode.combat_mode = !yaal_state.current_player->combat_mode;
                    network_transmit_packet(yaal_state.current_player->client->owner, &yaal_state.current_player->client->owner->client, upd_pak);
                }
                else if(chat_button)
                {
                    yaal_state.player_chat_mode = !yaal_state.player_chat_mode;
                    if(yaal_state.player_chat_mode)
                        start_text_input();
                    else
                        input_disable = false;
                }
                else if(menu_button)
                {
                    yaal_state.player_menu_open = true;
                }
                else
                {
                    
                }

                if(yaal_state.current_aiming_action != -1)
                {
                    world->ui->shadow = true;
                    struct player_action* action = &yaal_state.player_actions[yaal_state.current_aiming_action];
                    char action_txt[64];
                    snprintf(action_txt, 64, "Select where you want to use '%s'\n", action->action_name);
                    ui_draw_text(world->ui, 64.f, 16.f+64.f, action_txt, 1.f);
                    world->ui->shadow = false;
                    if(ui_draw_button(world->ui, 0.f, 16.f+32.f+64.f, 64.f, 32.f, yaal_state.player_cancel_button_tex, 1.f))
                    {
                        yaal_state.current_aiming_action = -1;
                        yaal_state.player_action_disable = false;
                        yaal_state.arrow_light.disable = true;
                    }
                    else
                    {
                        glm_vec3_copy(yaal_state.aiming_arrow_position,yaal_state.arrow_light.position);
                        //printf("%f,%f -> %f,%f,%f (%f,%f,%f)\n",mouse_position[0],mouse_position[1],yaal_state.aiming_arrow_position[0],yaal_state.aiming_arrow_position[1],yaal_state.aiming_arrow_position[2],dm[0],dm[1],dm[2]);
                        yaal_state.arrow_light.disable = false;
                        if(mouse_state.mouse_button_l && !yaal_state.player_action_debounce[yaal_state.current_aiming_action])
                        {
                            struct network_packet upd_pak;
                            struct xtra_packet_data* x_data2 = (struct xtra_packet_data*)&upd_pak.packet.data;
                            upd_pak.meta.packet_type = YAAL_PLAYER_ACTION;
                            x_data2->packet.player_action.action_id = action->action_id;
                            glm_vec3_copy(yaal_state.aiming_arrow_position, x_data2->packet.player_action.position);
                            network_transmit_packet(yaal_state.current_player->client->owner, &yaal_state.current_player->client->owner->client, upd_pak);
                            yaal_state.current_aiming_action = -1;
                            yaal_state.player_action_disable = false;
                            yaal_state.arrow_light.disable = true;
                        }
                        else
                        {
                            if(yaal_state.player_action_debounce[yaal_state.current_aiming_action] && !mouse_state.mouse_button_l)
                                yaal_state.player_action_debounce[yaal_state.current_aiming_action] = false;
                        }
                    }
                }            

                if(yaal_state.player_chat_mode)
                {
                    ui_draw_text(world->ui, 0.f, world->gfx.screen_height-32.f, "Chat message. Don't be an E-Tard.", 0.9f);
                    if(input_disable == false)
                    {
                        printf("yaal: %s\n", input_text);
                        yaal_state.player_chat_mode = false;

                        struct network_packet upd_pak;
                        upd_pak.meta.packet_type = PACKETTYPE_CHAT_MESSAGE;
                        upd_pak.meta.acknowledge = true;
                        strncpy(upd_pak.packet.chat_message.message,input_text,128);
                        network_transmit_packet(yaal_state.current_player->client->owner, &yaal_state.current_player->client->owner->client, upd_pak);
                    }
                }

        #ifndef HEADLESS
                if(keys_down[GLFW_KEY_C] && !yaal_state.player_chat_mode)
                {
                    keys_down[GLFW_KEY_C] = 0;
                    yaal_state.player_chat_mode = true;
                    start_text_input();
                }
        #endif
            }

            world->ui->persist = true;

            world->ui->foreground_color[0] = 1.f;
            world->ui->foreground_color[1] = 0.f;
            world->ui->foreground_color[2] = 0.f;

            ui_draw_text(ui, world->gfx.screen_width - (8.f * 8), world->gfx.screen_height - 64.f, "PREVIEW", 1.f);

            world->ui->foreground_color[0] = 1.f;
            world->ui->foreground_color[1] = 1.f;
            world->ui->foreground_color[2] = 1.f;

            ui_draw_image(ui, world->gfx.screen_width - 256.f, world->gfx.screen_height, 256.f, 64.f, world->gfx.sgl_background_image, 0.1f);


            world->ui->persist = false;

            glm_vec4_copy(oldfg, world->ui->foreground_color);
            glm_vec4_copy(oldbg, world->ui->background_color);

            yaal_state.last_player_list_id = 0;
            world->ui->background_color[0] = 0.f;
            world->ui->background_color[1] = 0.f;
            world->ui->background_color[2] = 0.f;
            
        #ifndef HEADLESS
            if(keys_down[GLFW_KEY_EQUAL])
                g_hash_table_foreach(world->client.players, __player_ui_list_render, world);
        #endif

            glm_vec4_copy(oldbg, world->ui->background_color);

            ui_draw_text(ui, 0.f, 16.f, txinfo, 1.f);

            if(yaal_state.map_downloading)
            {
                for(int i = 0; i < MAP_SIZE_MAX_X; i++)
                    ui_draw_text(ui, (fmodf(i,world->viewport[2] / 16.f)*8.f), 32.f, yaal_state.map_downloaded[i]?"D":"?", 1.f);
                snprintf(txinfo,256,"%f%% done (%i received, %i max)", ((float)yaal_state.map_downloaded_count/MAP_SIZE_MAX_X)*100.f, yaal_state.map_downloaded_count, MAP_SIZE_MAX_X);
                ui_draw_text(ui, 8.f+8.f*MAP_SIZE_MAX_X, 32.f, txinfo, 1.f);
            }

            break;
    }

    if(yaal_state.player_menu_open)
    {
        ui_draw_panel(world->ui, 8.f, world->gfx.screen_height-100.f, world->gfx.screen_width-16.f, world->gfx.screen_height/2.f, 0.5f);
        world->ui->current_panel->position_x += 1;
        ui_draw_text(world->ui, 0.f, 16.f, "YaalOnline Menu", 0.3f);
        if(ui_draw_button(world->ui, world->ui->current_panel->size_x-33.f, 0.f, 32.f, 16.f, yaal_state.player_exit_tex, 0.3f))
            yaal_state.player_menu_open = false;
        int options = 0;

#define PLAYER_MENU_OPTION(n,v) \
        ui_draw_text(world->ui, 0.f, 48.f+(options*16.f), n, 0.3f); \
        if(ui_draw_button(world->ui, strlen(n)*8.f+8.f, 48.f+(options*16.f)-16.f, 32.f, 16.f, v ? yaal_state.player_yes_tex : yaal_state.player_exit_tex, 0.3f)) \
        { \
        v = !v;\
        } \
        options++;

#define PLAYER_MENU_TXT(n) \
        ui_draw_text(world->ui, 0.f, 48.f+(options*16.f), n, 0.3f); \
        options++;

        PLAYER_MENU_TXT("YAAL OPTIONS");
        options++;

        if(world->server.status == NETWORKSTATUS_CONNECTED)
        {
            PLAYER_MENU_TXT("SERVER OPTIONS");
            options++;
            PLAYER_MENU_OPTION("Shutdown if empty", world->server.shutdown_empty);
            PLAYER_MENU_OPTION("Secure", world->server.security);
            PLAYER_MENU_OPTION("Open", world->server.server_open);
        }

        for(int i = 0; i < __SKILL_MAX; i++)
        {
            char skill_tx[64];
            snprintf(skill_tx,64,"%02i/99", MIN(99,MAX(yaal_state.current_player->skill_level[i],0)));
            ui_draw_image(world->ui, world->ui->current_panel->size_x/2.f + (64.f * (i % 2)), floorf(i/2)*16.f, 16.f, 16.f, yaal_state.skill_icons[i], 0.3f);
            ui_draw_text(world->ui, world->ui->current_panel->size_x/2.f + (64.f * (i % 2)) + 16.f, 16.f + floorf(i/2)*16.f, skill_tx, 0.3f);
        }

        ui_end_panel(world->ui);
    }

}

static void __load_map(const char* file_name)
{
    struct map_file_data* mfd = (struct map_file_data*)malloc(sizeof(struct map_file_data));
    FILE* mf = file_open(file_name, "r");
    if(mf)
    {
        int sz = fread(mfd, 1, sizeof(struct map_file_data), mf);
        // if(sz != sizeof(struct map_file_data))
        //     printf("yaal: warning: not enough bytes loaded to fill map_file_data (%i expected, %i got)\n", sizeof(struct map_file_data), sz);
        g_hash_table_insert(server_state.maps, &mfd->level_id, mfd);
        printf("yaal: map '%s', id %i loaded\n", mfd->level_name, mfd->level_id);
    }
    else
        printf("yaal: could not find map %s\n", file_name);
}

void sglthing_init_api(struct world* world)
{
    printf("yaal: you're running yaalonline\n");
#ifndef HEADLESS
    glfwSetWindowTitle(world->gfx.window, "YaalOnline");
#endif

    world->world_frame_user = __sglthing_frame;
    world->world_frame_render_user = __sglthing_frame_render;
    world->world_frame_ui_user = __sglthing_frame_ui;

    net_init(world);

    particles_init(&yaal_state.particles);

    world->cam.lsd = 0.0f;

    glm_vec4_copy(world->gfx.clear_color, world->gfx.fog_color);

    world->gfx.fog_maxdist = 40.f;
    world->gfx.fog_mindist = 32.f;

    yaal_state.area = light_create_area();
    yaal_state.current_player = 0;
    yaal_state.player_id = -1;
    yaal_state.current_aiming_action = -1;
    server_state.maps = g_hash_table_new(g_int_hash, g_int_equal);
    yaal_state.map_objects = g_hash_table_new(g_int_hash, g_int_equal);

    load_texture("pink_checkerboard.png");
    load_texture("yaal/ui/yaal_image.png");

    yaal_state.map_graphics_tiles[0] = get_texture("pink_checkerboard.png");
    for(int i = 1; i < MAP_TEXTURE_IDS; i++)
    {
        char texname[64];
        snprintf(texname, 64, "yaal/map/tex_%i.png", i);
        load_texture(texname);
        yaal_state.map_graphics_tiles[i] = get_texture(texname);
    }

    for(int i = 0; i < __SKILL_MAX; i++)
    {
        char texname[64];
        snprintf(texname, 64, "yaal/ui/skill/skill_%i.png", i);
        load_texture(texname);
        yaal_state.skill_icons[i] = get_texture(texname);
    }

    load_texture("yaal/ui/hotbar_d.png");
    yaal_state.player_action_disabled_tex = get_texture("yaal/ui/hotbar_d.png");
    for(int i = 0; i < ACTION_PICTURE_IDS; i++)
    {
        char texname[64];
        snprintf(texname, 64, "yaal/ui/hotbar_%i.png", i);
        load_texture(texname);
        yaal_state.player_action_images[i] = get_texture(texname);
    }

    yaal_state.map_tiles[0] = NULL;
    for(int i = 1; i < MAP_GRAPHICS_IDS; i++)
    {
        char mdlname[64];
        snprintf(mdlname, 64, "yaal/map/%i.obj", i);
        load_model(mdlname);
        yaal_state.map_tiles[i] = get_model(mdlname);
    }

    int maps_length;
    char** maps_to_load = g_key_file_get_string_list(world->config.key_file, "yaalonline", "maps", &maps_length, NULL);
    for(int i = 0; i < maps_length; i++)
        __load_map(maps_to_load[i]);

    yaal_state.chat = (struct chat_system*)malloc(sizeof(struct chat_system));
    chat_init(yaal_state.chat);

    world->gfx.sgl_background_image = get_texture("yaal/ui/yaal_image.png");

    yaal_state.tmp_range_calculation = NEW_RADIUS;

    load_texture("yaal/ui/heart_2.png");
    yaal_state.player_heart_full_tex = get_texture("yaal/ui/heart_2.png");
    load_texture("yaal/ui/heart_1.png");
    yaal_state.player_heart_half_tex = get_texture("yaal/ui/heart_1.png");
    load_texture("yaal/ui/heart_0.png");
    yaal_state.player_heart_none_tex = get_texture("yaal/ui/heart_0.png");
    load_texture("yaal/ui/mana_2.png");
    yaal_state.player_mana_full_tex = get_texture("yaal/ui/mana_2.png");
    load_texture("yaal/ui/mana_1.png");
    yaal_state.player_mana_half_tex = get_texture("yaal/ui/mana_1.png");
    load_texture("yaal/ui/mana_0.png");
    yaal_state.player_mana_none_tex = get_texture("yaal/ui/mana_0.png");
    load_texture("yaal/ui/cmbmode_off.png");
    yaal_state.player_combat_off_tex = get_texture("yaal/ui/cmbmode_off.png");
    load_texture("yaal/ui/cmbmode_on.png");
    yaal_state.player_combat_on_tex = get_texture("yaal/ui/cmbmode_on.png");
    load_texture("yaal/ui/cmbmode.png");
    yaal_state.player_combat_mode_tex = get_texture("yaal/ui/cmbmode.png");
    load_texture("yaal/ui/chat_off.png");
    yaal_state.player_chat_off_tex = get_texture("yaal/ui/chat_off.png");
    load_texture("yaal/ui/chat_on.png");
    yaal_state.player_chat_on_tex = get_texture("yaal/ui/chat_on.png");
    load_texture("yaal/ui/menu_button.png");
    yaal_state.player_menu_button_tex = get_texture("yaal/ui/menu_button.png");
    load_texture("yaal/ui/cancel.png");
    yaal_state.player_cancel_button_tex = get_texture("yaal/ui/cancel.png");
    load_texture("yaal/ui/hotbar_bar.png");
    yaal_state.player_hotbar_bar_tex = get_texture("yaal/ui/hotbar_bar.png");
    load_texture("yaal/ui/people.png");
    yaal_state.friends_tex = get_texture("yaal/ui/people.png");
    load_texture("yaal/select_particle.png");
    yaal_state.select_particle_tex = get_texture("yaal/select_particle.png");
    load_texture("yaal/ui/coins.png");
    yaal_state.player_coins_tex = get_texture("yaal/ui/coins.png");
    load_texture("yaal/ui/play_button.png");
    yaal_state.player_play_button_tex = get_texture("yaal/ui/play_button.png");
    load_texture("yaal/ui/ok_button.png");
    yaal_state.player_ok_button_tex = get_texture("yaal/ui/ok_button.png");
    load_texture("yaal/ui/lag.png");
    yaal_state.player_lag_tex = get_texture("yaal/ui/lag.png");
    load_texture("yaal/ui/exit.png");
    yaal_state.player_exit_tex = get_texture("yaal/ui/exit.png");
    load_texture("yaal/ui/yes.png");
    yaal_state.player_yes_tex = get_texture("yaal/ui/yes.png");

    load_model("test/box.obj");
    load_model("yaal/models/player.fbx");
    yaal_state.player_model = get_model("yaal/models/player.fbx");
    load_model("yaal/models/arrow.obj");
    yaal_state.arrow_model = get_model("yaal/models/arrow.obj");
    load_model("yaal/models/yaalonline.obj");
    yaal_state.menu_yaal_model = get_model("yaal/models/yaalonline.obj");
    load_model("yaal/models/city.obj");
    yaal_state.menu_city_model = get_model("yaal/models/city.obj");
    yaal_state.arrow_light.constant = 1.f;
    yaal_state.arrow_light.linear = 0.14f;
    yaal_state.arrow_light.quadratic = 0.07f;
    yaal_state.arrow_light.intensity = 1.f;
    yaal_state.arrow_light.flags = 0;

    yaal_state.arrow_light.ambient[0] = 0.0f;
    yaal_state.arrow_light.ambient[1] = 0.9f;
    yaal_state.arrow_light.ambient[2] = 0.0f;

    yaal_state.arrow_light.diffuse[0] = 0.1f;
    yaal_state.arrow_light.diffuse[1] = 0.9f;
    yaal_state.arrow_light.diffuse[2] = 0.1f;

    yaal_state.arrow_light.specular[0] = 0.4f;
    yaal_state.arrow_light.specular[1] = 0.9f;
    yaal_state.arrow_light.specular[2] = 0.4f;                    

    light_add(yaal_state.area, &yaal_state.arrow_light);

    for(int i = 0; i < ANIMATIONS_TO_LOAD; i++)
        if(animation_create(NULL, &yaal_state.player_model->meshes[0], i, &yaal_state.player_animations[i]) == -1)
            break;
        
    int f = compile_shader("shaders/fragment.fs", GL_FRAGMENT_SHADER);
    yaal_state.object_shader = link_program(compile_shader("shaders/normal.vs", GL_VERTEX_SHADER), f);
    yaal_state.player_shader = link_program(compile_shader("shaders/rigged.vs", GL_VERTEX_SHADER), f);

    init_fx(&yaal_state.fx_manager, yaal_state.object_shader, &yaal_state.particles);
    init_fx(&server_state.fx_manager, 0, NULL);
    server_state.fx_manager.server_fx = true;
    yaal_state.show_rpg_message = false;

    yaal_state.player_heart_end = 0.f;
    yaal_state.player_heart_amount = 0;

    yaal_state.damage_font = ui_load_font("uiassets/font3.png",32,32,16,8);

    yaal_state.mode = YAAL_STATE_MENU;

    load_snd("yaal/snd/hello.wav");
    play_snd("yaal/snd/hello.wav");
}