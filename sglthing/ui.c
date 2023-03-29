#include "ui.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string.h>
#include "keyboard.h"
#include "sglthing.h"
#include "shader.h"
#include "texture.h"
#include "keyboard.h"

#define MAX_CHARACTERS_STRING 32767
#define MAX_UI_ELEMENTS 512

void ui_draw_text(struct ui_data* ui, float position_x, float position_y, char* text, float depth)
{
    if(ui->ui_elements > MAX_UI_ELEMENTS || keys_down[GLFW_KEY_GRAVE_ACCENT])
        return;

    vec2 points[MAX_CHARACTERS_STRING][2] = {0};
    int point_count = 0;

    int line = 0;
    int keys = 0;

    for(int i = 0; i < strlen(text); i++)
    {
        if(text[i] == '\n')
        {
            keys = 0;
            line++;
            continue;
        }

        float x_add = sinf(keys+(ui->silliness_speed*glfwGetTime())+position_x)*ui->silliness;
        float y_add = cosf(keys+(ui->silliness_speed*glfwGetTime())+position_y+(line*(ui->ui_font->cy)))*ui->silliness;
        vec2 v_up_left    = {position_x+keys*ui->ui_font->cx+x_add,position_y+ui->ui_font->cy-line*(ui->ui_font->cy)+y_add};
        vec2 v_up_right   = {position_x+keys*ui->ui_font->cx+ui->ui_font->cx+x_add,position_y+ui->ui_font->cy-line*(ui->ui_font->cy)+y_add};
        vec2 v_down_left  = {position_x+keys*ui->ui_font->cx+x_add,position_y-line*(ui->ui_font->cy)+y_add};
        vec2 v_down_right = {position_x+keys*ui->ui_font->cx+ui->ui_font->cx+x_add,position_y-line*(ui->ui_font->cy)+y_add};
        keys++;

        // tri 1

        points[point_count][0][0] = v_up_left[0];
        points[point_count][0][1] = v_up_left[1];

        points[point_count+1][0][0] = v_down_left[0];
        points[point_count+1][0][1] = v_down_left[1];

        points[point_count+2][0][0] = v_up_right[0];
        points[point_count+2][0][1] = v_up_right[1];

        // tri 2

        points[point_count+3][0][0] = v_down_right[0];
        points[point_count+3][0][1] = v_down_right[1];

        points[point_count+4][0][0] = v_up_right[0];
        points[point_count+4][0][1] = v_up_right[1];

        points[point_count+5][0][0] = v_down_left[0];
        points[point_count+5][0][1] = v_down_left[1];

        float character = (float)text[i];

        float uv_x = fmodf(character - 1, ui->ui_font->cw);
        float uv_y = ((character - 1) / ui->ui_font->ch);
        
        float uv_w = ui->ui_font->cx;
        float uv_h = ui->ui_font->cy;

        vec2 uv_up_left    = {uv_x,      uv_y};
        vec2 uv_up_right   = {uv_x+uv_w, uv_y};
        vec2 uv_down_left  = {uv_x,      uv_y+uv_h};
        vec2 uv_down_right = {uv_x+uv_w, uv_y+uv_h};

        uv_up_left[0] /= ui->ui_font->tx;
        uv_up_right[0] /= ui->ui_font->tx;
        uv_down_left[0] /= ui->ui_font->tx;
        uv_down_right[0] /= ui->ui_font->tx;

        uv_up_left[1] /= ui->ui_font->ty;
        uv_up_right[1] /= ui->ui_font->ty;
        uv_down_left[1] /= ui->ui_font->ty;
        uv_down_right[1] /= ui->ui_font->ty;

        // tri 1

        points[point_count][1][0] = uv_up_left[0];
        points[point_count][1][1] = uv_up_left[1];

        points[point_count+1][1][0] = uv_down_left[0];
        points[point_count+1][1][1] = uv_down_left[1];

        points[point_count+2][1][0] = uv_up_right[0];
        points[point_count+2][1][1] = uv_up_right[1];

        // tri 2

        points[point_count+3][1][0] = uv_down_right[0];
        points[point_count+3][1][1] = uv_down_right[1];

        points[point_count+4][1][0] = uv_up_right[0];
        points[point_count+4][1][1] = uv_up_right[1];

        points[point_count+5][1][0] = uv_down_left[0];
        points[point_count+5][1][1] = uv_down_left[1];

        point_count += 6;
    }
    
    sglc(glBindVertexArray(ui->ui_vao));
    sglc(glBindBuffer(GL_ARRAY_BUFFER, ui->ui_vbo));
    sglc(glBufferSubData(GL_ARRAY_BUFFER, 0, point_count*2*2*sizeof(float), points));
    sglc(glBindBuffer(GL_ARRAY_BUFFER, 0));

    float old_background = ui->background_color[3];
    if(ui->silliness != 0.0)
        ui->background_color[3] = 0.0f;
    sglc(glUseProgram(ui->ui_program));
    sglc(glUniform4fv(glGetUniformLocation(ui->ui_program,"background_color"), 1, ui->background_color));
    sglc(glUniform4fv(glGetUniformLocation(ui->ui_program,"foreground_color"), 1, ui->foreground_color));
    sglc(glUniform1f(glGetUniformLocation(ui->ui_program,"depth"), -depth));
    sglc(glUniform1f(glGetUniformLocation(ui->ui_program,"time"), (float)glfwGetTime()));
    sglc(glUniform1f(glGetUniformLocation(ui->ui_program,"waviness"), ui->waviness));
    sglc(glUniformMatrix4fv(glGetUniformLocation(ui->ui_program,"projection"), 1, GL_FALSE, ui->projection[0]));    
    sglc(glActiveTexture(GL_TEXTURE0));
    sglc(glBindTexture(GL_TEXTURE_2D, ui->ui_font->font_texture));
    sglc(glDrawArrays(GL_TRIANGLES, 0, point_count));
    ui->ui_elements++;
    ui->background_color[3] = old_background;
}

