#ifndef LIGHT_H
#define LIGHT_H

#define MAX_LIGHTS 32
#include <cglm/cglm.h>
#include <stdbool.h>
#include <glib.h>

struct light {
    vec3 position;

    float constant;
    float linear;
    float quadratic;
    float intensity;
    float distance;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    bool disable;
    int flags;
    void* user_data;
};

struct light_area {
    int mask;
    struct light* active_lights[MAX_LIGHTS];
    GArray* scene_lights;
};

struct light_area* light_create_area();
void light_update(struct light_area* area, vec3 position);
void light_set_uniforms(int id, struct light* light, int shader_program);
void light_area_set_uniforms(struct light_area* area, int shader_program);
void light_add(struct light_area* area, struct light* light);
void light_del(struct light_area* area, struct light* light);

#endif