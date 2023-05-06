#ifndef UI_H
#define UI_H
#include <stdbool.h>
#include <cglm/cglm.h>

#define UI_VBO_LIMIT 512

struct ui_font
{
    int font_texture;
    int cx, cy;
    int cw, ch;
    int tx, ty;
    int chr_off;
};

struct ui_panel
{
    vec4 top_color;
    vec4 bottom_color; 
    struct ui_panel* parent_panel;
    float position_x;
    float position_y;
    float size_x;
    float size_y;
    vec4 oldbg;
    vec4 oldfg;
};

struct ui_data
{
    int ui_program;
    int ui_img_program;
    int ui_panel_program;
    struct ui_font* ui_font;

    int ui_vao;
    int ui_vbo;

    mat4 projection;
    vec2 screen_size;

    int ui_elements;

    int ui_cursor;
    int ui_size;

    vec4 background_color;
    vec4 foreground_color;
    float waviness;
    float silliness;
    float silliness_speed;

    struct ui_font* default_font;

    struct ui_panel* current_panel;

    float time;

    bool shadow;
    bool persist;
    bool debounce;
};

// depth should be 1.f
void ui_draw_text(struct ui_data* ui, float position_x, float position_y, char* text, float depth);
bool ui_draw_button(struct ui_data* ui, float position_x, float position_y, float size_x, float size_y, int image, float depth);
void ui_draw_image(struct ui_data* ui, float position_x, float position_y, float size_x, float size_y, int image, float depth);
void ui_draw_text_3d(struct ui_data* ui, vec4 viewport, vec3 camera_position, vec3 camera_front, vec3 position, float fov, mat4 m, mat4 vp, char* text);
void ui_draw_panel(struct ui_data* ui, float position_x, float position_y, float size_x, float size_y, float depth);
void ui_end_panel(struct ui_data* ui);
void ui_init(struct ui_data* ui);
struct ui_font* ui_load_font(char* file, float cx, float cy, float cw, float ch);

#define SCREEN_TOP 644.f

#endif