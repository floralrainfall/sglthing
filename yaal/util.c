#include "util.h"
#include "netmgr.h"

void yaal_update_player_action(int key_id, struct player_action action, struct network_client* client, struct network* network)
{
    struct network_packet upd_pak;
    struct xtra_packet_data* x_data2 = (struct xtra_packet_data*)&upd_pak.packet.data;
    upd_pak.meta.packet_type = YAAL_UPDATE_PLAYER_ACTION;
    upd_pak.meta.packet_size = sizeof(x_data2->packet.player_update_action);
    x_data2->packet.player_update_action.hotbar_id = key_id;
    memcpy(&x_data2->packet.player_update_action.new_action_data, &action, sizeof(struct player_action));
    network_transmit_packet(network, client, &upd_pak);
}

void yaal_update_player_transform(struct player* player)
{
    profiler_event("yaal_update_player_transform");
    mat4 rotation_matrix;
    glm_mat4_ucopy(player->model_matrix,(mat4){
        {1.0f,0.f,0.f,1.0f},
        {0.0f,1.f,0.f,1.0f},
        {0.0f,0.f,1.f,1.0f},
        {1.0f,1.f,1.f,0.0f}
    });

    glm_translate(player->model_matrix, player->old_position);
    glm_translate_y(player->model_matrix, 0.0f);

    glm_mat4_mul(player->model_matrix, rotation_matrix, player->model_matrix);

    glm_vec3_copy(player->player_position, player->player_light.position);
    player->player_light.position[1] = 1.5f;
    profiler_end();
}

void yaal_update_song()
{
    char song_fname[128];
    snprintf(song_fname,128,"yaal/snd/mus/theme_%i.mp3",yaal_state.current_map_song_id);                
    if(yaal_state.current_song)
        if(strcmp(song_fname,yaal_state.current_song->name)==0)
            return;
    switch(yaal_state.mode)
    {
        case YAAL_STATE_MENU:
        case YAAL_STATE_MAP:
            if(yaal_state.current_song)
            {
                kill_snd(yaal_state.current_song);
            }
            yaal_state.current_song = play_snd("yaal/snd/mus/theme_0.mp3");
            break;
        case YAAL_STATE_GAME:
            if(yaal_state.current_song)
            {
                kill_snd(yaal_state.current_song);
            }
            yaal_state.current_song = play_snd(song_fname);
            break;
    }
    if(yaal_state.current_song)
    {
        yaal_state.current_song->multiplier = 0.1f;
        yaal_state.current_song->loop = true;
    }
}