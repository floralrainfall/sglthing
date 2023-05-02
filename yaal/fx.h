#ifndef FX_H
#define FX_H
#include <stdbool.h>
#include <sglthing/world.h>
#include <sglthing/particles.h>

enum fxtypes
{
    FX_EXPLOSION,
    FX_BOMB_THROW,

};

struct fx
{
    enum fxtypes type;
    vec3 target;
    vec3 start;
    float time;
    float max_time;
    float progress;
    float speed;
    bool alive;
};

struct fx_manager
{
    GArray* fx_active;

    struct particle_system* system;
    int object_shader;
    struct model* bomb;
};

void init_fx(struct fx_manager* manager, int object_shader, struct particle_system* system);
void add_fx(struct fx_manager* manager, struct fx fx);
void render_fx(struct world* world, struct fx_manager* manager);

#endif