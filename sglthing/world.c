#include <stdio.h>
#include "world.h"
#include "keyboard.h"
#include "shader.h"
#include "model.h"
#include "sglthing.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "io.h"
#ifndef HEADLESS
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif

#ifdef ODE_ENABLED
static void world_physics_callback(void* data, dGeomID o1, dGeomID o2)
{
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);
    struct world* world = (struct world*)data;
    dContact contact;
    contact.surface.mode = dContactBounce | dContactSoftCFM;
    contact.surface.mu = dInfinity;
    contact.surface.mu2 = 0;
    contact.surface.bounce = 0.9;
    contact.surface.bounce_vel = 0.1;
    contact.surface.soft_cfm = 0.000001;
    if (dCollide (o1,o2,1,&contact.geom,sizeof(dContact))) {
        dJointID c = dJointCreateContact (world->physics.world,world->physics.contactgroup,&contact);
        dJointAttach (c,b1,b2);
        world->physics.collisions_in_frame++;
    }
}
#endif

#ifndef HEADLESS
struct world* world_init(char** argv, int argc, GLFWwindow* window)
#else
struct world* world_init(char** argv, int argc, void* p)
#endif
{
    struct world* world = malloc(sizeof(struct world));

#ifndef HEADLESS
    world->gfx.window = window;
#endif

    world->cam.position[0] = 0.f;
    world->cam.position[1] = 0.f;
    world->cam.position[2] = 0.f;
    world->cam.up[0] = 0.f;
    world->cam.up[1] = 1.f;
    world->cam.up[2] = 0.f;
    world->cam.world_up[0] = 0.f;
    world->cam.world_up[1] = 1.f;
    world->cam.world_up[2] = 0.f;
    world->cam.fov = 45.f;
    world->cam.yaw = 0.f;
    world->cam.lsd = 0.f;
    world->cam.lock = true;
    world->gfx.enable_sun = true;

    world->frames = 0;
    world->fps = 0.0;

    config_load(&world->config, "config.ini");
    for(int i = 2; i < argc; i += 2)
        g_key_file_set_value(world->config.key_file, "sglthing", argv[i-1], argv[i]);


    int archives_length;
    char** archives = g_key_file_get_string_list(world->config.key_file, "sglthing", "archives", &archives_length, NULL);
    for(int i = 0; i < archives_length; i++)
        fs_add_directory(archives[i]);

    char* net_mode = config_string_get(&world->config,"network_mode");
    if(net_mode && strcmp(net_mode,"server") == 0)
        g_key_file_set_value(world->config.key_file, "sglthing", "swap_interval", "0");

    bool network_download = false;
    world->assets_downloading = false;
    world->downloader.socket = -1;
    if(strcmp(net_mode,"client")==0 && config_number_get(&world->config,"network_download")==1.0)
    {
        network_start_download(&world->downloader,config_string_get(&world->config,"network_ip"), config_number_get(&world->config,"network_port"), "game/data.tar", config_string_get(&world->config,"server_pass"));
        network_download = true;
        world->assets_downloading = true;
    }

    world->enable_script = (config_number_get(&world->config, "script_enabled") == 1.0);

    load_texture("uiassets/sglthing.png");
    world->gfx.sgl_background_image = get_texture("uiassets/sglthing.png");
    load_texture("uiassets/text_input_bar.png");
    world->gfx.chat_bar_image = get_texture("uiassets/text_input_bar.png");

    if(!network_download)
        world->script = script_init("scripts/game.lua", world);
    world->script->enabled = world->enable_script ? world->script->enabled : false;

    world->gfx.shadow_pass = false;
    int ls_v = compile_shader("shaders/shadow_pass.vs",GL_VERTEX_SHADER);
    int ls_f = compile_shader("shaders/shadow_pass.fs",GL_FRAGMENT_SHADER);
    world->gfx.lighting_shader = link_program(ls_v,ls_f);
#ifdef FBO_ENABLED
    world->gfx.hdr_blur_shader = create_program();
    int b_v = compile_shader("shaders/quad.vs",GL_VERTEX_SHADER);   attach_program_shader(world->gfx.hdr_blur_shader, b_v);
    int b_g = compile_shader("shaders/quad.gs",GL_GEOMETRY_SHADER); attach_program_shader(world->gfx.hdr_blur_shader, b_g);
    int b_f = compile_shader("shaders/blur.fs",GL_FRAGMENT_SHADER); attach_program_shader(world->gfx.hdr_blur_shader, b_f);
    link_programv(world->gfx.hdr_blur_shader);

    world->gfx.hdr_blur2_shader = create_program();
    b_v = compile_shader("shaders/quad.vs",GL_VERTEX_SHADER);   attach_program_shader(world->gfx.hdr_blur2_shader, b_v);
    b_g = compile_shader("shaders/quad.gs",GL_GEOMETRY_SHADER); attach_program_shader(world->gfx.hdr_blur2_shader, b_g);
    b_f = compile_shader("shaders/blur2.fs",GL_FRAGMENT_SHADER); attach_program_shader(world->gfx.hdr_blur2_shader, b_f);
    link_programv(world->gfx.hdr_blur2_shader);
#endif
    world->gfx.quad_shader = create_program();
    int q_v = compile_shader("shaders/quad.vs",GL_VERTEX_SHADER);   attach_program_shader(world->gfx.quad_shader, q_v);
    int q_g = compile_shader("shaders/quad.gs",GL_GEOMETRY_SHADER); attach_program_shader(world->gfx.quad_shader, q_g);
    int q_f = compile_shader("shaders/quad.fs",GL_FRAGMENT_SHADER); attach_program_shader(world->gfx.quad_shader, q_f);
    link_programv(world->gfx.quad_shader);

#ifndef HEADLESS
    add_input((struct keyboard_mapping){.key_positive = GLFW_KEY_D, .key_negative = GLFW_KEY_A, .name = "x_axis"});
    add_input((struct keyboard_mapping){.key_positive = GLFW_KEY_Q, .key_negative = GLFW_KEY_E, .name = "y_axis"});
    add_input((struct keyboard_mapping){.key_positive = GLFW_KEY_W, .key_negative = GLFW_KEY_S, .name = "z_axis"});
#endif

    world->ui = (struct ui_data*)malloc(sizeof(struct ui_data));
    ui_init(world->ui);
    world->viewport[0] = 0.f;
    world->viewport[1] = 0.f;
    world->viewport[2] = 640.f;
    world->viewport[3] = 480.f;
    world->gfx.screen_width = 640;
    world->gfx.screen_width = 480;

    world->delta_time = 0.0;

    world->gfx.clear_color[0] = 250.f/255.f;
    world->gfx.clear_color[1] = 214.f/255.f;
    world->gfx.clear_color[2] = 165.f/255.f; 
    world->gfx.clear_color[3] = 0.f;

    world->gfx.fog_color[0] = world->gfx.clear_color[0];
    world->gfx.fog_color[1] = world->gfx.clear_color[1];
    world->gfx.fog_color[2] = world->gfx.clear_color[2];
    world->gfx.fog_color[3] = world->gfx.clear_color[3];

    world->gfx.fog_maxdist = 100.f;
    world->gfx.fog_mindist = 50.f;
    world->gfx.banding_effect = 0xff8;

    world->gfx.sun_direction[0] = -0.5f;
    world->gfx.sun_direction[1] = 0.5f;
    world->gfx.sun_direction[2] = 0.5f;

    world->gfx.ambient[0] = 30.f/255.f;
    world->gfx.ambient[1] = 26.f/255.f;
    world->gfx.ambient[2] = 117.f/255.f;

    world->gfx.diffuse[0] = 227.f/255.f;
    world->gfx.diffuse[1] = 168.f/255.f;
    world->gfx.diffuse[2] = 87.f/255.f;

    world->gfx.specular[0] = 255.f/255.f;
    world->gfx.specular[1] = 204.f/255.f;
    world->gfx.specular[2] = 51.f/255.f;

    world->primitives = create_primitives();

#ifdef ODE_ENABLED
    world->physics.world = dWorldCreate();
    dWorldSetContactSurfaceLayer(world->physics.world, 0.001);
    dWorldSetAutoDisableFlag(world->physics.world, 1);
    dWorldSetAutoDisableSteps(world->physics.world, 10);
    dWorldSetGravity(world->physics.world, 0, -9.28, 0);
    dWorldSetCFM(world->physics.world, 1e-5);
    dWorldSetERP(world->physics.world, 0.2);

    world->physics.space = dHashSpaceCreate(0);
    world->physics.contactgroup = dJointGroupCreate (0);
#endif

    world->render_area = 0;
    
    vec4 border_color = { 0.0f, 0.0f, 0.0f, 1.0f };

    sglc(glGenFramebuffers(1, &world->gfx.depth_map_fbo));
    sglc(glGenTextures(1, &world->gfx.depth_map_texture));

    sglc(glBindTexture(GL_TEXTURE_2D, world->gfx.depth_map_texture));
    sglc(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
                SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL));
    sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)); 
    sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER)); 
    sglc(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color));   

    sglc(glBindFramebuffer(GL_FRAMEBUFFER, world->gfx.depth_map_fbo));
    sglc(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, world->gfx.depth_map_texture, 0));
    sglc(glDrawBuffer(GL_NONE));
    sglc(glReadBuffer(GL_NONE));
    sglc(glBindFramebuffer(GL_FRAMEBUFFER, 0));  

    sglc(glGenFramebuffers(1, &world->gfx.depth_map_fbo_far));
    sglc(glGenTextures(1, &world->gfx.depth_map_texture_far));

    sglc(glBindTexture(GL_TEXTURE_2D, world->gfx.depth_map_texture_far));
    sglc(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
                SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL));
    sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)); 
    sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER)); 
    sglc(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color));   

    sglc(glBindFramebuffer(GL_FRAMEBUFFER, world->gfx.depth_map_fbo_far));
    sglc(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, world->gfx.depth_map_texture_far, 0));
    sglc(glDrawBuffer(GL_NONE));
    sglc(glReadBuffer(GL_NONE));
    sglc(glBindFramebuffer(GL_FRAMEBUFFER, 0));  

    world_updres(world);

    world->server.status = NETWORKSTATUS_DISCONNECTED;
    world->client.status = NETWORKSTATUS_DISCONNECTED;
    network_init(&world->server, world->script);
    if(!network_download)
        network_init(&world->client, world->script);
    world->server.world = world;
    world->client.world = world;
    world->client.security = (config_number_get(&world->config,"network_security") == 1.0);
    world->server.security = (config_number_get(&world->config,"network_security") == 1.0);
    world->server.shutdown_empty = (config_number_get(&world->config,"shutdown_empty") == 1.0);
    world->server.client_default_tick = config_number_get(&world->config,"server_default_tick");
    world->debug_mode = g_key_file_get_boolean(world->config.key_file,"sglthing","debug_mode", NULL);

    strncpy(world->server.debugger_pass,config_string_get(&world->config,"debugger_pass"),64);
    strncpy(world->server.server_pass,config_string_get(&world->config,"server_pass"),64);
    strncpy(world->client.debugger_pass,config_string_get(&world->config,"debugger_pass"),64);
    strncpy(world->client.server_pass,config_string_get(&world->config,"server_pass"),64);

    strncpy(world->server.server_motd,config_string_get(&world->config,"server_motd"),128);
    strncpy(world->server.server_name,config_string_get(&world->config,"server_name"),64);

