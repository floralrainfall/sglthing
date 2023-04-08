#include <sglthing/sglthing.h>
#include <sglthing/world.h>
#include <sglthing/net.h>
#include <sglthing/model.h>
#include <sglthing/texture.h>
#include <sglthing/shader.h>
#include <sglthing/keyboard.h>
#include <sglthing/light.h>
#include <sglthing/io.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include "yaal.h"

struct player {
    struct network_client* client;
    enum direction dir;

    vec3 player_position;
    vec3 old_position;

    int player_id;

    mat4 model_matrix;
    mat4 ghost_matrix;

    struct light player_light;
};


struct {
    int current_level_id;
    struct map_tile_data map_data[MAP_SIZE_MAX_X][MAP_SIZE_MAX_Y];
    int player_id;
    struct player* current_player;

    struct model* player_model;
    struct model* map_tiles[MAP_GRAPHICS_IDS];
    int map_graphics_tiles[MAP_TEXTURE_IDS];
    int player_shader;

    struct light_area* area;
} yaal_state;

struct model* map_model;

static void __update_player_transform(struct player* player)
{
    glm_mat4_identity(player->model_matrix);
    glm_translate(player->model_matrix, player->old_position);
    glm_translate_y(player->model_matrix, 1.0f);

    glm_mat4_identity(player->ghost_matrix);
    glm_translate(player->ghost_matrix, player->player_position);
    glm_translate_y(player->ghost_matrix, 1.0f);

    glm_vec3_copy(player->player_position, player->player_light.position);
}

static void __sglthing_new_player(struct network* network, struct network_client* client)
{
    printf("yaal: [%s] new player %i %p '%s'\n", (client->owner->mode == NETWORKMODE_SERVER)?"server":"client", client->player_id, client, client->client_name);
    struct player* new_player = (struct player*)malloc(sizeof(struct player));
    new_player->client = client;
    new_player->player_id = client->player_id;

    if(network->mode == NETWORKMODE_SERVER)
    {

    }
    else
    {
        new_player->player_light.constant = 1.f;
        new_player->player_light.linear = 0.14f;
        new_player->player_light.quadratic = 0.07f;
        new_player->player_light.intensity = 1.f;
        new_player->player_light.flags = 0;

        new_player->player_light.ambient[0] = 0.4f;
        new_player->player_light.ambient[1] = 0.7f;
        new_player->player_light.ambient[2] = 0.6f;

        new_player->player_light.diffuse[0] = 0.7f;
        new_player->player_light.diffuse[1] = 0.8f;
        new_player->player_light.diffuse[2] = 0.9f;

        new_player->player_light.specular[0] = 0.8f;
        new_player->player_light.specular[1] = 0.9f;
        new_player->player_light.specular[2] = 1.0f;
        
        light_add(&new_player->player_light);
    }

    glm_vec3_zero(new_player->old_position);
    glm_vec3_zero(new_player->player_position);

    __update_player_transform(new_player);

    client->user_data = (void*)new_player;
}

static void __sglthing_del_player(struct network* network, struct network_client* client)
{
    printf("yaal: [%s] del player %p '%s'\n", (network->mode == NETWORKMODE_SERVER)?"server":"client", client, client->client_name);
    struct player* old_player = (struct player*)client->user_data;

    free(old_player);
}

