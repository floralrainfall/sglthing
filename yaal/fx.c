#include "fx.h"
#include "yaal.h"
#include "util.h"
#include <sglthing/sglthing.h>
#include "netmgr.h"

void init_fx(struct fx_manager* manager, int shader, struct particle_system* system)
{
    manager->fx_active = g_array_new(true, true, sizeof(struct fx));
    manager->object_shader = shader;
    manager->system = system;
    load_model("yaal/models/bomb.obj");
    manager->bomb = get_model("yaal/models/bomb.obj");
    manager->server_fx = false;
}

void add_fx(struct fx_manager* manager, struct fx fx)
{
    fx.time = 0.f;
    fx.alive = true;
    fx.progress = 0.f;
    fx.dead = false;

    switch(fx.type)
    {
        case FX_BOMB_THROW:
            if(!manager->server_fx)
            {
                fx.snd = play_snd("yaal/snd/bomb_throw.wav");
                fx.snd->sound_3d = true;
                fx.snd->camera_position = &yaal_state.current_player->player_position;
            }
            break;
    }

    g_array_append_val(manager->fx_active, fx);
}

void tick_fx(struct world* world, struct fx_manager* manager)
{
    profiler_event("tick_fx");
    for(int i = 0; i < manager->fx_active->len; i++)
    {
        struct fx *fx = &g_array_index(manager->fx_active, struct fx, i);
        
        if(fx->time == 0)
            fx->alive = true;

        fx->time += world->delta_time * fx->speed;

        if(fx->time > fx->max_time)
        {
            fx->alive = false;        
            if(!fx->dead)
            {
                switch(fx->type)
                {
                    case FX_BOMB_THROW:
                        add_fx(manager, (struct fx){
                            .max_time = 2.0f,
                            .target[0] = fx->target[0],
                            .target[1] = fx->target[1],
                            .target[2] = fx->target[2],
                            .start[0] = fx->target[0],
                            .start[1] = fx->target[1],
                            .start[2] = fx->target[2],
                            .speed = 1.f,
                            .type = FX_EXPLOSION,
                            .level_id = fx->level_id,
                        });
                        if(manager->server_fx)
                        {
                            GArray* detections = NEW_RADIUS;
                            net_players_in_radius(world->server.server_clients, 32.f, fx->target, fx->level_id, detections);
                            for(int i = 0; i < detections->len; i++)
                            {
                                struct net_radius_detection detection = g_array_index(detections, struct net_radius_detection, i);

                                net_player_hurt(&world->server, detection.client, roundf(5.f/detection.distance), REASON_BOMB, fx->source_player_id);
                            }
                            g_array_free(detections, true);
                        }
                        break;
                    default:
                        break;
                }
                if(fx->snd)
                {
                    stop_snd(fx->snd);
                    free(fx->snd);
                    fx->snd = 0;
                }
                fx->dead = true;
            }
        }
    }

    int old_len = manager->fx_active->len;
    for(int i = 0; i < old_len; i++)
    {
        struct fx* fx = &g_array_index(manager->fx_active, struct fx, 0);
        if(!fx->alive)
        {
            g_array_remove_index(manager->fx_active, 0);
        }
    }
    profiler_end();
}

