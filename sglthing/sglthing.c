#ifndef HEADLESS
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif
#include <stdio.h>
#ifdef ODE_ENABLED
#include <ode/ode.h>
#endif
#include <signal.h>
#include <time.h>

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

bool should_close;

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
#ifndef HEADLESS
            glfwSetWindowShouldClose(world->gfx.window, true);
#endif
            should_close = true;
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
    #ifndef HEADLESS
    GLFWwindow* window = NULL;
    #else
    void* window = NULL;
    #endif
    #ifndef HEADLESS
    // init GLFW & GLAD
    if(!glfwInit())
        return -1;
    glfwSetErrorCallback(glerror);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_SAMPLES, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(640,480,v_name,NULL,NULL);
    if(!window)
        return -2;    
    #endif

    signal(SIGINT, sighandler);
    signal(SIGHUP, sighandler);
    signal(SIGTERM, sighandler);
    
    #ifndef HEADLESS
    printf("sglthing: window created\n");
    #else
    printf("sglthing: headless\n");
    #endif

    fs_add_directory("resources");
    fs_add_directory("../resources");
    fs_add_directory("~/.sglthing");
    #ifdef __unix__
    fs_add_directory("/opt/sglthing");
    #endif

#ifndef HEADLESS
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    //glEnable(GL_MULTISAMPLE);  
    glEnable(GL_BLEND);
#endif

#ifdef HEADLESS
    // custom timer
    GTimer* headless_timer = g_timer_new();
#endif

#ifdef ODE_ENABLED
    dInitODE2(0);
    dAllocateODEDataForThread(dAllocateMaskAll);
#endif
#ifndef HEADLESS
    init_kbd(window);
    set_focus(window, false);
#endif
    
    should_close = false;
    world = world_init(argv, argc, window);

    printf("sglthing: ok\n");

    // main render loop
    world->delta_time = 0.f;
#ifndef HEADLESS
    world->last_time = glfwGetTime();
    glfwSwapInterval(config_number_get(&world->config, "swap_interval"));
#else
    world->last_time = g_timer_elapsed(headless_timer, NULL);
#endif

    sglthing_init_api(world);

#ifndef HEADLESS
    vec4 old_viewport;
    glfwGetFramebufferSize(window, &world->gfx.screen_width, &world->gfx.screen_height);
    glm_vec4_copy(world->viewport, old_viewport);
    while(!glfwWindowShouldClose(window) && !should_close)
#else
    while(!should_close)
#endif
    {
        double frame_start = g_timer_elapsed(headless_timer, NULL);
        world->time = frame_start;
        world->frames_in_second++;
        if(frame_start - world->last_time >= 1.0) {
            world->fps = world->frames_in_second;
            world->frames_in_second = 0;
            world->last_time += 1.0;
        }        

#ifndef HEADLESS
        glfwGetFramebufferSize(window, &world->gfx.screen_width, &world->gfx.screen_height);
#endif
        
        world_frame(world);

#ifndef HEADLESS
        if(!glm_vec4_eqv(old_viewport, world->viewport))
        {
            printf("sglthing: updres\n");
            world_updres(world);
            glm_vec4_copy(world->viewport, old_viewport);
        }

        glfwSwapBuffers(window); 
#endif 
        float frame_end = g_timer_elapsed(headless_timer, NULL);
        world->delta_time = (frame_end - frame_start);
#ifndef HEADLESS
        if(get_focus())
            kbd_frame_end();    
        glfwPollEvents();   
#endif
    }

    world_deinit(world);
#ifndef HEADLESS
    glfwDestroyWindow(window);
    glfwTerminate();
#endif
    printf("sglthing: bye\n");
}

void __sglthing_assert_failed()
{    
    
}