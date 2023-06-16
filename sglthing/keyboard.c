#include "keyboard.h"
#include <string.h>
#include <stdbool.h>

#define MAX_KEYBORAD_MAPPINGS 256
struct keyboard_mapping mappings[MAX_KEYBORAD_MAPPINGS];
int mapping_count = 0;

struct mouse_state mouse_state = {0};
int keys_down[GLFW_KEY_LAST] = {0};
vec2 mouse_position = { 0, 0 };
bool mouse_focus = false;
char input_text[MAX_INPUT_TEXT] = {0};
bool input_disable = false;
bool input_lock_tab = false;
int input_cursor = 0;

#ifndef HEADLESS

static void __scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    mouse_state.scroll_x = xoffset;
    mouse_state.scroll_y = yoffset;
}

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

    if(input_disable)
    {
        if(action == GLFW_PRESS && key == GLFW_KEY_ENTER)
            input_disable = false;
        else if((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_BACKSPACE)
        {
            if(input_cursor == 0)
                return;
            input_cursor--;
            input_text[input_cursor] = 0;
        }
        return;
    }

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
    {
        if(key == GLFW_KEY_TAB && action == GLFW_PRESS && !input_lock_tab)
            set_focus(window, !mouse_focus);
        keys_down[key] = action;
    }
}

static void __click_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(button == GLFW_MOUSE_BUTTON_RIGHT)
        mouse_state.mouse_button_r = action == GLFW_PRESS;
    else if(button == GLFW_MOUSE_BUTTON_MIDDLE)
        mouse_state.mouse_button_m = action == GLFW_PRESS;
    else if(button == GLFW_MOUSE_BUTTON_LEFT)
        mouse_state.mouse_button_l = action == GLFW_PRESS;
}

static void __chara_callback(GLFWwindow* window, unsigned int codepoint)
{
    if(!input_disable)
        return;
    char chara = (char)codepoint;
    input_text[input_cursor] = chara;
    input_cursor++;
}

void init_kbd(GLFWwindow* window)
{
    if(!window)
        return;
    glfwSetKeyCallback(window, __kbd_callback);
    glfwSetMouseButtonCallback(window, __click_callback);
    glfwSetCursorPosCallback(window, __mouse_callback);
    glfwSetCharCallback(window, __chara_callback);
    glfwSetScrollCallback(window, __scroll_callback);
    input_disable = false;
}
#endif


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

#ifndef HEADLESS
void set_focus(GLFWwindow* window, bool state)
{
    if(!window)
        return;
    if(state == true)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if(state == false)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    mouse_focus = state;   
}
#endif

bool get_focus()
{
    return mouse_focus;
}

void kbd_frame_end()
{
    mouse_position[0] = 0.f;
    mouse_position[1] = 0.f;
    mouse_state.scroll_x = 0.f;
    mouse_state.scroll_y = 0.f;
}

void start_text_input()
{
    input_disable = true;
    input_cursor = 0;
    memset(input_text, 0, sizeof(input_text));
}