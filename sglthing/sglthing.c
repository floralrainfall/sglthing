#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

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

    init_kbd(window);
    set_focus(window, true);

    world = world_init();

    printf("sglthing: ok\n");

    // main render loop
    int screen_width, screen_height;
    glfwGetFramebufferSize(window, &screen_width, &screen_height);
    while(!glfwWindowShouldClose(window))
    {
        glfwMakeContextCurrent(window);
        glViewport(0, 0, screen_width, screen_height);
        glClearColor(0.1f, 0.39f, 0.88f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        world_frame(world);

        glfwSwapBuffers(window); 
        kbd_frame_end();    
        glfwPollEvents();   
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    printf("sglthing: bye\n");
}