#ifndef HEADLESS
    if(g_key_file_get_boolean(world->config.key_file, "sglthing", "fullscreen", NULL))
        glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, 
            g_key_file_get_integer(world->config.key_file, "sglthing", "fullscreen_x", NULL), 
            g_key_file_get_integer(world->config.key_file, "sglthing", "fullscreen_y", NULL), GLFW_DONT_CARE);
#endif

    if(g_key_file_has_key(world->config.key_file,"sglthing","user_color_r",NULL))
        world->client.client.player_color_r = config_number_get(&world->config, "user_color_r");
    else
        world->client.client.player_color_r = g_random_double_range(0.0, 1.0);

    if(g_key_file_has_key(world->config.key_file,"sglthing","user_color_g",NULL))
        world->client.client.player_color_g = config_number_get(&world->config, "user_color_g");
    else
        world->client.client.player_color_g = g_random_double_range(0.0, 1.0);

    if(g_key_file_has_key(world->config.key_file,"sglthing","user_color_b",NULL))
        world->client.client.player_color_b = config_number_get(&world->config, "user_color_b");
    else
        world->client.client.player_color_b = g_random_double_range(0.0, 1.0);

#ifdef HEADLESS
    if(strcmp(net_mode,"server")!=0)
    {
        printf("sglthing: headless mode only supports dedicated server\n");
        exit(-1);
    }
