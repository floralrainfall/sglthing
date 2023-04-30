#include "util.h"

void yaal_update_player_action(int key_id, struct player_action action, struct network_client* client, struct network* network)
{
    struct network_packet upd_pak;
    struct xtra_packet_data* x_data2 = (struct xtra_packet_data*)&upd_pak.packet.data;
    upd_pak.meta.packet_type = YAAL_UPDATE_PLAYER_ACTION;
    x_data2->packet.player_update_action.hotbar_id = key_id;
    memcpy(&x_data2->packet.player_update_action.new_action_data, &action, sizeof(struct player_action));
    network_transmit_packet(network, client, upd_pak);
}

void yaal_update_player_transform(struct player* player)
{
    glm_mat4_identity(player->model_matrix);
    glm_translate(player->model_matrix, player->old_position);
    glm_translate_y(player->model_matrix, 0.0f);

    glm_vec3_copy(player->player_position, player->player_light.position);
    player->player_light.position[1] = 1.5f;
}