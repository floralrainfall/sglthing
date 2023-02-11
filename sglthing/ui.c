#include "ui.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string.h>
#include "keyboard.h"
#include "sglthing.h"
#include "shader.h"
#include "texture.h"

void ui_draw_text(struct ui_data* ui, float position_x, float position_y, char* text, float depth)
{
    vec2 points[512][2] = {};
    int point_count = 0;

    float size = 8;

    for(int i = 0; i < strlen(text); i++)
    {
        vec2 v_up_left    = {position_x+i*size,position_y+size*2};
        vec2 v_up_right   = {position_x+i*size+size,position_y+size*2};
        vec2 v_down_left  = {position_x+i*size,position_y};
        vec2 v_down_right = {position_x+i*size+size,position_y};

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

        char character = text[i];
        float uv_x = 0.f;
        float uv_y = (character*16.f)/4096.f;

        vec2 uv_up_left    = {0.f, uv_y};
        vec2 uv_up_right   = {1.f, uv_y};
        vec2 uv_down_left  = {0.f, uv_y+(16.f/4096.f)};
        vec2 uv_down_right = {1.f, uv_y+(16.f/4096.f)};

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

    sglc(glUseProgram(ui->ui_program));
    sglc(glUniform1f(glGetUniformLocation(ui->ui_program,"depth"), -depth));
    sglc(glUniform1f(glGetUniformLocation(ui->ui_program,"time"), (float)glfwGetTime()));
    sglc(glUniformMatrix4fv(glGetUniformLocation(ui->ui_program,"projection"), 1, GL_FALSE, ui->projection[0]));    
    sglc(glActiveTexture(GL_TEXTURE0));
    sglc(glBindTexture(GL_TEXTURE_2D, ui->ui_font));
    sglc(glDrawArrays(GL_TRIANGLES, 0, point_count));
}

bool ui_draw_button(struct ui_data* ui, float position_x, float position_y, char* text, float depth)
{
    ui_draw_text(ui, position_x, position_y, text, depth);
}

void ui_init(struct ui_data* ui)
{
    int ui_vertex = compile_shader("../shaders/ui.vs", GL_VERTEX_SHADER);
    int ui_frag = compile_shader("../shaders/ui.fs", GL_FRAGMENT_SHADER);
    ui->ui_program = link_program(ui_vertex, ui_frag);

    load_texture("../resources/font.png");
    ui->ui_font = get_texture("../resources/font.png");

    glGenVertexArrays(1,&ui->ui_vao);    
    glGenBuffers(1,&ui->ui_vbo);
    glBindVertexArray(ui->ui_vao);

    glBindBuffer(GL_ARRAY_BUFFER, ui->ui_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * 512 * 2, NULL, GL_DYNAMIC_DRAW);

    // vec2: position
    sglc(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2) * 2, (void*)0));
    sglc(glEnableVertexAttribArray(0));

    // vec2: uv
    sglc(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec2) * 2, (void*)(sizeof(float)*2)));
    sglc(glEnableVertexAttribArray(1));

    glBindVertexArray(0);

    glm_ortho(0.f, 640.f, 0.f, 480.f, 0.1f, 1000.f, ui->projection);
}

void ui_draw_text_3d(struct ui_data* ui, vec4 viewport, vec3 camera_position, vec3 camera_front, vec3 position, float fov, mat4 mvp, char* text)
{
    vec3 dest_position;
    vec3 direction;
    glm_vec3_sub(position,camera_position,direction);
    float angle = glm_vec3_angle(camera_front, direction) / M_PI_180f;
    if(angle < fov)
    {
        glm_project((vec3){0.0f, 1.f, 0.0f}, mvp, viewport, dest_position);
        ui_draw_text(ui, dest_position[0], dest_position[1], text, dest_position[2]);
    }
}