#endif

    if(strcmp(net_mode,"server")==0)
    {
#ifndef HEADLESS
        char v_name[32];
        snprintf(v_name,32,"sglthing r%i DEDICATED SERVER",GIT_COMMIT_COUNT);
        glfwSetWindowTitle(world->gfx.window, v_name);
#endif
        network_open(&world->server, config_string_get(&world->config,"network_ip"), config_number_get(&world->config,"network_port"));
        world->server_on = true;
    } else if(strcmp(net_mode,"client")==0)
    {        
        world->client_on = true;
        if(!network_download)
            network_connect(&world->client, config_string_get(&world->config,"network_ip"), config_number_get(&world->config,"network_port"));
    } else if(strcmp(net_mode,"host")==0)
    {
        network_open(&world->server, config_string_get(&world->config,"network_ip"), config_number_get(&world->config,"network_port"));
        network_connect(&world->client, "127.0.0.1", config_number_get(&world->config,"network_port"));

        world->client_on = true;
        world->server_on = true;
    }

    snd_init(&world->s_mgr);

    world->world_frame_user = NULL;
    world->world_frame_ui_user = NULL;
    world->world_frame_render_user = NULL;
    world->world_uniforms_set = NULL;

    load_texture("uiassets/white.png");
    world->gfx.white_texture = get_texture("uiassets/white.png");

    return world;
}

