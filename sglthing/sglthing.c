#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#ifdef ODE_ENABLED
#include <ode/ode.h>
#endif
#include <signal.h>

#include "keyboard.h"
#include "world.h"
#include "io.h"
#include "config.h"
#include "sglthing.h"

#ifndef SGLTHING_COMPILE
#error "sglthing/sglthing.c should be compiled with SGLTHING_COMPILE"
#endif

static struct world* world;
extern void sglthing_init_api(struct world* world);

static void glerror(int error, const char* description)
{
    fprintf(stderr, "GLFW error %i: %s\n", error, description);
}

static void sighandler(int sig)
{
    printf("sglthing: received %s\n", strsignal(sig));
    switch(sig)
    {
        case SIGTERM:
        case SIGINT:
            glfwSetWindowShouldClose(world->gfx.window, true);
            break;
        case SIGHUP: // no term
            break;
        default:
            break;
    }
}

int main(int argc, char** argv)
{
    char v_name[32];
    snprintf(v_name, 32, "sglthing r%i", GIT_COMMIT_COUNT);
    printf("%s\n",v_name);
    // init GLFW & GLAD
    if(!glfwInit())
        return -1;
    glfwSetErrorCallback(glerror);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_SAMPLES, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(640,480,v_name,NULL,NULL);
    if(!window)
        return -2;    

    signal(SIGINT, sighandler);
    signal(SIGHUP, sighandler);
    signal(SIGTERM, sighandler);
    
    printf("sglthing: window created\n");

    fs_add_directory("resources");
    fs_add_directory("../resources");
    fs_add_directory("~/.sglthing");
    #ifdef __unix__
    fs_add_directory("/opt/sglthing");
    #endif

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    //glEnable(GL_MULTISAMPLE);  
    glEnable(GL_BLEND);

#ifdef ODE_ENABLED
    dInitODE2(0);
    dAllocateODEDataForThread(dAllocateMaskAll);
#endif
    init_kbd(window);
    set_focus(window, false);
    
    world = world_init(argv, argc, window);

    printf("sglthing: ok\n");

    // main render loop
    world->delta_time = 0.f;
    world->last_time = glfwGetTime();
    glfwSwapInterval(config_number_get(&world->config, "swap_interval"));

    sglthing_init_api(world);

    vec4 old_viewport;
    glfwGetFramebufferSize(window, &world->gfx.screen_width, &world->gfx.screen_height);
    glm_vec4_copy(world->viewport, old_viewport);
    while(!glfwWindowShouldClose(window))
    {
        double frame_start = glfwGetTime();
        world->frames_in_second++;
        if(frame_start - world->last_time >= 1.0) {
            world->fps = world->frames_in_second;
            world->frames_in_second = 0;
            world->last_time += 1.0;
        }        

        glfwGetFramebufferSize(window, &world->gfx.screen_width, &world->gfx.screen_height);
        
        world_frame(world);

        if(!glm_vec4_eqv(old_viewport, world->viewport))
        {
            printf("sglthing: updres\n");
            world_updres(world);
            glm_vec4_copy(world->viewport, old_viewport);
        }

        glfwSwapBuffers(window); 
        float frame_end = glfwGetTime();
        world->delta_time = (frame_end - frame_start);
        kbd_frame_end();    
        glfwPollEvents();   
    }

    world_deinit(world);
    glfwDestroyWindow(window);
    glfwTerminate();
    printf("sglthing: bye\n");
}

void __sglthing_assert_failed()
{    
    
}