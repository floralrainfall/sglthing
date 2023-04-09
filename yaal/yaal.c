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

#define MAP_TILE_SIZE 2

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

struct yaal_state {
    int current_level_id;
    char level_name[64];

    int player_id;
    struct player* current_player;

    struct model* player_model;
    struct model* map_tiles[MAP_GRAPHICS_IDS];
    int map_graphics_tiles[MAP_TEXTURE_IDS];
    int player_shader;

    struct light_area* area;

    GHashTable* map_objects;
    GHashTable* maps;
    
    bool cols_received[MAP_SIZE_MAX_X];
    struct map_tile_data map_data[MAP_SIZE_MAX_X][MAP_SIZE_MAX_Y];
};

struct yaal_state yaal_state;
struct yaal_state server_state;

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
        int intro_id = 0;
        struct map_file_data* map = g_hash_table_lookup(server_state.maps, &intro_id);
        if(map)
        {
            struct network_packet upd_pak;
            struct xtra_packet_data* x_data2 = (struct xtra_packet_data*)&upd_pak.packet.data;
            upd_pak.meta.packet_type = YAAL_ENTER_LEVEL;
            x_data2->packet.yaal_level.level_id = map->level_id;
            strncpy(x_data2->packet.yaal_level.level_name,map->level_name,64);
            network_transmit_packet(network, client, upd_pak);
        }
        else
            printf("yaal: no map id 0\n");
    }
    else
    {
        new_player->player_light.constant = 1.f;
        new_player->player_light.linear = 0.07f;
        new_player->player_light.quadratic = 0.017f;
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
        
        light_add(yaal_state.area, &new_player->player_light);
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
        case YAAL_LEVEL_DATA:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                printf("yaal: receiving %i bytes in level data\n", sizeof(x_data->packet.yaal_level_data));
                memcpy(&yaal_state.map_data[x_data->packet.yaal_level_data.yaal_x], &x_data->packet.yaal_level_data.data[0], sizeof(x_data->packet.yaal_level_data));

                for(int i = 0; i < MAP_SIZE_MAX_Y; i++)
                {
                    vec3 map_pos = { x_data->packet.yaal_level_data.yaal_x * MAP_TILE_SIZE, 0.f, i * MAP_TILE_SIZE };
                    struct map_tile_data* d = &yaal_state.map_data[x_data->packet.yaal_level_data.yaal_x][i];
                    switch(d->tile_light_type)
                    {
                        case LIGHT_OBJ4_LAMP:
                        {
                            struct light* tile_light = (struct light*)malloc(sizeof(struct light));
                            tile_light->constant = 1.f;
                            tile_light->linear = 0.07f;
                            tile_light->quadratic = 0.017f;
                            tile_light->intensity = 1.f;
                            tile_light->flags = 0;

                            tile_light->ambient[0] = 0.6f;
                            tile_light->ambient[1] = 0.7f;
                            tile_light->ambient[2] = 0.4f;

                            tile_light->diffuse[0] = 0.9f;
                            tile_light->diffuse[1] = 0.8f;
                            tile_light->diffuse[2] = 0.4f;

                            tile_light->specular[0] = 0.9f;
                            tile_light->specular[1] = 0.9f;
                            tile_light->specular[2] = 0.7f;                    

                            
                            glm_vec3_copy(map_pos, tile_light->position);

                            light_add(yaal_state.area, tile_light);
                        }
                            break;
                        default:
                        case LIGHT_NONE:
                            break;
                    }

                }
            }
            else
            {
                struct map_file_data* map = g_hash_table_lookup(server_state.maps, &x_data->packet.yaal_level_data.level_id);
                if(map)
                {
                    printf("yaal: sending %i bytes in level data\n", sizeof(x_data->packet.yaal_level_data));
                    memcpy(&x_data->packet.yaal_level_data.data[0], &map->map_row[x_data->packet.yaal_level_data.yaal_x].data[0], sizeof(x_data->packet.yaal_level_data));
                    network_transmit_packet(network, client, *packet);
                }
                else
                {
                    printf("yaal: unknown map %i\n", x_data->packet.yaal_level_data.level_id);
                }
            }
            break;
        case YAAL_ENTER_LEVEL:
            if(network->mode == NETWORKMODE_SERVER)
            {

            }
            else
            {
                printf("yaal: entering level %s\n", x_data->packet.yaal_level.level_name);

                yaal_state.current_level_id = x_data->packet.yaal_level.level_id;
                strncpy(yaal_state.level_name, x_data->packet.yaal_level.level_name, 64);

#define LEVEL_TRANSMISSION
#ifdef LEVEL_TRANSMISSION
                for(int i = 0; i < MAP_SIZE_MAX_X; i++)
                {
                    yaal_state.cols_received[i] = 0;

                    struct network_packet upd_pak;
                    struct xtra_packet_data* x_data2 = (struct xtra_packet_data*)&upd_pak.packet.data;
                    upd_pak.meta.packet_type = YAAL_LEVEL_DATA;
                    x_data2->packet.yaal_level_data.level_id = 0;
                    x_data2->packet.yaal_level_data.yaal_x = i;
                    network_transmit_packet(network, client, upd_pak);
                }
#endif
            }
            return false;
        case YAAL_UPDATE_POSITION:
            if(network->mode == NETWORKMODE_SERVER)
            {
                if(!client_player)
                    printf("yaal: client without client_player is attempting to update position\n");
                printf("yaal: [server] updating player position %i\n", client_player->player_id);

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
                else
                    printf("yaal: server tried to update a client that doesnt exist (%i)\n", x_data->packet.update_position.player_id);
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
            vec3 map_plr_pos = { -yaal_state.current_player->old_position[0], 0.f, -yaal_state.current_player->old_position[2] };
            light_update(world->render_area, map_plr_pos);

            glm_vec3_copy(map_plr_pos, world->gfx.sun_position);
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

        world->ui->foreground_color[0] = player->client->player_color_r;
        world->ui->foreground_color[1] = player->client->player_color_g;
        world->ui->foreground_color[2] = player->client->player_color_b;
        world->ui->background_color[3] = 0.f;
        world->ui->persist = true;
        if(!world->gfx.shadow_pass)
            ui_draw_text_3d(world->ui, world->viewport, world->cam.position, world->cam.front, (vec3){0.f,2.f,0.f}, world->cam.fov, player->model_matrix, world->vp, nameplate_txt);
        world->ui->persist = false;
    }
}

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
                vec3 map_plr_pos = { -yaal_state.current_player->player_position[0], 0.f, -yaal_state.current_player->player_position[2] };
                float dist = glm_vec3_distance(map_pos, map_plr_pos);
                if(dist > 32.f)
                    continue;
                vec3 direction;
                glm_vec3_sub(world->cam.position,map_pos,direction);
                float angle = glm_vec3_dot(world->cam.front, direction);
                if(angle > (world->cam.fov * M_PI_180f))
                    continue;
                mat4 model_matrix;
                glm_mat4_identity(model_matrix);
                glm_translate(model_matrix, map_pos);
                if(map_tile->map_tile_type == TILE_WALL_CHEAP)
                {
                    glm_translate_y(model_matrix, 4.f);
                    glm_scale(model_matrix, (vec3){1.f, 4.f, 1.f});
                }
                // glm_rotate(model_matrix, (map_tile->direction * 90.f) * (180 / M_PIf), (vec3){0.f,1.f,0.f});
                struct model* mdl = yaal_state.map_tiles[map_tile->tile_graphics_id];
                if(mdl)
                    if(map_tile->tile_graphics_tex)
                    {
                        int tex_id = yaal_state.map_graphics_tiles[map_tile->tile_graphics_tex];
                        if(tex_id == 0)
                            tex_id = get_texture("pink_checkerboard.png");
                        sglc(glUseProgram(yaal_state.player_shader));
                        sglc(glActiveTexture(GL_TEXTURE0));
                        sglc(glBindTexture(GL_TEXTURE_2D, tex_id));                
                        sglc(glUniform1i(glGetUniformLocation(yaal_state.player_shader,"diffuse0"), 0));
                        world_draw_model(world, mdl, yaal_state.player_shader, model_matrix, false);
                    }
                    else
                        world_draw_model(world, mdl, yaal_state.player_shader, model_matrix, true);
                struct model* mdl2 = yaal_state.map_tiles[map_tile->tile_graphics_ext_id];
                if(mdl2 && dist < 32.f)
                    world_draw_model(world, mdl2, yaal_state.player_shader, model_matrix, true);
            }
        }
    }    

    vec4 oldfg, oldbg;
    glm_vec4_copy(world->ui->foreground_color, oldfg);
    glm_vec4_copy(world->ui->background_color, oldbg);
    g_hash_table_foreach(world->client.players, __player_render, (gpointer)world);        
    glm_vec4_copy(oldfg, world->ui->foreground_color);
    glm_vec4_copy(oldbg, world->ui->background_color);
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

