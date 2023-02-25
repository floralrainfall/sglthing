#include "light.h"
#include <glad/glad.h>
#include "world.h"

// FIXME: using a normal array like this is not a good option for this task but i dont know how to implement a good list
#define MAX_SCENE_LIGHTS 255
struct light* scene_lights[MAX_SCENE_LIGHTS];
int scene_lights_allocated = 0;

struct light_area* light_create_area()
{
    struct light_area* new_area = malloc(sizeof(struct light_area));
    for(int i = 0; i < MAX_LIGHTS; i++)
        new_area->active_lights[i] = 0;
    new_area->mask = 0;
    return new_area;
}

void light_update(struct light_area* area, vec3 position)
{
    int lights_allocated_in_area = 0;
    for(int i = 0; i < scene_lights_allocated; i++)
    {
        area->active_lights[lights_allocated_in_area] = 0;
        if((scene_lights[i]->flags & area->mask) == 0)
        {
            float dist = glm_vec3_distance(position,scene_lights[i]->position);
            if(dist < 1000.f)
            {
                area->active_lights[lights_allocated_in_area] = scene_lights[i];
                lights_allocated_in_area++;
                if(lights_allocated_in_area == 4)
                    break;
            }
        }
    }
}

void light_add(struct light* light)
{
    scene_lights[scene_lights_allocated] = light;
    scene_lights_allocated++;
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
            light_set_uniforms(i,area->active_lights[i],shader_program);
        else
        {
            snprintf(uniform_name,64,"lights[%i].present",i);
            glUniform1f(glGetUniformLocation(shader_program,uniform_name), 0.f);
        }
    }
}