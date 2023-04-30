#include "light.h"
#include <glad/glad.h>
#include "world.h"

struct light_area* light_create_area()
{
    struct light_area* new_area = malloc(sizeof(struct light_area));
    for(int i = 0; i < MAX_LIGHTS; i++)
        new_area->active_lights[i] = 0;
    new_area->mask = 0;
    new_area->scene_lights = g_array_new(true, true, sizeof(struct light*));
    return new_area;
}

static int __light_sort(gconstpointer a, gconstpointer b, gpointer user_data)
{
    float* position = (float*)user_data;
    struct light* a_l = *(struct light**)a;
    struct light* b_l = *(struct light**)b;

    float a_dist = glm_vec3_distance(a_l->position, position);
    float b_dist = glm_vec3_distance(b_l->position, position);

    // printf("(%f, %f, %f) %p %f (%f,%f,%f) %p %f (%f,%f,%f)\n", position[0], position[1], position[2], a_l, a_dist, a_l->position[0], a_l->position[1], a_l->position[2], b_l, b_dist, b_l->position[0], b_l->position[1], b_l->position[2]);

    if(a_l->disable)
        return -1;
    if(b_l->disable)
        return 1;

    return a_dist - b_dist;
}

void light_update(struct light_area* area, vec3 position)
{
    if(!area)
        return;

    g_array_sort_with_data(area->scene_lights, __light_sort, position);
    int lights_allocated_in_area = 0;
    for(int i = 0; i < area->scene_lights->len; i++)
    {
        struct light* l = g_array_index(area->scene_lights, struct light*, i);
        area->active_lights[lights_allocated_in_area] = 0;
        if((l->flags & area->mask) == 0)
        {
            float dist = glm_vec3_distance(position,l->position);
            area->active_lights[lights_allocated_in_area] = l;
            area->active_lights[lights_allocated_in_area]->distance = dist;
            lights_allocated_in_area++;
            if(lights_allocated_in_area == MAX_LIGHTS)
                break;
        }
    }
}

void light_add(struct light_area* area, struct light* light)
{
    light->disable = false;
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
    snprintf(uniform_name,64,"lights[%i].dist",id);
    glUniform1f(glGetUniformLocation(shader_program,uniform_name), light->distance);

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
        if(area->active_lights[i] && area->active_lights[i]->disable == false)
        {
            light_set_uniforms(i,area->active_lights[i],shader_program);
        }
        else
        {
            snprintf(uniform_name,64,"lights[%i].present",0.0);
            glUniform1f(glGetUniformLocation(shader_program,uniform_name), 0.f);
        }
    }
}