void world_frame_render(struct world* world)
{    
    #ifndef HEADLESS
    glPushDebugGroupKHR(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Render Scene");
    if(!world->assets_downloading)
        script_frame_render(world->script, world->gfx.shadow_pass);
    if(world->world_frame_render_user)
        world->world_frame_render_user(world);
    //world_draw_model(world, world->test_object, world->normal_shader, test_model, true);
    glPopDebugGroupKHR();  
    #endif 
}

void world_frame_light_pass(struct world* world, float quality, int framebuffer, int framebuffer_x, int framebuffer_y)
{
    #ifndef HEADLESS
    glPushDebugGroupKHR(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Render Light Pass");
    world->gfx.shadow_pass = true;

    sglc(glViewport(0,0,framebuffer_x,framebuffer_y));
    sglc(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer));
    sglc(glClear(GL_DEPTH_BUFFER_BIT));
    sglc(glCullFace(GL_FRONT));
    world_frame_render(world);
    sglc(glCullFace(GL_BACK));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    sglc(glViewport(0,0,world->viewport[2],world->viewport[3]));
    
    world->gfx.shadow_pass = false;
    glPopDebugGroupKHR();   
    #endif
}

void world_frame(struct world* world)
{
    world->render_count = 0;

    sglc(glViewport(0, 0, world->gfx.screen_width, world->gfx.screen_height));
    world->viewport[2] = (float)world->gfx.screen_width;
    world->viewport[3] = (float)world->gfx.screen_height;

    sglc(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    if(world->client.status == NETWORKSTATUS_CONNECTED)
    {
        sglc(glClearColor(world->gfx.clear_color[0], world->gfx.clear_color[1], world->gfx.clear_color[2], world->gfx.clear_color[3]));
    }
    else
    {
        sglc(glClearColor(0.2f,0.2f,0.2f,1.0f));
    }
    sglc(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
    sglc(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));  
    sglc(glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO));
    sglc(glEnable(GL_DEPTH_TEST));
#ifdef FBO_ENABLED
    sglc(glBindFramebuffer(GL_FRAMEBUFFER, world->gfx.hdr_fbo));
    if(world->client.status == NETWORKSTATUS_CONNECTED)
    {
        unsigned int attachments[1] = { GL_COLOR_ATTACHMENT1 };
        sglc(glDrawBuffers(1, attachments));  
        sglc(glClearColor(0.0f,0.0f,0.0f,1.0f));
        sglc(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));

        unsigned int attachments_2[1] = { GL_COLOR_ATTACHMENT0 };
        sglc(glDrawBuffers(1, attachments_2));  
        sglc(glClearColor(world->gfx.clear_color[0], world->gfx.clear_color[1], world->gfx.clear_color[2], world->gfx.clear_color[3]));
        sglc(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
    }
    else
    {
        sglc(glClearColor(0.2f,0.2f,0.2f,1.0f));
        sglc(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
    }

    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    sglc(glDrawBuffers(2, attachments));  

    sglc(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));  
    sglc(glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO));
    sglc(glEnable(GL_DEPTH_TEST));

#else
    sglc(glBindFramebuffer(GL_FRAMEBUFFER, 0));
#endif

    if(world->world_frame_user)
        world->world_frame_user(world);

    if(world->cam.lock)
    {
        if(world->cam.pitch > 85.f)
            world->cam.pitch = 85.f;
        if(world->cam.pitch < -85.f)
            world->cam.pitch = -85.f;
    }

    glm_perspective(world->cam.fov * M_PI_180f, world->gfx.screen_width/world->gfx.screen_height, 0.1f, world->gfx.fog_maxdist, world->p);
    glm_ortho(0.f, world->viewport[2], 0.f, world->viewport[3], 0.1f, 1000.f, world->ui->projection);
    world->ui->screen_size[0] = world->viewport[2];
    world->ui->screen_size[1] = world->viewport[3];

    float yaw = world->cam.yaw + (g_random_double() * world->cam.random_shake) + (sinf(world->time*2.f) * world->cam.wobble);
    float pitch = world->cam.pitch + (g_random_double() * world->cam.random_shake) + (cosf(world->time) * world->cam.wobble); 

    world->cam.front[0] = cosf(yaw * M_PI_180f) * cosf(pitch * M_PI_180f);
    world->cam.front[1] = sinf(pitch * M_PI_180f);
    world->cam.front[2] = sinf(yaw * M_PI_180f) * cosf(pitch * M_PI_180f);

    glm_normalize(world->cam.front);
    glm_cross(world->cam.front, world->cam.world_up, world->cam.right);
    glm_normalize(world->cam.right);
    glm_cross(world->cam.right, world->cam.front, world->cam.up);
    glm_normalize(world->cam.up);

    vec3 target;
    glm_vec3_add(world->cam.position, world->cam.front, target);

    glm_lookat(world->cam.position, target, world->cam.up, world->v);
    //glm_lookat((vec3){world->cam.position[0]-10.f,world->cam.position[1]+10.f,world->cam.position[2]-10.f},(vec3){world->cam.position[0],world->cam.position[1],world->cam.position[2]},(vec3){0.f,1.f,0.f},world->v);

    glm_mul(world->p, world->v, world->vp);
    
    network_frame(&world->server, world->delta_time, world->time);
    if(!world->assets_downloading)
        network_frame(&world->client, world->delta_time, world->time);

    if(world->client.status != NETWORKSTATUS_CONNECTED)
    {
        sglc(glBindFramebuffer(GL_FRAMEBUFFER,2));
    }
    if(!world->assets_downloading)
        script_frame(world->script);
    if(world->client.status != NETWORKSTATUS_CONNECTED)
    {
        sglc(glBindFramebuffer(GL_FRAMEBUFFER,0));
    }
    //if(get_focus())
    //{
    //    world->cam.yaw += mouse_position[0] * world->delta_time * 10.f;
    //    world->cam.pitch -= mouse_position[1] * world->delta_time * 10.f;
    //}

    //glm_lookat(world->cam.position, (vec3){0.f,16.f,0.f}, world->cam.up, world->v);

    // render pass
    /*glDisable(GL_DEPTH_TEST);
    world_draw_model(world, world->gfx.sky_ball, world->sky_shader, sky_matrix, false);
    world_draw_primitive(world, world->cloud_shader, GL_FILL, PRIMITIVE_PLANE, cloud_matrix, (vec4){1.f,1.f,1.f,1.f});*/

    if(world->gfx.enable_sun)
    {
        vec3 sun_direction_camera;
        mat4 light_projection, light_projection_far;
        mat4 light_view, light_view_far;

        glm_ortho(-25.f, 25.f, -25.f, 25.f, 1.0f, 100.0f,light_projection);
        glm_ortho(-75.f, 75.f, -75.f, 75.f, 1.0f, 200.0f,light_projection_far);

        sun_direction_camera[0] = world->gfx.sun_direction[0]*50.f + world->gfx.sun_position[0];
        sun_direction_camera[1] = world->gfx.sun_direction[1]*50.f + world->gfx.sun_position[1];
        sun_direction_camera[2] = world->gfx.sun_direction[2]*50.f + world->gfx.sun_position[2];
        glm_lookat(sun_direction_camera, world->gfx.sun_position,(vec3){0.f,1.f,0.f},light_view);

        sun_direction_camera[0] = world->gfx.sun_direction[0]*100.f + world->gfx.sun_position[0];
        sun_direction_camera[1] = world->gfx.sun_direction[1]*100.f + world->gfx.sun_position[1];
        sun_direction_camera[2] = world->gfx.sun_direction[2]*100.f + world->gfx.sun_position[2];
        glm_lookat(sun_direction_camera, world->gfx.sun_position,(vec3){0.f,1.f,0.f},light_view_far);

        glm_mat4_mul(light_projection, light_view, world->gfx.light_space_matrix);
        glm_mat4_mul(light_projection_far, light_view_far, world->gfx.light_space_matrix_far);
    }

    if(world->client.status == NETWORKSTATUS_CONNECTED && !world->assets_downloading)
    {
        if(world->gfx.enable_sun)
        {
            world->gfx.current_map = 0;
            world_frame_light_pass(world,15.f,world->gfx.depth_map_fbo,SHADOW_WIDTH,SHADOW_HEIGHT);
            world->gfx.current_map = 1;
            world_frame_light_pass(world,50.f,world->gfx.depth_map_fbo_far,SHADOW_WIDTH,SHADOW_HEIGHT);
        }

    #ifdef FBO_ENABLED
        sglc(glBindFramebuffer(GL_FRAMEBUFFER, world->gfx.hdr_fbo));
        sglc(glEnable(GL_DEPTH_TEST));  
        sglc(glEnable(GL_CULL_FACE));  
        sglc(glEnable(GL_BLEND));   
        sglc(glClear(GL_DEPTH_BUFFER_BIT));
    #else
        sglc(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    #endif
        world_frame_render(world);

    #ifdef FBO_ENABLED
        bool bloom_blur_horiz = true, first_bloom_iter = true;
        int amount = 25;
        sglc(glUseProgram(world->gfx.hdr_blur_shader));
        for(int i = 0; i < amount; i++)
        {
            sglc(glBindFramebuffer(GL_FRAMEBUFFER, world->gfx.hdr_pingpong_fbos[bloom_blur_horiz]));
            sglc(glUniform1i(glGetUniformLocation(world->gfx.hdr_blur_shader,"horizontal"), bloom_blur_horiz)); 
            //sglc(glUniform2f(glGetUniformLocation(world->gfx.hdr_blur_shader,"size"), 1.0f, 1.0f)); 
            //sglc(glUniform2f(glGetUniformLocation(world->gfx.hdr_blur_shader,"offset"), 0.0f, 0.0f)); 
            sglc(glActiveTexture(GL_TEXTURE0));
            sglc(glBindTexture(GL_TEXTURE_2D, first_bloom_iter ? world->gfx.hdr_color_buffers[1] : world->gfx.hdr_pingpong_buffers[!bloom_blur_horiz]));
            sglc(glUniform1i(glGetUniformLocation(world->gfx.hdr_blur_shader,"image"), 0));
            sglc(glBindBuffer(GL_ARRAY_BUFFER, 0));            
            sglc(glBindVertexArray(1));
            sglc(glDrawArrays(GL_POINTS, 0, 1));
            bloom_blur_horiz = !bloom_blur_horiz;
            if(first_bloom_iter)
                first_bloom_iter = false;
        }

        sglc(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        sglc(glUseProgram(world->gfx.hdr_blur2_shader));        
        
        sglc(glActiveTexture(GL_TEXTURE0));
        sglc(glBindTexture(GL_TEXTURE_2D, world->gfx.hdr_color_buffers[0]));
        sglc(glUniform1i(glGetUniformLocation(world->gfx.hdr_blur2_shader,"scene"), 0));

        sglc(glActiveTexture(GL_TEXTURE1));
        sglc(glBindTexture(GL_TEXTURE_2D, world->gfx.hdr_pingpong_buffers[1]));
        sglc(glUniform1i(glGetUniformLocation(world->gfx.hdr_blur2_shader,"bloom"), 1));

        sglc(glActiveTexture(GL_TEXTURE2));
        sglc(glBindTexture(GL_TEXTURE_2D, world->gfx.hdr_depth_buffer));
        sglc(glUniform1i(glGetUniformLocation(world->gfx.hdr_blur2_shader,"depth"), 2));

        sglc(glUniform1f(glGetUniformLocation(world->gfx.hdr_blur2_shader,"exposure"), 1.0f));
        sglc(glUniform1f(glGetUniformLocation(world->gfx.hdr_blur2_shader,"time"), glfwGetTime()));
        sglc(glUniform1f(glGetUniformLocation(world->gfx.hdr_blur2_shader,"lsd"), world->cam.lsd));
        sglc(glUniform4fv(glGetUniformLocation(world->gfx.hdr_blur2_shader,"viewport"), 1, world->viewport));
        sglc(glBindBuffer(GL_ARRAY_BUFFER, 0));
        sglc(glDrawArrays(GL_POINTS, 0, 1));
    #endif
        // world_draw_model(world, world->test_object, world->normal_shader, test_model2, true);

        sglc(glClear(GL_DEPTH_BUFFER_BIT));

        if(input_disable)
        {
            ui_draw_image(world->ui, 0.f, world->gfx.screen_height, world->gfx.screen_width, 16.f, world->gfx.chat_bar_image, 0.9f);
            vec4 oldbg, oldfg;
            glm_vec4_copy(world->ui->background_color, oldbg);
            glm_vec4_copy(world->ui->foreground_color, oldfg);
            world->ui->background_color[0] = 0.f;
            world->ui->background_color[1] = 0.f;
            world->ui->background_color[2] = 0.f;
            world->ui->background_color[3] = 0.f;

            world->ui->foreground_color[0] = 0.f;
            world->ui->foreground_color[1] = 0.f;
            world->ui->foreground_color[2] = 0.f;
            world->ui->foreground_color[3] = 1.f;
            world->ui->shadow = false;

            ui_draw_text(world->ui, 0.f, world->gfx.screen_height-(16.f), input_text, 0.8f);

            world->ui->shadow = true;
            glm_vec4_copy(oldbg, world->ui->background_color);
            glm_vec4_copy(oldfg, world->ui->foreground_color);
        }

        if(config_number_get(&world->config,"debug_mode") == 1.0)
        {
            char dbg_info[256];
            int old_elements = world->ui->ui_elements;
            snprintf(dbg_info, 256, "DEBUG\n\ncam: V=(%.2f,%.2f,%.2f)\nY=%.2f,P=%.2f\nU=(%.2f,%.2f,%.2f)\nF=(%.2f,%.2f,%.2f)\nR=(%.f,%.f,%.f)\nF=%.f\nv=(%i,%i)\nr (scene)=%i,r (ui)=%i\nt=%f, F=%i, d=%f, fps=%f\n\n"
#ifdef ODE_ENABLED
                "physics pause=%s\nphysics geoms=%i\ncollisions in frame=%i\n"
#endif
                ,
                world->cam.position[0], world->cam.position[1], world->cam.position[2],
                world->cam.yaw, world->cam.pitch,
                world->cam.up[0], world->cam.up[1], world->cam.up[2],
                world->cam.front[0], world->cam.front[1], world->cam.front[2],
                world->cam.right[0], world->cam.right[1], world->cam.right[2],
                world->cam.fov,
                (int)world->viewport[2], (int)world->viewport[3],
                world->render_count,
                old_elements,
                world->time, world->frames, world->delta_time, world->fps
#ifdef ODE_ENABLED
                , world->physics.paused?"true":"false",
                dSpaceGetNumGeoms(world->physics.space),
                world->physics.collisions_in_frame
#endif
            );
            ui_draw_text(world->ui, 0.f, world->gfx.screen_height-(16.f*3), dbg_info, 1.f);

            for(int i = 0; i < archives_loaded; i++)
            {
                snprintf(dbg_info, 256, "FS Archive %i: %s", i, archives[i].directory);
                ui_draw_text(world->ui, world->gfx.screen_width/2, world->gfx.screen_height-(16.f*3)-(i*16.f), dbg_info, 1.f);
            }
            
            network_dbg_ui(&world->client, world->ui);
            network_dbg_ui(&world->server, world->ui);
        }

        if(!world->assets_downloading)
            script_frame_ui(world->script);
        if(world->world_frame_ui_user)
            world->world_frame_ui_user(world);
    }
    else
    {
        sglc(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        world->gfx.current_map = 0;
        world->ui->ui_elements = 0;
        world->gfx.shadow_pass = false;
        ui_draw_image(world->ui, world->gfx.screen_width/2 - 256.f/2, world->gfx.screen_height/2 + 64.f/2, 256.f, 64.f, world->gfx.sgl_background_image, 1.f);
        if(world->assets_downloading)
        {
            char tx[256];
            if(world->downloader.data_size)
            {
                snprintf(tx,256,"%0.2f Mbytes received, %f%% done, packet %i", world->downloader.data_downloaded/1000.f/1000.f, ((float)world->downloader.data_downloaded / world->downloader.data_size) * 100.0, world->downloader.data_packet_id);
                ui_draw_text(world->ui, 0.f, 16.f, tx, 1.f);
            }
            snprintf(tx,256,"Downloading server content from '%s' on %s:%i.", world->downloader.server_name, config_string_get(&world->config, "network_ip"), (int)config_number_get(&world->config, "network_port"));
            ui_draw_text(world->ui, 0.f, 0.f, tx, 1.f);
            snprintf(tx,256,"Server MOTD: '%s'", world->downloader.server_motd);
            ui_draw_text(world->ui, 0.f, 32.f, tx, 1.f);            
            snprintf(tx,256,"SGLAPI MOTD: '%s'", world->downloader.http_client.motd);
            ui_draw_text(world->ui, 0.f, 48.f, tx, 1.f);

            network_tick_download(&world->downloader);

            world->assets_downloading = !world->downloader.data_done;   

            if(world->downloader.data_done)
            {
                world->script = script_init("scripts/game.scm", world);
                network_init(&world->client, world->script);
                network_connect(&world->client, config_string_get(&world->config,"network_ip"), config_number_get(&world->config,"network_port"));
            }
        }
        else
        {
            if(world->client.status == NETWORKSTATUS_DISCONNECTED)
            {        
                ui_draw_text(world->ui, 0.f, 16.f, world->client.disconnect_reason, 1.f);
                ui_draw_text(world->ui, 0.f, 0.f, "Disconnected (world->client.mode == NETWORKSTATUS_DISCONNECTED)", 1.f);
            }
            if(world->server.status == NETWORKSTATUS_DISCONNECTED)
            {        
                ui_draw_text(world->ui, 0.f, 48.f, world->server.disconnect_reason, 1.f);
                ui_draw_text(world->ui, 0.f, 32.f, "Disconnected (world->server.mode == NETWORKSTATUS_DISCONNECTED)", 1.f);
            }
        }
        if(world->server.status == NETWORKSTATUS_CONNECTED)
            network_dbg_ui(&world->server, world->ui);
#ifndef HEADLESS
        set_focus(world->gfx.window, false);
#endif
    }

    world->ui->persist = true;
    char sglthing_v_name[32];
    snprintf(sglthing_v_name,32,"sglthing r%i (%.2f fps)", GIT_COMMIT_COUNT, world->fps);
    ui_draw_text(world->ui, 0.f, world->gfx.screen_height-16.f, sglthing_v_name, 15.f);
    world->ui->persist = false;

    if(world->client.status == NETWORKSTATUS_DISCONNECTED && world->server.status == NETWORKSTATUS_DISCONNECTED && (config_number_get(&world->config, "shutdown_empty") == 1.0))
    {
        printf("sglthing: server empty/offline or client disconnected\n");
#ifndef HEADLESS
        glfwSetWindowShouldClose(world->gfx.window, true);
#endif
    }

#ifndef HEADLESS
    if(keys_down[GLFW_KEY_GRAVE_ACCENT])
    {
        int pixel_buf_size = 3*world->gfx.screen_width*world->gfx.screen_height;
        char* pixels = (char*)malloc(pixel_buf_size);
        sglc(glBindFramebuffer(GL_READ_FRAMEBUFFER, world->gfx.hdr_fbo));
        sglc(glReadBuffer(GL_COLOR_ATTACHMENT0));
        sglc(glReadPixels(0, 0, world->gfx.screen_width, world->gfx.screen_height, GL_RGB, GL_UNSIGNED_BYTE, pixels));
        stbi_flip_vertically_on_write(true);
        printf("sglthing: screenshot taken (%ix%i)\n", world->gfx.screen_width, world->gfx.screen_height);
        stbi_write_png("screenshot.png", world->gfx.screen_width, world->gfx.screen_height, 3, pixels, 0);
        free(pixels);
    }
#endif

    if(!world->frames)
        printf("sglthing: frame 1 done\n");
    world->frames++;
    world->ui->ui_elements = 0;
}

void world_uniforms(struct world* world, int shader_program, mat4 model_matrix)
{
#ifndef HEADLESS
    // camera
    glUniformMatrix4fv(glGetUniformLocation(shader_program,"model"), 1, GL_FALSE, model_matrix[0]);
    glUniformMatrix4fv(glGetUniformLocation(shader_program,"view"), 1, GL_FALSE, world->v[0]);
    glUniformMatrix4fv(glGetUniformLocation(shader_program,"projection"), 1, GL_FALSE, world->p[0]);
    glUniformMatrix4fv(glGetUniformLocation(shader_program,"lsm"), 1, GL_FALSE, world->gfx.light_space_matrix[0]);
    glUniformMatrix4fv(glGetUniformLocation(shader_program,"lsm_far"), 1, GL_FALSE, world->gfx.light_space_matrix_far[0]);
    glUniform3fv(glGetUniformLocation(shader_program,"camera_position"), 1, world->cam.position);

    // fog
    glUniform1f(glGetUniformLocation(shader_program,"fog_maxdist"), world->gfx.fog_maxdist);
    glUniform1f(glGetUniformLocation(shader_program,"fog_mindist"), world->gfx.fog_mindist);
    glUniform4fv(glGetUniformLocation(shader_program,"fog_color"), 1, world->gfx.fog_color);

    // misc
    glUniform4fv(glGetUniformLocation(shader_program,"viewport"), 1, world->viewport);
    glUniform3fv(glGetUniformLocation(shader_program,"sun_direction"), 1, world->gfx.sun_direction);
    glUniform3fv(glGetUniformLocation(shader_program,"sun_position"), 1, world->gfx.sun_position);
    glUniform3fv(glGetUniformLocation(shader_program,"diffuse"), 1, world->gfx.diffuse);
    glUniform3fv(glGetUniformLocation(shader_program,"ambient"), 1, world->gfx.ambient);
    glUniform3fv(glGetUniformLocation(shader_program,"specular"), 1, world->gfx.specular);
    glUniform1f(glGetUniformLocation(shader_program,"time"), (float)world->time);
    glUniform1i(glGetUniformLocation(shader_program,"banding_effect"), world->gfx.banding_effect);
    glUniform1i(glGetUniformLocation(shader_program,"sel_map"), world->gfx.current_map);

    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, world->gfx.depth_map_texture);
    glUniform1i(glGetUniformLocation(shader_program,"depth_map"), 7);

    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, world->gfx.depth_map_texture_far);
    glUniform1i(glGetUniformLocation(shader_program,"depth_map_far"), 8);

    if(world->world_uniforms_set)
        world->world_uniforms_set(world);

    while(glGetError()!=0);
#endif
}

void world_draw(struct world* world, int count, int vertex_array, int shader_program, mat4 model_matrix)
{
    ASSERT(count != 0);
    ASSERT(vertex_array != 0);
    if(world->gfx.shadow_pass)
        shader_program = world->gfx.lighting_shader;
    ASSERT(shader_program != 0);
    sglc(glUseProgram(shader_program));
    world_uniforms(world, shader_program, model_matrix);
    sglc(glBindVertexArray(vertex_array));
    sglc(glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0));
    world->render_count++;
}

