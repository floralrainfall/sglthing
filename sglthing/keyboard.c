#include "keyboard.h"
#include <string.h>
#include <stdbool.h>

#define MAX_KEYBORAD_MAPPINGS 256
struct keyboard_mapping mappings[MAX_KEYBORAD_MAPPINGS];
int mapping_count = 0;

int keys_down[GLFW_KEY_LAST] = {0};
vec2 mouse_position = { 0, 0 };
bool mouse_focus = false;

static void __mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    mouse_position[0] = xpos;
    mouse_position[1] = ypos;
    if(mouse_focus)
        glfwSetCursorPos(window, 0, 0);  
}

static void __kbd_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    for(int i = 0; i < mapping_count; i++)
    {
        if(action == GLFW_PRESS)
        {
            if(mappings[i].key_positive == key)
                mappings[i].value = 1.f;
            if(mappings[i].key_negative == key)
                mappings[i].value = -1.f;
        }
        else if(action == GLFW_RELEASE)
        {
            if(mappings[i].key_positive == key && mappings[i].value != -1.f)
                mappings[i].value = 0.f;
            if(mappings[i].key_negative == key && mappings[i].value != 1.f)
                mappings[i].value = -0.f;
        }
    }

    if(action == GLFW_PRESS || action == GLFW_RELEASE)
        keys_down[key] = action;
}

void init_kbd(GLFWwindow* window)
{
    glfwSetKeyCallback(window, __kbd_callback);
    //glfwSetMouseButtonCallback(window, __mouset_callback);
    glfwSetCursorPosCallback(window, __mouse_callback);
}


void add_input(struct keyboard_mapping mapping)
{
    mappings[mapping_count] = mapping;
    mappings[mapping_count].value = 0.f;
    mapping_count++;
}

float get_input(char* name)
{
    for(int i = 0; i < mapping_count; i++)
    {
        if(strncmp(mappings[i].name, name, 16)==0)
        {
            return mappings[i].value;
        }
    }
    return 0.f;
}

void set_focus(GLFWwindow* window, bool state)
{
    if(state == true)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if(state == false)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    mouse_focus = state;   
}

bool get_focus()
{
    return mouse_focus;
}

void kbd_frame_end()
{
    mouse_position[0] = 0.f;
    mouse_position[1] = 0.f;
}