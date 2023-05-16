#ifndef FX_H
#define FX_H
#include <stdbool.h>
#include <sglthing/world.h>
#include <sglthing/particles.h>

enum fxtypes
{
    FX_EXPLOSION,
    FX_BOMB_THROW,
    FX_HEAL_DAMAGE,
};

struct fx
{
    enum fxtypes type;
    int level_id;
    bool alive;
    bool dead;
    char tx[8];
    bool alt;
    int amt;
    float time;
    float max_time;
    float progress;
    float speed;
    vec3 target;
    vec3 start;
    int source_player_id;
    struct snd* snd;
};

struct fx_manager
{
    GArray* fx_active;

    struct particle_system* system;
    int object_shader;
    struct model* bomb;

    bool server_fx;
};

void init_fx(struct fx_manager* manager, int object_shader, struct particle_system* system);
void add_fx(struct fx_manager* manager, struct fx fx);
void tick_fx(struct world* world, struct fx_manager* manager);
void render_fx(struct world* world, struct fx_manager* manager);
void render_ui_fx(struct world* world, struct fx_manager* manager);

#endif