void world_draw_model(struct world* world, struct model* model, int shader_program, mat4 model_matrix, bool set_textures)
{
#ifndef HEADLESS
    mat4 mvp;

    ASSERT(model != 0);
    if(world->gfx.shadow_pass)
        shader_program = world->gfx.lighting_shader;
    ASSERT(shader_program != 0);
    int last_vertex_array = 0;
    char txbuf[64];
    for(int i = 0; i < model->mesh_count; i++)
    {
        sglc(glUseProgram(shader_program));

        world_uniforms(world, shader_program, model_matrix);
        if(world->render_area && !world->gfx.shadow_pass)
            light_area_set_uniforms(world->render_area, shader_program);
        struct mesh* mesh_sel = &model->meshes[i];
        vec3 src_position = {0.f, 1.f, 0.f};
        
        if(set_textures)
        {
            char tx_name[64];
            for(int j = 0; j < mesh_sel->diffuse_textures; j++)
            {
                glActiveTexture(GL_TEXTURE0 + j);
                glBindTexture(GL_TEXTURE_2D, mesh_sel->diffuse_texture[i]);                
                snprintf(tx_name,64,"diffuse%i", j);
                glUniform1i(glGetUniformLocation(shader_program,tx_name), j);
            }
            for(int j = 0; j < mesh_sel->specular_textures; j++)
            {
                glActiveTexture(GL_TEXTURE0 + j + 2);
                glBindTexture(GL_TEXTURE_2D, mesh_sel->specular_texture[i]);
                snprintf(tx_name,64,"specular%i", j);
                glUniform1i(glGetUniformLocation(shader_program,tx_name), j+2);
            }
        }
        model_bind_vbos(mesh_sel);
        sglc(glBindVertexArray(mesh_sel->vertex_array));
        //sglc(glBindBuffer(GL_ARRAY_BUFFER, mesh_sel->vertex_buffer));
        //sglc(glVertexArrayVertexBuffer(mesh_sel->vertex_array, 0, mesh_sel->vertex_buffer, 0, sizeof(struct model_vertex)));
        sglc(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_sel->element_buffer));
        sglc(glDrawElements(GL_TRIANGLES, mesh_sel->element_count, GL_UNSIGNED_INT, 0));
        world->render_count++;

        //snprintf(txbuf,64,"%s:%i\n(%.2f,%.2f,%.2f)\n%p", model->name, i, model_matrix[3][0],model_matrix[3][1],model_matrix[3][2],model);
        //ui_draw_text_3d(world->ui, world->viewport, world->cam.position, world->cam.front, (vec3){0.f,1.f,0.f}, world->cam.fov, model_matrix, world->vp, txbuf);
        //world_draw_primitive(world, PRIMITIVE_BOX, model_matrix, (vec4){1.f, 1.f, 1.f, 1.f});
    }
