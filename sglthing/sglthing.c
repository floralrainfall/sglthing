#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <ode/ode.h>
#include <signal.h>

#include "keyboard.h"
#include "world.h"
#include "io.h"
#include "config.h"

static struct world* world;

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
    // init GLFW & GLAD
    if(!glfwInit())
        return -1;
    glfwSetErrorCallback(glerror);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_SAMPLES, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(640,480,"sglthing",NULL,NULL);
    if(!window)
        return -2;    

    signal(SIGINT, sighandler);
    signal(SIGHUP, sighandler);
    signal(SIGTERM, sighandler);
    
    printf("sglthing: window created\n");

    fs_add_directory("resources");
    fs_add_directory("../resources");

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    //glEnable(GL_MULTISAMPLE);  
    glEnable(GL_BLEND);

    dInitODE2(0);
    dAllocateODEDataForThread(dAllocateMaskAll);
    init_kbd(window);
    set_focus(window, false);
    
    world = world_init(argv, argc);
    world->gfx.window = window;

    printf("sglthing: ok\n");

    // main render loop
    world->delta_time = 0.f;
    world->last_time = glfwGetTime();
    glfwSwapInterval(config_number_get(&world->config, "swap_interval"));
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
        glfwMakeContextCurrent(window);
        
        world_frame(world);

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