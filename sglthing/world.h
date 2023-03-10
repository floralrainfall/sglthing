#ifndef WORLD_H
#define WORLD_H
#include <cglm/cglm.h>
#include <ode/ode.h>
#include "graphic.h"
#include "model.h"
#include "ui.h"
#include "primitives.h"
#include "light.h"
#include "script.h"
#include "config.h"
#include "net.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define SHADOW_WIDTH 2048
#define SHADOW_HEIGHT 2048

struct world {
    struct {
        vec3 position;
        float pitch;
        float yaw;
        vec3 up;
        vec3 world_up;
        vec3 front;
        vec3 right;
        float fov;
    } cam;

    struct {
        float fog_maxdist;
        float fog_mindist;
        vec4 fog_color;
        vec4 clear_color;
        vec3 sun_direction;
        int banding_effect;
        int screen_width;
        int screen_height;

        int depth_map_fbo;
        int depth_map_texture;    
        int depth_map_fbo_far;
        int depth_map_texture_far;   
        bool shadow_pass;
        int lighting_shader;
        mat4 light_space_matrix;
        mat4 light_space_matrix_far;
        int current_map;

#ifdef FBO_ENABLED
        int hdr_fbo;
        int hdr_color_buffers[2];
        int hdr_pingpong_fbos[2];
        int hdr_pingpong_buffers[2];
        int hdr_blur_shader;
#endif

        int quad_shader;

        GLFWwindow *window;
    } gfx;

    mat4 v;
    mat4 p;
    mat4 vp;
    vec4 viewport;

    struct ui_data* ui;

    struct light_area* render_area;
    struct script_system* script;
    struct primitives primitives;

    int render_count;
    
    double delta_time;
    double last_time;
    double fps;
    int frames_in_second;
    int frames;
    
    struct {
        bool paused;
        dWorldID world;
        dSpaceID space;
        dJointGroupID contactgroup;

        int collisions_in_frame;
    } physics;

    struct network server;
    struct network client;

    struct config_file config;
    struct config_file config_private;
};

struct world* world_init(char** argv, int argc);
void world_deinit(struct world* world);

void world_frame(struct world* world);
void world_frame_render(struct world* world);
void world_frame_light_pass(struct world* world, float quality, int framebuffer, int framebuffer_x, int framebuffer_y);
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