#endif
}

void world_draw_primitive(struct world* world, int shader, int fill, enum primitive_type type, mat4 model_matrix, vec4 color)
{
#ifndef HEADLESS
    sglc(glUseProgram(shader));
    world_uniforms(world, shader, model_matrix);
    glUniform4fv(glGetUniformLocation(shader,"color"), 1, color);
    glGetError();
    sglc(glBindVertexArray(world->primitives.debug_arrays));
    switch(type)
    {
        case PRIMITIVE_BOX:
            sglc(glPolygonMode(GL_FRONT_AND_BACK,fill));
            sglc(glBindBuffer(GL_ARRAY_BUFFER, world->primitives.box_primitive));
            sglc(glDrawArrays(GL_TRIANGLES, 0, world->primitives.box_vertex_count));
            sglc(glPolygonMode(GL_FRONT_AND_BACK,GL_FILL));
            break;
        case PRIMITIVE_ARROW:
            sglc(glBindBuffer(GL_ARRAY_BUFFER, world->primitives.arrow_primitive));
            sglc(glDrawArrays(GL_LINES, 0, world->primitives.arrow_vertex_count));
            break;
        case PRIMITIVE_PLANE:
            sglc(glPolygonMode(GL_FRONT_AND_BACK,fill));
            sglc(glBindBuffer(GL_ARRAY_BUFFER, world->primitives.plane_primitive));
            sglc(glDrawArrays(GL_TRIANGLES, 0, world->primitives.plane_vertex_count));
            sglc(glPolygonMode(GL_FRONT_AND_BACK,GL_FILL));
            break;
    }
#endif
}

