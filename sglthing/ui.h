#ifndef UI_H
#define UI_H
#include <stdbool.h>
#include <cglm/cglm.h>

#define UI_VBO_LIMIT 512

struct ui_data
{
    int ui_program;
    int ui_font;

    int ui_vao;
    int ui_vbo;

    mat4 projection;

    int ui_elements;

    int ui_cursor;

    vec4 background_color;
    float waviness;
};

// depth should be 1.f
void ui_draw_text(struct ui_data* ui, float position_x, float position_y, char* text, float depth);
bool ui_draw_button(struct ui_data* ui, float position_x, float position_y, char* text, float depth);
void ui_draw_image(struct ui_data* ui, float position_x, float position_y, int image, float depth);
void ui_draw_text_3d(struct ui_data* ui, vec4 viewport, vec3 camera_position, vec3 camera_front, vec3 position, float fov, mat4 m, mat4 vp, char* text);
void ui_init(struct ui_data* ui);

#define SCREEN_TOP 644.f

#endif