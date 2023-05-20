#include "world.h"
#include "particles.h"
#include "sglthing.h"

#define MAX_PARTICLES_IN_SYSTEM 512

struct vtx
{
    float pos[2];
    float uv[2];
};

struct vtx particle_vtx[] = {
    { .pos = {0.5f, -0.5f}, .uv = {1.f, 0.f} },
    { .pos = {-0.5f, -0.5f}, .uv = {0.f, 0.f} },
    { .pos = {-0.5f, 0.5f}, .uv = {0.f, 1.f} },

    { .pos = {0.5f, -0.5f}, .uv = {1.f, 0.f} },
    { .pos = {-0.5f, 0.5f}, .uv = {0.f, 1.f} },
    { .pos = {0.5f, 0.5f}, .uv = {1.f, 1.f} },
};

void particles_init(struct particle_system* system)
{
    int vtx = compile_shader("shaders/particle.vs", GL_VERTEX_SHADER);
    int frg = compile_shader("shaders/particle.fs", GL_FRAGMENT_SHADER);
    system->shader = link_program(vtx, frg);
    system->particles = g_array_new(true, true, sizeof(struct particle));
    system->particles_in_system = MAX_PARTICLES_IN_SYSTEM;
    for(int i = 0; i < MAX_PARTICLES_IN_SYSTEM; i++)
    {
        struct particle particle;
        g_array_insert_val(system->particles, i, particle);
    }

    load_texture("basic_particle.png");
    system->basic_particle = get_texture("basic_particle.png");

    sglc(glGenBuffers(1, &system->particle_buffer));
    sglc(glGenVertexArrays(1, &system->particle_vao));

    sglc(glBindVertexArray(system->particle_vao));
    sglc(glBindBuffer(GL_ARRAY_BUFFER, system->particle_buffer));
    sglc(glBufferData(GL_ARRAY_BUFFER, sizeof(particle_vtx), &particle_vtx[0], GL_STATIC_DRAW));
    
    sglc(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct vtx), (void*)0)); // position
    sglc(glEnableVertexAttribArray(0));
    sglc(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct vtx), (void*)offsetof(struct vtx, uv))); // uv
    sglc(glEnableVertexAttribArray(1));
}

static int __first_unused_particle(struct particle_system* system)
{
    for(int i = system->last_used_particle; i < MAX_PARTICLES_IN_SYSTEM; i++)
    {
        struct particle particle = g_array_index(system->particles, struct particle, i);
        if (particle.life <= 0.0f){
            system->last_used_particle = i;
            return i;
        }
    }
    for (int i = 0; i < system->last_used_particle; i++) {
        struct particle particle = g_array_index(system->particles, struct particle, i);
        if (particle.life <= 0.0f){
            system->last_used_particle = i;
            return i;
        }
    }    
    system->last_used_particle = 0;
    return 0;
}

void particles_add(struct particle_system* system, struct particle particle)
{
    int pid = __first_unused_particle(system);
    struct particle* _particle = &g_array_index(system->particles, struct particle, pid);
    memcpy(_particle, &particle, sizeof(struct particle));
}

void particles_render(struct world* world, struct particle_system* system)
{
    sglc(glBlendFunc(GL_SRC_ALPHA, GL_ONE));
    sglc(glBindVertexArray(system->particle_vao));  
    sglc(glBindBuffer(GL_ARRAY_BUFFER, system->particle_buffer));
    sglc(glUseProgram(system->shader));
    sglc(glUniformMatrix4fv(glGetUniformLocation(system->shader,"view"), 1, GL_FALSE, world->v[0]));
    sglc(glUniformMatrix4fv(glGetUniformLocation(system->shader,"projection"), 1, GL_FALSE, world->p[0]));       
    sglc(glUniform1f(glGetUniformLocation(system->shader,"fog_maxdist"), world->gfx.fog_maxdist));
    sglc(glUniform1f(glGetUniformLocation(system->shader,"fog_mindist"), world->gfx.fog_mindist));
    sglc(glUniform4fv(glGetUniformLocation(system->shader,"fog_color"), 1, world->gfx.fog_color));
    sglc(glUniform3fv(glGetUniformLocation(system->shader,"camera_position"), 1, world->cam.position));
    sglc(glUniform1i(glGetUniformLocation(system->shader,"banding_effect"), world->gfx.banding_effect));
    mat4 model_matrix;
    for(int i = 0; i < system->particles->len; i++)
    {
        struct particle* particle = &g_array_index(system->particles, struct particle, i);
        particle->life -= world->delta_time;
        if(particle->life > 0.0f)
        {
            profiler_event("particle_render");
            vec3 truevel;
            glm_vec3_mul(particle->velocity, (vec3){world->delta_time,world->delta_time,world->delta_time}, truevel);
            glm_vec3_sub(particle->position, truevel, particle->position);
            particle->color[3] -= particle->dim * world->delta_time;
            particle->color[3] = MAX(particle->color[3], 0.0f);

            glm_mat4_identity(model_matrix);
            vec3 direction;
            glm_vec3_sub(world->cam.position, particle->position,  direction);
            versor quat;
            glm_quat_for(direction, (vec3){0.f,1.f,0.f}, quat);
            glm_translate(model_matrix, particle->position);   
            glm_quat_rotate(model_matrix, quat, model_matrix);     

            sglc(glActiveTexture(GL_TEXTURE0));
            sglc(glBindTexture(GL_TEXTURE_2D, particle->texture));                
            sglc(glUniform3f(glGetUniformLocation(system->shader,"offset"), particle->position[0], particle->position[1], particle->position[2]));
            sglc(glUniform4f(glGetUniformLocation(system->shader,"color"), particle->color[0], particle->color[1], particle->color[2], particle->color[3]));
            sglc(glUniformMatrix4fv(glGetUniformLocation(system->shader,"model"), 1, GL_FALSE, model_matrix[0]));   

            sglc(glDrawArrays(GL_TRIANGLES, 0, 6));
            profiler_end();
        }
    }
    sglc(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
}