static void __load_map(const char* file_name)
{
    struct map_file_data* mfd = (struct map_file_data*)malloc(sizeof(struct map_file_data));
    FILE* mf = file_open(file_name, "r");
    if(mf)
    {
        int sz = fread(mfd, 1, sizeof(struct map_file_data), mf);
        if(sz != sizeof(struct map_file_data))
            printf("yaal: warning: not enough bytes loaded to fill map_file_data (%i expected, %i got)\n", sizeof(struct map_file_data), sz);
        g_hash_table_insert(server_state.maps, &mfd->level_id, mfd);
        printf("yaal: map '%s', id %i loaded\n", mfd->level_name, mfd->level_id);
    }
    else
        printf("yaal: could not find map %s\n", file_name);
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

    world->gfx.fog_color[0] = 0.f;
    world->gfx.fog_color[1] = 0.f;
    world->gfx.fog_color[2] = 0.f;
    world->gfx.fog_maxdist = 64.f;
    world->gfx.fog_mindist = 32.f;

    yaal_state.area = light_create_area();
    yaal_state.current_player = 0;
    yaal_state.player_id = -1;
    server_state.maps = g_hash_table_new(g_int_hash, g_int_equal);

    yaal_state.map_tiles[0] = NULL;
    for(int i = 1; i < MAP_GRAPHICS_IDS; i++)
    {
        char mdlname[64];
        snprintf(mdlname, 64, "yaal/map/%i.obj", i);
        load_model(mdlname);
        yaal_state.map_tiles[i] = get_model(mdlname);
    }

    load_texture("pink_checkerboard.png");

    yaal_state.map_graphics_tiles[0] = 0;
    for(int i = 1; i < MAP_TEXTURE_IDS; i++)
    {
        char texname[64];
        snprintf(texname, 64, "yaal/map/tex_%i.png", i);
        load_texture(texname);
        yaal_state.map_graphics_tiles[i] = get_texture(texname);
    }

    /*for(int map_x = 0; map_x < MAP_SIZE_MAX_X; map_x++)
    {
        for(int map_y = 0; map_y < MAP_SIZE_MAX_Y; map_y++)
        {
            struct map_tile_data* map_tile = &server_state.map_data[map_x][map_y];
            map_tile->map_tile_type = ((map_x%5==0)||(map_y%7==0))?TILE_WALL:TILE_FLOOR;
            if(map_tile->map_tile_type == TILE_WALL)
                map_tile->tile_graphics_id = 2;
            else
                map_tile->tile_graphics_id = 1;
            int rng = rand();
            if(map_tile->map_tile_type == TILE_FLOOR && (rng % 5) == 0)
                map_tile->tile_graphics_ext_id = 3;
            else if(map_tile->map_tile_type == TILE_FLOOR && (rng % 5) == 1)
            {
                map_tile->tile_graphics_ext_id = 4;
                map_tile->tile_light_type = LIGHT_OBJ4_LAMP;    
            }
            map_tile->tile_graphics_tex = 0;
            map_tile->direction = rand() * DIR_MAX;
        }
    }*/

    __load_map("yaal/maps/introduction.map");

    load_model("test/box.obj");
    yaal_state.player_model = get_model("test/box.obj");
    yaal_state.player_shader = link_program(compile_shader("shaders/normal.vs", GL_VERTEX_SHADER), compile_shader("shaders/fragment.fs", GL_FRAGMENT_SHADER));
}