void render_fx(struct world* world, struct fx_manager* manager)
{
    if(world->gfx.shadow_pass)
        return;
    profiler_event("render_fx");
    for(int i = 0; i < manager->fx_active->len; i++)
    {
        struct fx *fx = &g_array_index(manager->fx_active, struct fx, i);
        if(fx->alive)
        {
            mat4 fx_mat;
            glm_mat4_identity(fx_mat);
            fx->progress = fx->time / fx->max_time;            
            vec3 dest_pos;
            glm_vec3_mix(fx->start, fx->target, fx->progress, dest_pos);
            glm_translate(fx_mat, dest_pos);

            if(fx->snd)
                glm_vec3_copy(dest_pos, fx->snd->sound_position);

            switch(fx->type)
            {
                case FX_BOMB_THROW:
                    float add = sinf(fx->progress*M_PIf)*10.f;
                    glm_translate_y(fx_mat, add);
                    glm_scale(fx_mat, (vec3){0.1f,0.1f,0.1f});
                    world_draw_model(world, manager->bomb, manager->object_shader, fx_mat, true);
                    if(world->frames % 10 == 0)
                        particles_add(manager->system, (struct particle) {
                            .position[0] = dest_pos[0],
                            .position[1] = dest_pos[1]+1.f+add,
                            .position[2] = dest_pos[2],

                            .velocity[0] = g_random_double_range(-5.0,5.0),
                            .velocity[1] = g_random_double_range(-10.0,5.0),
                            .velocity[2] = g_random_double_range(-5.0,5.0),

                            .color[0] = 0.1f,
                            .color[1] = 0.1f,
                            .color[2] = 0.1f,
                            .color[3] = 1.0f,

                            .life = 1.f,
                            .dim = 2.5f,
                            .texture = manager->system->basic_particle
                        });
                    break;
                case FX_EXPLOSION:
                    if(world->frames % 10 == 0)
                    {
                        vec2 r;
                        r[0] = (float)g_random_double_range(0.5,1.0);
                        r[1] = (float)g_random_double_range(0.0,1.0);
                        r[1] = MIN(r[0],r[1]);
                        particles_add(manager->system, (struct particle) {
                            .position[0] = fx->start[0],
                            .position[1] = fx->start[1],
                            .position[2] = fx->start[2],

                            .velocity[0] = g_random_double_range(-5.0,5.0),
                            .velocity[1] = g_random_double_range(-10.0,5.0),
                            .velocity[2] = g_random_double_range(-5.0,5.0),

                            .color[0] = r[0],
                            .color[1] = r[1],
                            .color[2] = 0.f,
                            .color[3] = 1.0f,

                            .life = 1.f,
                            .dim = 2.5f,
                            .texture = manager->system->basic_particle
                        });
                    }
                    break;
                default:
                    break;
            }
        }
    }
    profiler_end();
}

void render_ui_fx(struct world* world, struct fx_manager* manager)
{
    profiler_event("render_ui_fx");
    vec4 oldfg, oldbg;
    glm_vec4_copy(world->ui->foreground_color, oldfg);
    glm_vec4_copy(world->ui->background_color, oldbg);
    mat4 m, mvp;
    vec3 dm;
    glm_mat4_identity(m);
    glm_mat4_mul(world->vp, m, mvp);
    for(int i = 0; i < manager->fx_active->len; i++)
    {
        struct fx *fx = &g_array_index(manager->fx_active, struct fx, i);
        if(fx->alive)
        {
            fx->progress = fx->time / fx->max_time;   
            vec3 dest_pos;
            glm_vec3_mix(fx->start, fx->target, fx->progress, dest_pos);
            glm_project(dest_pos, mvp, world->viewport, dest_pos);
            vec2 fx_position;
            fx_position[0] = dest_pos[0];
            fx_position[1] = dest_pos[1];

            switch(fx->type)
            {
                case FX_HEAL_DAMAGE:
                    world->ui->background_color[3] = 0.f;

                    //world->ui->foreground_color[0] = fx->alt ? 0.016f : 0.502f;
                    //world->ui->foreground_color[1] = fx->alt ? 0.475f : 0.0f;
                    //world->ui->foreground_color[2] = fx->alt ? 0.039f : 0.0f;

                    float player_hearts = fx->amt/2.f;
                    int player_full_hearts = floorf(player_hearts);
                    vec2 offset = {fx_position[0], fx_position[1]+16.f};
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
                    break;
                default:
                    ui_draw_image(world->ui, fx_position[0], fx_position[1], 10, 10, yaal_state.player_heart_full_tex, 1.f);
                    break;
            }
        }
    }
    glm_vec4_copy(oldfg, world->ui->foreground_color);
    glm_vec4_copy(oldbg, world->ui->background_color);
    profiler_end();
}