static bool __sglthing_packet(struct network* network, struct network_client* client, struct network_packet* packet)
{
    struct xtra_packet_data* x_data = (struct xtra_packet_data*)&packet->packet.data;
    struct player* client_player = (struct player*)client->user_data;
    switch((enum yaal_packet_type)packet->meta.packet_type)
    {
        case PACKETTYPE_PING:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                if(yaal_state.current_player)
                {
                    float dist = glm_vec3_distance(yaal_state.current_player->player_position, yaal_state.current_player->old_position);
                    if(dist > 0.01f)
                    {
                        printf("yaal: [%s] tx position %f\n", (network->mode == NETWORKMODE_SERVER)?"server":"client", dist);
                        struct network_packet upd_pak;
                        struct xtra_packet_data* x_data2 = (struct xtra_packet_data*)&upd_pak.packet.data;
                        x_data2->packet.update_position.player_id = -1;
                        upd_pak.meta.packet_type = YAAL_UPDATE_POSITION;
                        vec3 delta;
                        glm_vec3_sub(yaal_state.current_player->old_position, yaal_state.current_player->player_position, delta);
                        glm_vec3_copy(delta, x_data2->packet.update_position.delta_pos);
                        glm_vec3_copy(yaal_state.current_player->player_position, yaal_state.current_player->old_position);
                        network_transmit_packet(network, client, upd_pak);
                    }
                }
            }
            return true;
        case PACKETTYPE_SERVERINFO:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                yaal_state.player_id = packet->packet.serverinfo.player_id;
                printf("yaal: our player id: %i\n", yaal_state.player_id);
            }
            return true;
        case YAAL_ENTER_LEVEL:
            if(network->mode == NETWORKMODE_SERVER)
            {

            }
            else
            {
                if(x_data->packet.yaal_level.level_id != yaal_state.current_level_id)
                {
                    char map_filename[256];
                    snprintf(map_filename, 256, "maps/lvl-%i.obj", x_data->packet.yaal_level);
                }
            }
            return false;
        case YAAL_UPDATE_POSITION:
            if(network->mode == NETWORKMODE_SERVER)
            {
                if(!client_player)
                    printf("yaal: client without client_player is attempting to update position\n");

                glm_vec3_add(client_player->player_position, x_data->packet.update_position.delta_pos, client_player->player_position);
                glm_vec3_copy(client_player->player_position, client_player->old_position);

                struct network_packet upd_pak;
                struct xtra_packet_data* x_data2 = (struct xtra_packet_data*)&upd_pak.packet.data;
                upd_pak.meta.packet_type = YAAL_UPDATE_POSITION;
                x_data2->packet.update_position.player_id = client->player_id;
                glm_vec3_copy(client_player->player_position, x_data2->packet.update_position.delta_pos);
                network_transmit_packet_all(network, upd_pak);
            }
            else
            {
                struct network_client* upd_client = g_hash_table_lookup(network->players, &x_data->packet.update_position.player_id);
                if(upd_client)
                {
                    if(upd_client != yaal_state.current_player->client)
                        printf("yaal: updating client that is not user\n");
                    struct player* upd_player = (struct player*)upd_client->user_data;
                    glm_vec3_copy(x_data->packet.update_position.delta_pos, upd_player->old_position);
                    glm_vec3_copy(x_data->packet.update_position.delta_pos, upd_player->player_position);
                    printf("yaal: updating player %i position (%f,%f,%f)\n", upd_client->player_id, upd_player->old_position[0], upd_player->old_position[1], upd_player->old_position[2]);
                    __update_player_transform(upd_player);
                }
            }
            return false;
        default: // unknown packet, for system
            printf("yaal: [%s] unknown packet %i\n", (network->mode == NETWORKMODE_SERVER)?"server":"client", packet->meta.packet_type);
            return true;
    }
}

static void __player_iter(gpointer key, gpointer value, gpointer user)
{
    struct world* world = (struct world*)user;
    struct network_client* client = (struct network_client*)value;
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
        g_hash_table_foreach(world->client.players, __player_iter, (gpointer)world);

        if(yaal_state.current_player)
        {
            light_update(world->render_area, yaal_state.current_player->player_position);

            world->cam.position[0] = -yaal_state.current_player->old_position[0] - 15.f;
            world->cam.position[1] = yaal_state.current_player->old_position[1] + 20.f;
            world->cam.position[2] = -yaal_state.current_player->old_position[2] - 15.f;
            world->cam.yaw = 45.f;
            world->cam.pitch = -45.f;

            vec3 pot_move;
            pot_move[0] = get_input("x_axis") * world->delta_time * 3.f;
            pot_move[2] = -get_input("z_axis") * world->delta_time * 3.f;
            glm_vec3_add(pot_move, yaal_state.current_player->player_position, yaal_state.current_player->player_position);
        }
    }
    if(world->server.status == NETWORKSTATUS_CONNECTED)
    {
        g_hash_table_foreach(world->server.players, __player_iter, (gpointer)world);        
    }
}

static void __player_render(gpointer key, gpointer value, gpointer user)
{
    struct world* world = (struct world*)user;
    struct network_client* client = (struct network_client*)value;
    struct player* player = (struct player*)client->user_data;

    if(player)
    {
        world_draw_model(world, yaal_state.player_model, yaal_state.player_shader, player->model_matrix, true);
        world_draw_model(world, yaal_state.player_model, yaal_state.player_shader, player->ghost_matrix, true);
        char nameplate_txt[256];
        snprintf(nameplate_txt, 256, "%s", client->client_name);

        if(!world->gfx.shadow_pass)
            ui_draw_text_3d(world->ui, world->viewport, world->cam.position, world->cam.front, (vec3){0.f,0.f,-2.f}, world->cam.fov, player->model_matrix, world->vp, nameplate_txt);
    }
}

#define MAP_TILE_SIZE 2

