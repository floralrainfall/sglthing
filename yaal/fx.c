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
}

void add_fx(struct fx_manager* manager, struct fx fx)
{
    fx.time = 0.f;
    fx.alive = true;
    g_array_append_val(manager->fx_active, fx);
}

void render_fx(struct world* world, struct fx_manager* manager)
{
    for(int i = 0; i < manager->fx_active->len; i++)
    {
        struct fx *fx = &g_array_index(manager->fx_active, struct fx, i);
        
        fx->time += world->delta_time * fx->speed;

        if(fx->alive == false)
            continue;

        if(fx->time > fx->max_time)
        {
            fx->alive = false;        
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
                    if(world->server.status == NETWORKSTATUS_CONNECTED)
                    {
                        GArray* detections = NEW_RADIUS;
                        net_players_in_radius(world->server.server_clients, 32.f, fx->target, fx->level_id, detections);
                        for(int i = 0; i < detections->len; i++)
                        {
                            struct net_radius_detection detection = g_array_index(detections, struct net_radius_detection, i);

                            net_player_hurt(&world->server, detection.client, roundf(5.f/detection.distance));
                        }
                        g_array_free(detections, true);
                    }
                    break;
                default:
                    break;
            }
        }
        else
        {
            mat4 fx_mat;
            glm_mat4_identity(fx_mat);
            fx->progress = fx->time / fx->max_time;            
            vec3 dest_pos;
            glm_vec3_mix(fx->start, fx->target, fx->progress, dest_pos);
            glm_translate(fx_mat, dest_pos);

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
                    fx->alive = false;        
                    break;
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
}