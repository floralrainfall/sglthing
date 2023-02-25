#ifndef LIGHT_H
#define LIGHT_H

#define MAX_LIGHTS 4
#include <cglm/cglm.h>
#include <stdbool.h>

struct light {
    vec3 position;

    float constant;
    float linear;
    float quadratic;
    float intensity;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    int flags;
};

struct light_area {
    int mask;
    struct light* active_lights[MAX_LIGHTS];
};

struct light_area* light_create_area();
void light_update(struct light_area* area, vec3 position);
void light_set_uniforms(int id, struct light* light, int shader_program);
void light_area_set_uniforms(struct light_area* area, int shader_program);
void light_add(struct light* light);

#endif