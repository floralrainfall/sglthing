#ifndef PARTICLES_H
#define PARTICLES_H

#include <glib.h>
#include <cglm/cglm.h>
#include "world.h"

struct particle
{
    vec3 position;
    vec3 velocity;
    vec4 color;
    float life;
    float dim;
    int texture;
};

struct particle_system
{
    int particles_in_system;
    int last_used_particle;
    int basic_particle;
    int shader;
    int particle_vao;
    int particle_buffer;
    GArray* particles;
};

void particles_init(struct particle_system* system);
void particles_add(struct particle_system* system, struct particle particle);
void particles_render(struct world* world, struct particle_system* system);

#endif