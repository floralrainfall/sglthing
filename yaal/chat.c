#include "chat.h"
#ifndef HEADLESS
#include <GLFW/glfw3.h>
#endif

void chat_render_player(struct world* world, struct chat_system* chat, struct player* player, mat4 matrix)
{
    if(world->gfx.shadow_pass)
        return;
    profiler_event("chat_render_player");
    char tx[256];
    for(int i = chat->chat_messages->len-1; i >= 0; i--)
    {
        struct chat_message msg = g_array_index(chat->chat_messages, struct chat_message, i);
        if(msg.player_id == player->player_id && world->time - msg.msg_time < 16.0)
        {
            snprintf(tx,256,"'%s'",msg.message);
            ui_draw_text_3d(world->ui, world->viewport, world->cam.position, world->cam.front, (vec3){0.f,2.f,0.f}, world->cam.fov, player->model_matrix, world->vp, tx);
            break;
        }
    }
    profiler_end();
}

void chat_new_message(struct chat_system* chat, struct chat_message message)
{
    g_array_append_val(chat->chat_messages, message);
    if(yaal_state.mode == YAAL_STATE_GAME)
    {
        struct snd* notif_sound = play_snd("yaal/snd/notification.wav");
        notif_sound->multiplier = 0.1f;
    }
}

void chat_init(struct chat_system* chat)
{
    chat->chat_messages = g_array_new(true, true, sizeof(struct chat_message));
    chat->max_messages = 16;
    for(int i = 0; i < 2; i++)
        chat_new_message(chat, (struct chat_message){.player_id = -1, .message = "Welcome to YaalOnline.", .player_name = "System", .msg_time = 0.f});
}

void chat_render(struct world* world, struct chat_system* chat)
{
    if(chat->chat_messages->len == 0)
        return;
    profiler_event("chat_render");
    vec4 oldfg, oldbg;
    glm_vec4_copy(world->ui->foreground_color, oldfg);
    glm_vec4_copy(world->ui->background_color, oldbg);
    char tx[256];   
    int id = MIN(chat->max_messages,chat->chat_messages->len);
    int max = id;
    for(int i = chat->chat_messages->len-1; i > chat->chat_messages->len-max; i--)
    {
        struct chat_message msg = g_array_index(chat->chat_messages, struct chat_message, i);
        snprintf(tx,256,"%s: '%s'",msg.player_name, msg.message);
        ui_draw_text(world->ui, 0.f, world->gfx.screen_height-24.f-(id*16.f), tx, 1.f);
        id--;
    }
    glm_vec4_copy(oldfg, world->ui->foreground_color);
    glm_vec4_copy(oldbg, world->ui->background_color);
    profiler_end();
}