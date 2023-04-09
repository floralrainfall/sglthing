#include "light.h"
#include <glad/glad.h>
#include "world.h"

#define MAX_SCENE_LIGHTS 255

struct light_area* light_create_area()
{
    struct light_area* new_area = malloc(sizeof(struct light_area));
    for(int i = 0; i < MAX_LIGHTS; i++)
        new_area->active_lights[i] = 0;
    new_area->mask = 0;
    new_area->scene_lights = g_array_new(true, true, sizeof(struct light*));
    return new_area;
}

static vec3 __c_position;
static int __light_sort(gconstpointer a, gconstpointer b)
{
    struct light* a_l = (struct light*)a;
    struct light* b_l = (struct light*)b;
    float a_dist = glm_vec3_distance(a_l->position, __c_position);
    float b_dist = glm_vec3_distance(b_l->position, __c_position);

    if(a_dist > b_dist)
        return 1;
    if(a_dist == b_dist)
        return 0;
    if(a_dist < b_dist)
        return -1;
}

void light_update(struct light_area* area, vec3 position)
{
    if(!area)
        return;

    __c_position[0] = position[0];
    __c_position[1] = position[1];
    __c_position[2] = position[2];
    g_array_sort(area->scene_lights, __light_sort);
    int lights_allocated_in_area = 0;
    for(int i = 0; i < area->scene_lights->len; i++)
    {
        struct light* l = g_array_index(area->scene_lights, struct light*, i);
        area->active_lights[lights_allocated_in_area] = 0;
        if((l->flags & area->mask) == 0)
        {
            float dist = glm_vec3_distance(position,l->position);
            if(dist < 64.f)
            {
                area->active_lights[lights_allocated_in_area] = l;
                lights_allocated_in_area++;
                if(lights_allocated_in_area == 4)
                    break;
            }
        }
    }
}

void light_add(struct light_area* area, struct light* light)
{
    g_array_append_val(area->scene_lights, light);
}

void light_del(struct light_area* area, struct light* light)
{
    for(int i = 0; i < area->scene_lights->len; i++)
    {
        struct light* l = g_array_index(area->scene_lights, struct light*, i);
        if(l == light)
        {
            g_array_remove_index(area->scene_lights, i);
            return;
        }
    }
}

void light_set_uniforms(int id, struct light* light, int shader_program)
{
    char uniform_name[255];
    snprintf(uniform_name,64,"lights[%i].position",id);
    glUniform3fv(glGetUniformLocation(shader_program,uniform_name), 1, light->position);

    snprintf(uniform_name,64,"lights[%i].ambient",id);
    glUniform3fv(glGetUniformLocation(shader_program,uniform_name), 1, light->ambient);
    snprintf(uniform_name,64,"lights[%i].diffuse",id);
    glUniform3fv(glGetUniformLocation(shader_program,uniform_name), 1, light->diffuse);
    snprintf(uniform_name,64,"lights[%i].specular",id);
    glUniform3fv(glGetUniformLocation(shader_program,uniform_name), 1, light->specular);

    snprintf(uniform_name,64,"lights[%i].constant",id);
    glUniform1f(glGetUniformLocation(shader_program,uniform_name), light->constant);
    snprintf(uniform_name,64,"lights[%i].linear",id);
    glUniform1f(glGetUniformLocation(shader_program,uniform_name), light->linear);
    snprintf(uniform_name,64,"lights[%i].quadratic",id);
    glUniform1f(glGetUniformLocation(shader_program,uniform_name), light->quadratic);
    snprintf(uniform_name,64,"lights[%i].intensity",id);
    glUniform1f(glGetUniformLocation(shader_program,uniform_name), light->intensity);

    snprintf(uniform_name,64,"lights[%i].present",id);
    glUniform1f(glGetUniformLocation(shader_program,uniform_name), 1.f);
    
    while(glGetError()!=0);
}

void light_area_set_uniforms(struct light_area* area, int shader_program)
{
    for(int i = 0; i < MAX_LIGHTS; i++)
    {
        char uniform_name[64];
        while(glGetError()!=0);
        if(area->active_lights[i])
        {
            light_set_uniforms(i,area->active_lights[i],shader_program);
        }
        else
        {
            snprintf(uniform_name,64,"lights[%i].present",i);
            glUniform1f(glGetUniformLocation(shader_program,uniform_name), 0.f);
        }
    }
}