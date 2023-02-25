#ifndef WORLD_H
#define WORLD_H
#include <cglm/cglm.h>
#include <ode/ode.h>
#include "graphic.h"
#include "model.h"
#include "map.h"
#include "ui.h"
#include "primitives.h"
#include "light.h"
#include "script.h"

#define SHADOW_WIDTH 1024
#define SHADOW_HEIGHT 1024

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

    struct {
        float fog_maxdist;
        float fog_mindist;
        vec4 fog_color;
        vec4 clear_color;
        int banding_effect;
        int screen_width;
        int screen_height;

        int depth_map_fbo;
        int depth_map_texture;
    } gfx;

    mat4 v;
    mat4 p;
    mat4 vp;
    vec4 viewport;

    struct ui_data* ui;

    struct model* test_object;
    struct map* test_map;
    struct light_area* render_area;
    struct script_system* script;
    struct primitives primitives;

    int render_count;

    dBodyID player_body;
    dGeomID player_geom;
    dMass player_mass;
    
    double delta_time;

    int hdr_fbo;
    int color_buffers[2];
    int pingpong_fbo[2];
    int pingpong_buffers[2];
    
    struct {
        bool paused;
        dWorldID world;
        dSpaceID space;
        dBodyID body;
        dGeomID geom;
        dMass m;
        dJointGroupID contactgroup;

        int collisions_in_frame;
    } physics;
};

struct world* world_init();

void world_frame(struct world* world);
void world_frame_render(struct world* world);
void world_draw(struct world* world, int count, int vertex_array, int shader_program, mat4 model_matrix);
void world_draw_model(struct world* world, struct model* model, int shader_program, mat4 model_matrix, bool set_textures);

enum primitive_type
{
    PRIMITIVE_BOX,
    PRIMITIVE_ARROW,
    PRIMITIVE_PLANE,
};
void world_draw_primitive(struct world* world, int shader, int fill, enum primitive_type type, mat4 model_matrix, vec4 color);

#endif