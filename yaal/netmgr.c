#include "netmgr.h"

static bool __work_action(struct network_packet* packet, struct network* network, struct network_client* client)
{
    struct xtra_packet_data* x_data = (struct xtra_packet_data*)&packet->packet.data;
    struct player* player;
    player = (struct player*)client->user_data;
    ASSERT(player);
    switch(x_data->packet.player_action.action_id)
    {
        case ACTION_EXIT:
            if(network->mode == NETWORKMODE_SERVER)
                network_disconnect_player(network, true, "Disconnect by menu", client);
            break;
        case ACTION_SERVER_MENU:
            break;
        case ACTION_BOMB_DROP:
        case ACTION_BOMB_THROW:
            if(network->mode == NETWORKMODE_SERVER)
            {
                if(player->player_bombs == 0)
                    return false;
                player->player_bombs--;
                
                BOMB_THROW_ACTION_UPDATE(player, client, network);            
            }
            else
            {
                if(x_data->packet.player_action.player_id != -1)
                    add_fx(&yaal_state.fx_manager, (struct fx){
                        .start[0] = player->player_position[0],
                        .start[1] = player->player_position[1],
                        .start[2] = player->player_position[2],
                        .target[0] = x_data->packet.player_action.position[0],
                        .target[1] = x_data->packet.player_action.position[1],
                        .target[2] = x_data->packet.player_action.position[2],
                        .type = FX_BOMB_THROW,
                        .max_time = 3.f,
                        .speed = 1.f,
                    });
            }
            return true;
        default:
            printf("yaal: unknown action id %i\n", x_data->packet.player_action.action_id);
            break;
    }
    return false;
}

static void __sglthing_new_player(struct network* network, struct network_client* client)
{
    printf("yaal: [%s] new player %i %p '%s'\n", (client->owner->mode == NETWORKMODE_SERVER)?"server":"client", client->player_id, client, client->client_name);
    struct player* new_player = (struct player*)malloc(sizeof(struct player));
    new_player->client = client;
    new_player->player_id = client->player_id;
    new_player->level_id = -1;

    if(network->mode == NETWORKMODE_SERVER)
    {
        int intro_id = 0;
        new_player->level_id = intro_id;
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
        yaal_update_player_action(8, (struct player_action){.action_name = "Exit", .action_desc = "LMAO", .action_id = ACTION_EXIT, .action_picture_id = 1, .visible = true, .action_count = -1, .combat_mode = false}, client, network);
        yaal_update_player_action(7, (struct player_action){.action_name = "Menu", .action_desc = "LMAO", .action_id = ACTION_SERVER_MENU, .action_picture_id = 4, .visible = true, .action_count = -1, .combat_mode = false}, client, network);
    }
    else
    {
        new_player->player_light.constant = 1.f;
        new_player->player_light.linear = 0.09f;
        new_player->player_light.quadratic = 0.032f;
        new_player->player_light.intensity = 1.f;
        new_player->player_light.flags = 0;

        new_player->player_light.ambient[0] = client->player_color_r;
        new_player->player_light.ambient[1] = client->player_color_g;
        new_player->player_light.ambient[2] = client->player_color_b;

        new_player->player_light.diffuse[0] = client->player_color_r;
        new_player->player_light.diffuse[1] = client->player_color_g;
        new_player->player_light.diffuse[2] = client->player_color_b;

        new_player->player_light.specular[0] = client->player_color_r;
        new_player->player_light.specular[1] = client->player_color_g;
        new_player->player_light.specular[2] = client->player_color_b;
        
        light_add(yaal_state.area, &new_player->player_light);
        new_player->player_light.user_data = new_player;

        animator_create(&new_player->animator);
        animator_set_animation(&new_player->animator, &yaal_state.player_animations[1]);

        if(new_player->player_id == yaal_state.player_id)
            yaal_state.current_player = new_player;
        else
        {
            char welc_txt[256];
            struct chat_message welc_msg;
            snprintf(welc_txt,256,"%s has joined the game",client->client_name);
            strncpy(welc_msg.player_name,"System",64);
            strncpy(welc_msg.message,welc_txt,255);
            welc_msg.player_id = -1;
            welc_msg.msg_time = network->distributed_time;
            chat_new_message(yaal_state.chat, welc_msg);
        }
    }

    glm_vec3_zero(new_player->player_position);

    new_player->player_position[0] = 3*MAP_TILE_SIZE;
    new_player->player_position[2] = 3*MAP_TILE_SIZE;
    glm_vec3_copy(new_player->player_position,new_player->old_position);

    yaal_update_player_transform(new_player);

    new_player->player_health = 6;
    new_player->player_max_health = 6;
    new_player->player_mana = 4;
    new_player->player_max_mana = 4;
    new_player->player_bombs = 10;
    new_player->combat_mode = false;

    // after init
    if(network->mode == NETWORKMODE_SERVER)
    {        
        BOMB_THROW_ACTION_UPDATE(new_player, client, network);     
    }

    client->user_data = (void*)new_player;
}

