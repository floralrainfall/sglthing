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
#include "stb_image.h"
#include "memory.h"
#include "prof.h"

#ifdef _WIN32
  #ifdef _WIN32_WINNT
    #define _WIN32_WINNT 0x0501
  #endif
  #include <winsock2.h>
  #include <Ws2tcpip.h>
#endif

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

#ifdef ENABLE_SIGHANDLER
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
            profiler_dump();
            break;
        case SIGHUP: // no term
            break;
        case SIGSEGV: // segmentation fault
            printf("sglthing: segmentation fault\n");
            __sglthing_assert_failed();
            world_deinit(world);
            exit(-1);
            break;
        default:
            break;
    }
}
#endif

int main(int argc, char** argv)
{
    char v_name[32];
    snprintf(v_name, 32, "sglthing-" GIT_BRANCH " r%i", GIT_COMMIT_COUNT);
    printf("%s\n",v_name);
    #ifndef HEADLESS
    GLFWwindow* window = NULL;

#ifdef _WIN32
    WSADATA wsa_data; // start up winsock
    WSAStartup(MAKEWORD(1,1), &wsa_data);
#endif

#ifndef YES_WSL
    FILE* wsl_test = fopen("/proc/sys/fs/binfmt_misc/WSLInterop", "r");
    if(wsl_test)
    {
        printf("sglthing: cannot run in a mangled environment\n"); // teehee
        exit(-1);
    }
#endif

    #else
    void* window = NULL;
    #endif
    #ifndef HEADLESS

    // init GLFW & GLAD
    if(!glfwInit())
        return -1;
    glfwSetErrorCallback(glerror);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(640,480,v_name,NULL,NULL);
    if(!window)
        return -2;    
    #endif

#ifdef ENABLE_SIGHANDLER
    signal(SIGINT, sighandler);
    signal(SIGHUP, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGSEGV, sighandler);
#endif    

    #ifndef HEADLESS
    printf("sglthing: window created\n");
    #else
    printf("sglthing: headless\n");
    #endif

    fs_add_directory(".");
    fs_add_directory("resources");
    fs_add_directory("../resources");
    #ifdef __unix__
    fs_add_directory("/opt/sglthing");
    #endif

#ifndef HEADLESS
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
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
    
    profiler_global_init();

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
    GLFWimage images[2];
    char icon_path[255];
    int f = file_get_path(icon_path, 255, "icon.png"); 
    if(f != -1)
    {
        images[1].pixels = stbi_load(icon_path, &images[1].width, &images[1].height, 0, 4);
        f = file_get_path(icon_path, 255, "icon_large.png"); 
        if(f != -1)
        {
            images[0].pixels = stbi_load(icon_path, &images[0].width, &images[0].height, 0, 4);
            glfwSetWindowIcon(window, 2, images); 
        }
        else
        {
            printf("sglthing: couldn't find icon_large.png\n");
        }
        stbi_image_free(images[0].pixels);
    }
    else
    {
        printf("sglthing: couldn't find icon.png\n");
    }

    vec4 old_viewport;
    glfwGetFramebufferSize(window, &world->gfx.screen_width, &world->gfx.screen_height);
    if(world->gfx.screen_width > 0 || world->gfx.screen_height > 0)
         { world->gfx.screen_width = 640; world->gfx.screen_height = 480; }
    glm_vec4_copy(world->viewport, old_viewport);
    while(!glfwWindowShouldClose(window) && !should_close)
#else
    float next_dedi_announcement = world->time + 1.f;
    world->gfx.screen_width = 640;
    world->gfx.screen_height = 480;
    while(!should_close)
#endif
    {
#ifdef HEADLESS
        double frame_start = g_timer_elapsed(headless_timer, NULL);
#else 
        double frame_start = glfwGetTime();
#endif
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

#ifdef HEADLESS
        float frame_end = g_timer_elapsed(headless_timer, NULL);

#else 
        float frame_end = glfwGetTime();
#endif
        world->delta_time = (frame_end - frame_start);
#ifdef HEADLESS
        if(world->time > next_dedi_announcement)
        {
            printf("sglthing: %fs up, %f frames/sec (%i), %i in universe, dT: %f\n", world->time, world->fps, world->frames, world->server.server_clients->len, world->delta_time);
            next_dedi_announcement = world->time + 2.5f;
        }
#endif

#ifndef HEADLESS
        if(get_focus())
            kbd_frame_end();    
        glfwPollEvents();   
#endif
    }

    world_deinit(world);

#ifdef _WIN32
    WSACleanup();
#endif

#ifndef HEADLESS
    glfwDestroyWindow(window);
    glfwTerminate();
#endif
    printf("sglthing: bye\n");
}

void __sglthing_assert_failed()
{    
    
}
