#ifndef CHAT_H
#define CHAT_H
#include <glib.h>
#include "yaal.h"

struct chat_message
{
    int player_id;
    char player_name[64];
    char message[255];
    double msg_time;
};

struct chat_system
{
    GArray* chat_messages;
    int max_messages;
};

void chat_render_player(struct world* world, struct chat_system* chat, struct player* player, mat4 matrix);
void chat_render(struct world* world, struct chat_system* chat);
void chat_new_message(struct chat_system* chat, struct chat_message message);
void chat_init(struct chat_system* chat);

#endif 