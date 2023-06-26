#ifndef WORLD_H
#define WORLD_H
#include <cglm/cglm.h>
#ifdef ODE_ENABLED
#include <ode/ode.h>
#endif
#include "graphic.h"
#include "model.h"
#include "ui.h"
#include "primitives.h"
#include "light.h"
#include "config.h"
#include "net.h"
#include "script.h"
#include "snd.h"
#include "prof.h"
#include "memory.h"
#ifndef HEADLESS
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif

#define SHADOW_WIDTH 2048
#define SHADOW_HEIGHT 2048

enum world_state {
    WORLD_STATE_MAINMENU,
    WORLD_STATE_GAME,
};

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
        float random_shake;
        float wobble; // yaw = sin(2t), pitch = cos(t) where t is time in seconds
        float lsd;
        bool lock;
    } cam;

    struct {
        float far_boundary;
        float near_boundary;
        float fog_maxdist;
        float fog_mindist;
        vec4 fog_color;
        vec4 clear_color;
        vec3 sun_direction;
        vec3 ambient;
        vec3 diffuse;
        vec3 specular;
        int banding_effect;
        int screen_width;
        int screen_height;
        int white_texture;
        int alpha_texture;

        bool enable_sun;
        vec3 sun_position;
        int depth_map_fbo;
        int depth_map_texture;    
        int depth_map_fbo_far;
        int depth_map_texture_far;   
        bool shadow_pass;
        int lighting_shader;
        mat4 light_space_matrix;
        mat4 light_space_matrix_far;
        int current_map;
    
        int sgl_background_image;
        int chat_bar_image;

#ifdef FBO_ENABLED
        int hdr_fbo;
        int hdr_color_buffers[2];
        int hdr_pingpong_fbos[2];
        int hdr_pingpong_buffers[2];
        int hdr_depth_buffer;
        int hdr_blur_shader;
        int hdr_blur2_shader;
#endif

        int quad_shader;

#ifndef HEADLESS
        GLFWwindow *window;
#endif
    } gfx;

    enum world_state state;

    bool assets_downloading;

    mat4 v;
    mat4 p;
    mat4 vp;
    vec4 viewport;

    struct ui_data* ui;

    struct light_area* render_area;

    struct script_system* script;
    bool enable_script;

    struct primitives primitives;

    int render_count;

    bool client_on;
    bool server_on;
    
    double delta_time;
    double last_time;
    double fps;
    double time;
    int frames_in_second;
    int frames;
    
#ifdef ODE_ENABLED
    struct {
        bool paused;
        dWorldID world;
        dSpaceID space;
        dJointGroupID contactgroup;

        int collisions_in_frame;
    } physics;
#endif

    struct network server;
    struct network client;
    struct network_downloader downloader;

    struct config_file config;
    struct config_file config_private;

    struct sndmgr s_mgr;

    void (*world_frame_user)(struct world* world);
    void (*world_frame_render_user)(struct world* world);
    void (*world_frame_ui_user)(struct world* world);
    void (*world_uniforms_set)(struct world* world);

    bool debug_mode;
    bool profiler_on;
};

#ifndef HEADLESS
struct world* world_init(char** argv, int argc, GLFWwindow* window);
#else
struct world* world_init(char** argv, int argc, void* a);
#endif

void world_deinit(struct world* world);

void world_frame(struct world* world);
void world_frame_render(struct world* world);
void world_frame_light_pass(struct world* world, float quality, int framebuffer, int framebuffer_x, int framebuffer_y);

/* old api
void world_draw(struct world* world, int count, int vertex_array, int shader_program, mat4 model_matrix);
void world_draw_model(struct world* world, struct model* model, int shader_program, mat4 model_matrix, bool set_textures);

enum primitive_type
{
    PRIMITIVE_BOX,
    PRIMITIVE_ARROW,
    PRIMITIVE_PLANE,
};
void world_draw_primitive(struct world* world, int shader, int fill, enum primitive_type type, mat4 model_matrix, vec4 color);
void world_uniforms(struct world* world, int shader_program, mat4 model_matrix);
*/

void world_updres(struct world* world);
void world_start_game(struct world* world); // for clients that are in WORLD_STATE_MAINMENu

#endif