#ifndef KEYBOARD_H
#define KEYBOARD_H
#ifndef HEADLESS
#include <GLFW/glfw3.h>
#endif
#include <cglm/cglm.h>
#include <stdbool.h>

struct keyboard_mapping
{
    int key_positive;
    int key_negative;

    bool key_positive_down;
    bool key_negative_down;

    float value;
    char name[16];
};

struct mouse_state
{
    bool mouse_button_r;
    bool mouse_button_m;
    bool mouse_button_l;
    double scroll_x;
    double scroll_y;
};

#ifdef HEADLESS
#define GLFW_KEY_LAST 1
#endif

#define MAX_INPUT_TEXT 512
extern struct mouse_state mouse_state;
extern vec2 mouse_position;
extern int keys_down[GLFW_KEY_LAST];
extern char input_text[MAX_INPUT_TEXT];
extern bool input_disable;
extern bool input_lock_tab;

#ifndef HEADLESS
void init_kbd(GLFWwindow* window);
#endif
void add_input(struct keyboard_mapping mapping);
float get_input(char* name);
void kbd_frame_end();
#ifndef HEADLESS
void set_focus(GLFWwindow* window, bool state);
#endif
bool get_focus();
void start_text_input();

#endif