// TODO: this
void world_deinit(struct world* world)
{
    char url[256];
    if(world->server.status == NETWORKSTATUS_CONNECTED)
    {
        network_close(&world->server);
        snprintf(url,256,"auth/cancel?sessionkey=%s", world->server.http_client.sessionkey);
        if(world->client.http_client.login)
            http_get(&world->server.http_client,url);
    }
    if(world->client.status == NETWORKSTATUS_CONNECTED)
    {
        network_close(&world->client);
        snprintf(url,256,"auth/cancel?sessionkey=%s", world->client.http_client.sessionkey);
        if(world->client.http_client.login)
            http_get(&world->client.http_client,url);
    }
    network_stop_download(&world->downloader);
}

void world_updres(struct world* world)
{
#ifndef HEADLESS
#ifdef FBO_ENABLED
    glDeleteTextures(1, &world->gfx.hdr_depth_buffer);
    glDeleteFramebuffers(1, &world->gfx.hdr_fbo);
    glDeleteTextures(2, world->gfx.hdr_color_buffers);
    glDeleteFramebuffers(2, world->gfx.hdr_pingpong_fbos);
    glDeleteTextures(2, world->gfx.hdr_pingpong_buffers);
    glGetError(); // consume error since it doesnt matter

    sglc(glGenTextures(1, &world->gfx.hdr_depth_buffer));
    sglc(glBindTexture(GL_TEXTURE_2D, world->gfx.hdr_depth_buffer));
    sglc(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    sglc(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    sglc(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    sglc(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    sglc(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, world->viewport[2], world->viewport[3], 0, GL_DEPTH_COMPONENT, GL_BYTE, NULL));

    sglc(glGenFramebuffers(1, &world->gfx.hdr_fbo));
    sglc(glBindFramebuffer(GL_FRAMEBUFFER, world->gfx.hdr_fbo));
    sglc(glGenTextures(2, world->gfx.hdr_color_buffers));
    for (unsigned int i = 0; i < 2; i++)
    {
        sglc(glBindTexture(GL_TEXTURE_2D, world->gfx.hdr_color_buffers[i]));
        sglc(glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA, world->viewport[2], world->viewport[3], 0, GL_RGBA, GL_FLOAT, NULL
        ));
        sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        sglc(glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, world->gfx.hdr_color_buffers[i], 0
        ));
        sglc(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, world->gfx.hdr_depth_buffer, 0));
    }  
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    sglc(glDrawBuffers(2, attachments));  
    sglc(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    
    sglc(glGenFramebuffers(2, world->gfx.hdr_pingpong_fbos));
    sglc(glGenTextures(2, world->gfx.hdr_pingpong_buffers));
    for (unsigned int i = 0; i < 2; i++)
    {
        sglc(glBindFramebuffer(GL_FRAMEBUFFER, world->gfx.hdr_pingpong_fbos[i]));
        sglc(glBindTexture(GL_TEXTURE_2D, world->gfx.hdr_pingpong_buffers[i]));
        sglc(glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA, world->viewport[2], world->viewport[3], 0, GL_RGBA, GL_FLOAT, NULL
        ));
        sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        sglc(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        sglc(glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, world->gfx.hdr_pingpong_buffers[i], 0
        ));
    }

    sglc(glBindFramebuffer(GL_FRAMEBUFFER, 0));
#endif
#endif
}