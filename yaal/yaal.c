#include <sglthing/sglthing.h>
#include <sglthing/world.h>
#include <sglthing/net.h>
#include <sglthing/model.h>
#include <sglthing/shader.h>
#include <sglthing/keyboard.h>
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
};

struct {
    int current_level_id;
    int player_id;
    struct player* current_player;

    struct model* player_model;
    int player_shader;
} yaal_state;

struct model* map_model;

static void __update_player_transform(struct player* player)
{
    glm_mat4_identity(player->model_matrix);
    glm_translate(player->model_matrix, player->player_position);

    glm_mat4_identity(player->ghost_matrix);
    glm_translate(player->ghost_matrix, player->old_position);
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
                    if(dist > 1.f)
                    {
                        printf("yaal: [%s] tx position %f\n", (network->mode == NETWORKMODE_SERVER)?"server":"client", dist);
                        struct network_packet upd_pak;
                        struct xtra_packet_data* x_data2 = (struct xtra_packet_data*)&upd_pak.packet.data;
                        x_data2->packet.update_position.player_id = -1;
                        upd_pak.meta.packet_type = YAAL_UPDATE_POSITION;
                        vec3 delta;
                        glm_vec3_sub(yaal_state.current_player->old_position, yaal_state.current_player->player_position, delta);
                        glm_vec3_copy(delta, x_data2->packet.update_position.delta_pos);
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
                glm_vec3_divs(x_data->packet.update_position.delta_pos, 1.1f, client_player->player_position);
                glm_vec3_add(client_player->player_position, x_data->packet.update_position.delta_pos, client_player->player_position);

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
                    struct player* upd_player = (struct player*)upd_client->user_data;
                    glm_vec3_copy(x_data->packet.update_position.delta_pos, upd_player->old_position);
                    glm_vec3_copy(x_data->packet.update_position.delta_pos, upd_player->player_position);
                    printf("updating player %i position (%f,%f,%f)\n", upd_client->player_id, upd_player->player_position[0], upd_player->player_position[1], upd_player->player_position[2]);
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
            world->cam.position[0] = 0.f;
            world->cam.position[1] = 32.f;
            world->cam.position[2] = 0.f;
            world->cam.yaw = 90.f;
            world->cam.pitch = -90.f;

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
        ui_draw_text_3d(world->ui, world->viewport, world->cam.position, world->cam.front, player->player_position, world->cam.fov, player->model_matrix, world->vp, nameplate_txt);
    }
}

static void __sglthing_frame_render(struct world* world)
{
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

    yaal_state.current_player = 0;
    yaal_state.player_id = -1;

    load_model("test/box.obj");
    yaal_state.player_model = get_model("test/box.obj");
    yaal_state.player_shader = link_program(compile_shader("shaders/normal.vs", GL_VERTEX_SHADER), compile_shader("shaders/fragment.fs", GL_FRAGMENT_SHADER));
}