static void __sglthing_del_player(struct network* network, struct network_client* client)
{
    printf("yaal: [%s] del player %p '%s'\n", (network->mode == NETWORKMODE_SERVER)?"server":"client", client, client->client_name);
    struct player* old_player = (struct player*)client->user_data;

    char welc_txt[256];
    struct chat_message welc_msg;
    snprintf(welc_txt,256,"%s has left the game",client->client_name);
    strncpy(welc_msg.player_name,"System",64);
    strncpy(welc_msg.message,welc_txt,255);
    welc_msg.player_id = -1;
    welc_msg.msg_time = network->distributed_time;
    chat_new_message(yaal_state.chat, welc_msg);

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
                    if(dist > 0.00001f)
                    {
                        //printf("yaal: [%s] tx position %f\n", (network->mode == NETWORKMODE_SERVER)?"server":"client", dist);
                        struct network_packet upd_pak;
                        struct xtra_packet_data* x_data2 = (struct xtra_packet_data*)&upd_pak.packet.data;
                        x_data2->packet.update_position.player_id = -1;
                        upd_pak.meta.packet_type = YAAL_UPDATE_POSITION;
                        vec3 delta;
                        glm_vec3_sub(yaal_state.current_player->player_position, yaal_state.current_player->old_position, delta);
                        glm_vec3_copy(delta, x_data2->packet.update_position.delta_pos);
                        glm_vec3_copy(yaal_state.current_player->player_position, yaal_state.current_player->old_position);
                        network_transmit_packet(network, client, upd_pak);
                        yaal_update_player_transform(yaal_state.current_player);
                    }
                }
            }
            return true;
        case PACKETTYPE_SERVERINFO:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                yaal_state.player_id = packet->packet.serverinfo.player_id;
                printf("yaal: our player id: %i\n", yaal_state.player_id);

                char welc_txt[256];
                struct chat_message welc_msg;
                snprintf(welc_txt,256,"You have joined the %s", packet->packet.serverinfo.server_name);
                strncpy(welc_msg.player_name,"System",64);
                strncpy(welc_msg.message,welc_txt,255);
                welc_msg.player_id = -1;
                welc_msg.msg_time = network->distributed_time;
                chat_new_message(yaal_state.chat, welc_msg);
                snprintf(welc_txt,256,"%s", packet->packet.serverinfo.server_motd);
                strncpy(welc_msg.message,welc_txt,255);
                chat_new_message(yaal_state.chat, welc_msg);
            }
            return true;
        case YAAL_LEVEL_DATA:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                memcpy(&yaal_state.map_data[x_data->packet.yaal_level_data.yaal_x], &x_data->packet.yaal_level_data.data[0], sizeof(x_data->packet.yaal_level_data));
                yaal_state.map_downloaded_count++;
                yaal_state.map_downloaded[x_data->packet.yaal_level_data.yaal_x] = true;
                for(int i = 0; i < MAP_SIZE_MAX_X; i++)
                {
                    yaal_state.map_downloading = false;
                    if(!yaal_state.map_downloaded[i])
                    {
                        yaal_state.map_downloading = true;
                        break;
                    }
                }
                for(int i = 0; i < MAP_SIZE_MAX_Y; i++)
                {
                    vec3 map_pos = { x_data->packet.yaal_level_data.yaal_x * MAP_TILE_SIZE, 0.f, i * MAP_TILE_SIZE };
                    struct map_tile_data* d = &yaal_state.map_data[x_data->packet.yaal_level_data.yaal_x][i];
                    if(d->tile_light_type != 0)
                    {
                        struct light* tile_light = (struct light*)malloc(sizeof(struct light));  
                        glm_vec3_copy(map_pos, tile_light->position);                  
                        switch(d->tile_light_type)
                        {
                            case LIGHT_OBJ4_LAMP:
                            {
                                tile_light->constant = 1.f;
                                tile_light->linear = 0.14f;
                                tile_light->quadratic = 0.07f;
                                tile_light->intensity = 1.f;
                                tile_light->flags = 0;

                                tile_light->ambient[0] = 0.8f;
                                tile_light->ambient[1] = 0.9f;
                                tile_light->ambient[2] = 0.0f;

                                tile_light->diffuse[0] = 0.9f;
                                tile_light->diffuse[1] = 0.9f;
                                tile_light->diffuse[2] = 0.1f;

                                tile_light->specular[0] = 0.9f;
                                tile_light->specular[1] = 0.9f;
                                tile_light->specular[2] = 0.4f;                    

                                tile_light->position[1] = 1.0f;
                            }
                                break;
                            default:
                                break;
                        }
                        light_add(yaal_state.area, tile_light);
                    }
                }
            }
            else
            {
                struct map_file_data* map = g_hash_table_lookup(server_state.maps, &x_data->packet.yaal_level_data.level_id);
                if(map)
                {
                    // printf("yaal: sending %i bytes in level data from id %i\n", sizeof(x_data->packet.yaal_level_data), x_data->packet.yaal_level_data.level_id);
                    memcpy(&x_data->packet.yaal_level_data.data[0], &map->map_row[x_data->packet.yaal_level_data.yaal_x].data[0], sizeof(x_data->packet.yaal_level_data));
                    network_transmit_packet(network, client, *packet);
                }
                else
                {
                    printf("yaal: unknown map %i\n", x_data->packet.yaal_level_data.level_id);
                }
            }
            return false;
        case YAAL_ENTER_LEVEL:
            if(network->mode == NETWORKMODE_SERVER)
            {

            }
            else
            {
                printf("yaal: entering level %s (%i)\n", x_data->packet.yaal_level.level_name, x_data->packet.yaal_level.level_id);

                yaal_state.current_level_id = x_data->packet.yaal_level.level_id;
                yaal_state.map_downloaded_count = 0;
                strncpy(yaal_state.level_name, x_data->packet.yaal_level.level_name, 64);

#define LEVEL_TRANSMISSION
#ifdef LEVEL_TRANSMISSION
                yaal_state.map_downloading = true;
                for(int i = 0; i < MAP_SIZE_MAX_X; i++)
                {
                    yaal_state.map_downloaded[i] = false;

                    struct network_packet upd_pak;
                    struct xtra_packet_data* x_data2 = (struct xtra_packet_data*)&upd_pak.packet.data;
                    upd_pak.meta.packet_type = YAAL_LEVEL_DATA;
                    x_data2->packet.yaal_level_data.level_id = yaal_state.current_level_id;
                    x_data2->packet.yaal_level_data.yaal_x = i;
                    network_transmit_packet(network, client, upd_pak);
                }
#endif

                for(int map_x = 0; map_x < MAP_SIZE_MAX_X; map_x++)
                    for(int map_y = 0; map_y < MAP_SIZE_MAX_Y; map_y++)
                    {
                        struct map_tile_data* map_tile = &yaal_state.map_data[map_x][map_y];
                        map_tile->map_tile_type = TILE_DEBUG;
                    }
            }
            return false;
        case YAAL_UPDATE_PLAYERANIM:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                struct network_client* upd_client = g_hash_table_lookup(network->players, &x_data->packet.update_playeranim.player_id);
                if(upd_client)
                {
                    struct player* upd_player = (struct player*)upd_client->user_data;
                    if(upd_player)
                        animator_set_animation(&upd_player->animator, &yaal_state.player_animations[x_data->packet.update_playeranim.animation_id]);
                }
            }
            break;
        case YAAL_UPDATE_POSITION:
            if(network->mode == NETWORKMODE_SERVER)
            {
                if(!client_player)
                    printf("yaal: client without client_player is attempting to update position\n");
                //printf("yaal: [server] updating player position %i\n", client_player->player_id);

                vec3 new_position;
                glm_vec3_add(client_player->player_position, x_data->packet.update_position.delta_pos, new_position);

                int map_tile_x = (int)roundf(new_position[0] / (MAP_TILE_SIZE));
                int map_tile_y = (int)roundf(new_position[2] / (MAP_TILE_SIZE));

                bool collide;
                if(map_tile_x < MAP_SIZE_MAX_X && map_tile_y < MAP_SIZE_MAX_Y && map_tile_x >= 0 && map_tile_y >= 0)
                {
                    struct map_file_data* map = g_hash_table_lookup(server_state.maps, &client_player->level_id);
                    if(!map)
                        printf("yaal: client out of this world (%i, id %i)\n", client_player->player_id, client_player->level_id);
                    struct map_tile_data* tile = &map->map_row[map_tile_x].data[map_tile_y];
                    if(glm_vec3_distance(new_position, client_player->player_position) > 1.5f)
                        collide = true;
                    else                           
                        collide = determine_tile_collision(*tile);
                }
                else
                    collide = true;

                if(!collide)
                {
                    glm_vec3_copy(new_position, client_player->player_position);
                    glm_vec3_copy(new_position, client_player->old_position);
                }
                else
                    printf("collision\n");

                struct network_packet upd_pak;
                struct xtra_packet_data* x_data2 = (struct xtra_packet_data*)&upd_pak.packet.data;
                upd_pak.meta.packet_type = YAAL_UPDATE_POSITION;
                glm_vec3_copy(client_player->player_position, x_data2->packet.update_position.delta_pos);
                x_data2->packet.update_position.player_id = client->player_id;
                x_data2->packet.update_position.urgent = !collide;
                x_data2->packet.update_position.lag = client->lag;
                x_data2->packet.update_position.level_id = client_player->level_id;
                upd_pak.meta.acknowledge = true;
                network_transmit_packet_all(network, upd_pak);
            }
            else
            {
                struct network_client* upd_client = g_hash_table_lookup(network->players, &x_data->packet.update_position.player_id);
                if(upd_client)
                {
                    upd_client->lag = x_data->packet.update_position.lag;
                    //if(upd_client->player_id != yaal_state.player_id)
                    //    printf("yaal: updating client that is not user\n");
                    struct player* upd_player = (struct player*)upd_client->user_data;
                    upd_player->level_id = x_data->packet.update_position.level_id;
                    if(upd_player->player_id != yaal_state.player_id || (glm_vec3_distance(x_data->packet.update_position.delta_pos, upd_player->old_position) > 1.f || !x_data->packet.update_position.urgent))
                    {
                        glm_vec3_copy(x_data->packet.update_position.delta_pos, upd_player->old_position);
                        glm_vec3_copy(x_data->packet.update_position.delta_pos, upd_player->player_position);
                        if(upd_player->level_id == yaal_state.current_level_id)
                            upd_player->player_light.disable = false;                        
                        else
                            upd_player->player_light.disable = true;
                        //printf("yaal: updating player %i position (%f,%f,%f)\n", upd_client->player_id, upd_player->old_position[0], upd_player->old_position[1], upd_player->old_position[2]);
                        yaal_update_player_transform(upd_player);
                    }
                }
                else
                    printf("yaal: server tried to update a client that doesnt exist (%i)\n", x_data->packet.update_position.player_id);
            }
            return false;
        case YAAL_UPDATE_COMBAT_MODE:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                struct network_client* upd_client = g_hash_table_lookup(network->players, &x_data->packet.update_combat_mode.player_id);
                if(upd_client)
                {
                    struct player* upd_player = (struct player*)upd_client->user_data;
                    if(upd_player)
                    {
                        upd_player->combat_mode = x_data->packet.update_combat_mode.combat_mode;
                        if(upd_player->player_id == yaal_state.current_player->player_id)
                        {
                            if(!x_data->packet.update_combat_mode.combat_mode)
                            {
                                yaal_state.current_aiming_action = -1;
                                yaal_state.player_action_disable = false;
                            }
                        }
                    }
                }
            }
            else
            {
                if(!client_player)
                    return false;
                client_player->combat_mode = x_data->packet.update_combat_mode.combat_mode;

                struct network_packet upd_pak;
                struct xtra_packet_data* x_data2 = (struct xtra_packet_data*)&upd_pak.packet.data;
                upd_pak.meta.packet_type = YAAL_UPDATE_COMBAT_MODE;
                x_data2->packet.update_combat_mode.player_id = client->player_id;
                x_data2->packet.update_combat_mode.combat_mode = client_player->combat_mode;
                struct map_file_data* map = g_hash_table_lookup(server_state.maps, &client_player->level_id);
                if(!client_player->client->debugger)
                    if(map->map_pvp_enabled == false)
                    {
                        x_data2->packet.update_combat_mode.combat_mode = false;
                        client_player->combat_mode = false;
                    }
                network_transmit_packet_all(network, upd_pak);            
            }
            return false;
        case YAAL_UPDATE_PLAYER_ACTION:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                if(x_data->packet.player_update_action.hotbar_id > 9)
                    return false;
                memcpy(&yaal_state.player_actions[x_data->packet.player_update_action.hotbar_id],&x_data->packet.player_update_action.new_action_data,sizeof(struct player_action));                
            }
            return false;
        case YAAL_PLAYER_ACTION:
            if(network->mode == NETWORKMODE_SERVER)
            {
                x_data->packet.player_action.player_id = client->player_id;
                x_data->packet.player_action.level_id = client_player->level_id;
                if(__work_action(packet, network, client))
                    network_transmit_packet_all(network, *packet);            
            }
            else
            {
                struct network_client* upd_client = g_hash_table_lookup(network->players, &x_data->packet.player_action.player_id);
                if(x_data->packet.player_action.level_id == yaal_state.current_level_id)
                    __work_action(packet, network, upd_client);
            }
            return false;
        case YAAL_RPG_MESSAGE:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                strncpy(yaal_state.rpg_message, x_data->packet.rpg_message.text, 255);
                yaal_state.show_rpg_message = true;
            }
            return false;
        case PACKETTYPE_CHAT_MESSAGE:
            if(network->mode == NETWORKMODE_CLIENT)
            {
                struct chat_message in_msg;
                in_msg.player_id = packet->packet.chat_message.player_id;
                strncpy(in_msg.player_name, packet->packet.chat_message.client_name, 64);
                strncpy(in_msg.message, packet->packet.chat_message.message, 255);
                chat_new_message(yaal_state.chat, in_msg);
            }
            return true;
        default: // unknown packet, for system
            // printf("yaal: [%s] unknown packet %i\n", (network->mode == NETWORKMODE_SERVER)?"server":"client", packet->meta.packet_type);
            return true;
    }
    return true;
}

void net_init(struct world* world)
{
    world->client.receive_packet_callback = __sglthing_packet;
    world->client.new_player_callback = __sglthing_new_player;
    world->client.del_player_callback = __sglthing_del_player;

    world->server.receive_packet_callback = __sglthing_packet;
    world->server.new_player_callback = __sglthing_new_player;
    world->server.del_player_callback = __sglthing_del_player;
}

void net_players_in_radius(GArray* clients, float radius, vec3 position, int level_id, GArray* out)
{
    if(out->len != 0)
        g_array_remove_range(out, 0, out->len);
    for(int i = 0; i < clients->len; i++)
    {
        struct network_client* client = &g_array_index(clients, struct network_client, i);
        if(client->user_data)
        {
            struct player* player = (struct player*)client->user_data;
            if(player->level_id == level_id)
            {
                float distance = glm_vec3_distance(position, player->old_position);
                if(distance < radius)
                {
                    struct net_radius_detection det;
                    det.distance = distance;
                    det.client = client;
                    g_array_append_val(out, det);
                }
            }
        }
    }
}