#ifndef WORLD_H
#define WORLD_H
#include <cglm/cglm.h>
#include "graphic.h"
#include "model.h"
#include "ui.h"

struct world {
    struct {
        vec3 position;
        float pitch;
        float yaw;
        vec3 up;
        vec3 front;
        vec3 right;
        float fov;
    } cam;

    mat4 v;
    mat4 p;
    mat4 vp;
    vec4 viewport;

    int normal_shader;

    struct ui_data* ui;

    struct model* test_object;

    int render_count;
};

struct world* world_init();

void world_frame(struct world* world);
void world_draw(struct world* world, int count, int vertex_array, int shader_program, mat4 model_matrix);
void world_draw_model(struct world* world, struct model* model, int shader_program, mat4 model_matrix);

#endif