static void __sglthing_frame_render(struct world* world)
{
    world->render_area = yaal_state.area;
    for(int map_x = 0; map_x < MAP_SIZE_MAX_X; map_x++)
    {
        for(int map_y = 0; map_y < MAP_SIZE_MAX_Y; map_y++)
        {
            struct map_tile_data* map_tile = &yaal_state.map_data[map_x][map_y];
            if(map_tile->map_tile_type == TILE_AIR || !yaal_state.current_player)
            {

            }
            else
            {
                vec3 map_pos = { map_x * MAP_TILE_SIZE, 0.f, map_y * MAP_TILE_SIZE };
                if(glm_vec3_distance(map_pos, world->cam.position) > 64.f)
                    continue;
                vec3 direction;
                glm_vec3_sub(world->cam.position,map_pos,direction);
                float angle = glm_vec3_dot(world->cam.front, direction) / M_PI_180f;
                if(angle > world->cam.fov)
                    continue;
                mat4 model_matrix;
                glm_mat4_identity(model_matrix);
                glm_translate(model_matrix, map_pos);
                if(map_tile->map_tile_type == TILE_WALL_CHEAP)
                {
                    glm_translate_y(model_matrix, 4.f);
                    glm_scale(model_matrix, (vec3){1.f, 4.f, 1.f});
                }
                if(map_tile->map_tile_type == TILE_WALL_CHEAP || map_tile->map_tile_type == TILE_FLOOR)
                {

                }
                struct model* mdl = yaal_state.map_tiles[map_tile->tile_graphics_id];
                if(mdl)
                    if(map_tile->tile_graphics_tex)
                    {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, yaal_state.map_graphics_tiles[map_tile->tile_graphics_tex]);                
                        glUniform1i(glGetUniformLocation(yaal_state.player_shader,"diffuse0"), 0);
                        world_draw_model(world, mdl, yaal_state.player_shader, model_matrix, false);
                    }
                    else
                        world_draw_model(world, mdl, yaal_state.player_shader, model_matrix, true);
                struct model* mdl2 = yaal_state.map_tiles[map_tile->tile_graphics_ext_id];
                if(mdl2)
                {
                    glm_translate_y(model_matrix, 2.0f);
                    world_draw_model(world, mdl2, yaal_state.player_shader, model_matrix, true);
                }
            }
        }
    }    
    g_hash_table_foreach(world->client.players, __player_render, (gpointer)world);
}

static void __sglthing_frame_ui(struct world* world)
{
    struct ui_data* ui = world->ui;

    char txinfo[256];
    if(yaal_state.current_player)
    {
        snprintf(txinfo,256,"P=(%f,%f,%f), O=(%f,%f,%f), player id = %i\n",
            yaal_state.current_player->player_position[0],
            yaal_state.current_player->player_position[1],
            yaal_state.current_player->player_position[2],
            yaal_state.current_player->old_position[0],
            yaal_state.current_player->old_position[1],
            yaal_state.current_player->old_position[2],
            yaal_state.current_player->player_id);
    }
    else
    {
        snprintf(txinfo,256,"Waiting for player (%i)", yaal_state.player_id);
    }

    ui_draw_text(ui, 0.f, 16.f, txinfo, 1.f);
}

void sglthing_init_api(struct world* world)
{
    printf("yaal: you're running yaalonline\n");

    world->world_frame_user = __sglthing_frame;
    world->world_frame_render_user = __sglthing_frame_render;
    world->world_frame_ui_user = __sglthing_frame_ui;

    world->client.receive_packet_callback = __sglthing_packet;
    world->client.new_player_callback = __sglthing_new_player;
    world->client.del_player_callback = __sglthing_del_player;

    world->server.receive_packet_callback = __sglthing_packet;
    world->server.new_player_callback = __sglthing_new_player;
    world->server.del_player_callback = __sglthing_del_player;

    world->gfx.clear_color[0] = 0.f;
    world->gfx.clear_color[1] = 0.f;
    world->gfx.clear_color[2] = 0.f;

    yaal_state.area = light_create_area();
    yaal_state.current_player = 0;
    yaal_state.player_id = -1;

    yaal_state.map_tiles[0] = NULL;
    for(int i = 1; i < MAP_GRAPHICS_IDS; i++)
    {
        char mdlname[64];
        snprintf(mdlname, 64, "yaal/map/%i.obj", i);
        load_model(mdlname);
        yaal_state.map_tiles[i] = get_model(mdlname);
    }

    yaal_state.map_graphics_tiles[0] = 0;
    for(int i = 1; i < MAP_TEXTURE_IDS; i++)
    {
        char texname[64];
        snprintf(texname, 64, "yaal/map/tex_%i.png", i);
        load_texture(texname);
        yaal_state.map_graphics_tiles[i] = get_texture(texname);
    }

    for(int map_x = 0; map_x < MAP_SIZE_MAX_X; map_x++)
    {
        for(int map_y = 0; map_y < MAP_SIZE_MAX_Y; map_y++)
        {
            struct map_tile_data* map_tile = &yaal_state.map_data[map_x][map_y];
            map_tile->map_tile_type = ((map_x%5==0)||(map_y%7==0))?TILE_WALL_CHEAP:TILE_FLOOR;
            map_tile->tile_graphics_id = 1;
            if(map_tile->map_tile_type == TILE_FLOOR)
                map_tile->tile_graphics_ext_id = 0;
            map_tile->tile_graphics_tex = 0;
            map_tile->direction = rand() * DIR_MAX;
        }
    }

    load_model("test/box.obj");
    yaal_state.player_model = get_model("test/box.obj");
    yaal_state.player_shader = link_program(compile_shader("shaders/normal.vs", GL_VERTEX_SHADER), compile_shader("shaders/fragment.fs", GL_FRAGMENT_SHADER));
}