bool ui_draw_button(struct ui_data* ui, float position_x, float position_y, char* text, float depth)
{
    if(ui->ui_elements > MAX_UI_ELEMENTS || keys_down[GLFW_KEY_GRAVE_ACCENT])
        return;

    ui_draw_text(ui, position_x, position_y, text, depth);
}

void ui_init(struct ui_data* ui)
{
    int ui_vertex = compile_shader("uiassets/shaders/ui.vs", GL_VERTEX_SHADER);
    int ui_frag = compile_shader("uiassets/shaders/ui.fs", GL_FRAGMENT_SHADER);
    ui->ui_program = link_program(ui_vertex, ui_frag);

    ui->default_font = ui_load_font("uiassets/font.png", 8, 16, 1, 256);

    ui->ui_font = ui->default_font;

    glGenVertexArrays(1,&ui->ui_vao);    
    glGenBuffers(1,&ui->ui_vbo);
    glBindVertexArray(ui->ui_vao);

    glBindBuffer(GL_ARRAY_BUFFER, ui->ui_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * MAX_CHARACTERS_STRING * 2, NULL, GL_DYNAMIC_DRAW);

    // vec2: position
    sglc(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2) * 2, (void*)0));
    sglc(glEnableVertexAttribArray(0));

    // vec2: uv
    sglc(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec2) * 2, (void*)(sizeof(float)*2)));
    sglc(glEnableVertexAttribArray(1));

    glBindVertexArray(0);

    ui->background_color[0] = 0.5f;
    ui->background_color[1] = 0.5f;
    ui->background_color[2] = 0.5f;
    ui->background_color[3] = 0.2f;

    ui->foreground_color[0] = 1.0f;
    ui->foreground_color[1] = 1.0f;
    ui->foreground_color[2] = 1.0f;
    ui->foreground_color[3] = 1.0f;
    ui->waviness = 0.f;
    ui->silliness = 0.f;
    ui->silliness_speed = 5.f;
    ui->ui_size = 0;
}

void ui_draw_text_3d(struct ui_data* ui, vec4 viewport, vec3 camera_position, vec3 camera_front, vec3 position, float fov, mat4 m, mat4 vp, char* text)
{
    if(ui->ui_elements > MAX_UI_ELEMENTS || keys_down[GLFW_KEY_GRAVE_ACCENT])
        return;
        
    vec3 dest_position;
    vec3 direction;
    vec3 mm_position;
    glm_mat4_mulv3(m,position,1.f,mm_position);
    glm_vec3_sub(camera_position,mm_position,direction);
    float angle = glm_vec3_dot(camera_front, direction) / M_PI_180f;
    if(angle < fov)
    {
        float dist = glm_vec3_distance(camera_position, position);
        glm_project(mm_position, vp, viewport, dest_position);
        ui_draw_text(ui, dest_position[0], dest_position[1], text, dist);
    }
}

struct ui_font* ui_load_font(char* file, float cx, float cy, float cw, float ch)
{
    struct texture_load_info tex_info = load_texture(file);
    int font_texture = get_texture(file);
    if(font_texture)
    {
        struct ui_font* new_font = malloc(sizeof(struct ui_font));

        new_font->font_texture = font_texture;
        new_font->cx = cx;
        new_font->cy = cy;
        new_font->cw = cw;
        new_font->ch = ch;
        new_font->tx = tex_info.texture_width;
        new_font->ty = tex_info.texture_height;

        return new_font;
    }
    return NULL;
}