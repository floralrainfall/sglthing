#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <ode/ode.h>

#include "keyboard.h"
#include "world.h"

static void glerror(int error, const char* description)
{
    fprintf(stderr, "GLFW error %i: %s\n", error, description);
}

int main(int argc, char** argv)
{
    struct world* world;

    // init GLFW & GLAD
    if(!glfwInit())
        return -1;
    glfwSetErrorCallback(glerror);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(640,480,"sglthing",NULL,NULL);
    if(!window)
        return -2;    
    printf("sglthing: window created\n");
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    dInitODE2(0);
    dAllocateODEDataForThread(dAllocateMaskAll);
    init_kbd(window);
    set_focus(window, false);

    world = world_init();

    printf("sglthing: ok\n");

    // main render loop
    int screen_width, screen_height;
    glfwGetFramebufferSize(window, &screen_width, &screen_height);
    world->delta_time = 0.f;
    while(!glfwWindowShouldClose(window))
    {
        double frame_start = glfwGetTime();
        glfwMakeContextCurrent(window);
        glViewport(0, 0, screen_width, screen_height);
        glClearColor(world->gfx.clear_color[0], world->gfx.clear_color[1], world->gfx.clear_color[2], world->gfx.clear_color[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        world_frame(world);

        glfwSwapBuffers(window); 
        float frame_end = glfwGetTime();
        world->delta_time = (frame_end - frame_start);
        kbd_frame_end();    
        glfwPollEvents();   
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    printf("sglthing: bye\n");
}