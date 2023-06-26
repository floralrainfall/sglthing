#include "ui.h"
#ifndef HEADLESS
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif
#include <string.h>
#include "keyboard.h"
#include "sglthing.h"
#include "shader.h"
#include "texture.h"
#include "keyboard.h"
#include "io.h"

#define MAX_CHARACTERS_STRING 65535
#define MAX_UI_ELEMENTS 512
#define ADJUST_POSITION(ui) if(ui->current_panel) { position_x += ui->current_panel->position_x; position_y = ui->current_panel->position_y - position_y; }

void ui_draw_text(struct ui_data* ui, float position_x, float position_y, char* text, float depth)
{
    #ifndef HEADLESS
    if(ui->ui_elements > MAX_UI_ELEMENTS || (keys_down[GLFW_KEY_GRAVE_ACCENT] && !ui->persist))
        return;

    profiler_event("ui_draw_text");

    ADJUST_POSITION(ui);

    int txlen = strlen(text);

    vec2 points[MAX_CHARACTERS_STRING][2] = {0};
    int point_count = 0;

    int line = 0;
    int keys = 0;

    for(int i = 0; i < txlen; i++)
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

        int character = text[i] - ui->ui_font->chr_off;

        int uv_x = fmodf(character, ui->ui_font->cw);
        int uv_y = fmodf(ceilf(character / ui->ui_font->cw), ui->ui_font->ch);
        
        float uv_w = ui->ui_font->cx;
        float uv_h = ui->ui_font->cy;

        vec2 uv_up_left    = {(uv_x*uv_w),      (uv_y*uv_h)};
        vec2 uv_up_right   = {(uv_x*uv_w)+uv_w, (uv_y*uv_h)};
        vec2 uv_down_left  = {(uv_x*uv_w),      (uv_y*uv_h)+uv_h};
        vec2 uv_down_right = {(uv_x*uv_w)+uv_w, (uv_y*uv_h)+uv_h};

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
    sglc(glActiveTexture(GL_TEXTURE0));
    sglc(glBindTexture(GL_TEXTURE_2D, ui->ui_font->font_texture));
    vec3 offset;
    sglc(glUniform1f(glGetUniformLocation(ui->ui_program,"time"), (float)glfwGetTime()));
    sglc(glUniform1f(glGetUniformLocation(ui->ui_program,"waviness"), ui->waviness));
    sglc(glUniformMatrix4fv(glGetUniformLocation(ui->ui_program,"projection"), 1, GL_FALSE, ui->projection[0]));      
    sglc(glUniformMatrix4fv(glGetUniformLocation(ui->ui_program,"transform"), 1, GL_FALSE, ui->transform[0])); 

    offset[0] = 0.0f;
    offset[1] = 0.0f;
    offset[2] = 0.0f;
    if(ui->shadow)
    {   
        vec4 background_color = { 0.0f, 0.0f, 0.0f, 0.0f };
        vec4 foreground_color;

        offset[0] = -2.0f;
        offset[1] = -1.0f;
        offset[2] = -0.02f;
        
        glm_vec4_divs(ui->foreground_color, 2.f, foreground_color);
        sglc(glUniform1f(glGetUniformLocation(ui->ui_program,"depth"), -depth));
        sglc(glUniform3fv(glGetUniformLocation(ui->ui_program,"offset"), 1, offset));
        sglc(glUniform4fv(glGetUniformLocation(ui->ui_program,"foreground_color"), 1, foreground_color));
        sglc(glUniform4fv(glGetUniformLocation(ui->ui_program,"background_color"), 1, background_color));
        sglc(glDrawArrays(GL_TRIANGLES, 0, point_count));

        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = -0.01f;

        sglc(glUniform1f(glGetUniformLocation(ui->ui_program,"depth"), -depth));
        sglc(glUniform3fv(glGetUniformLocation(ui->ui_program,"offset"), 1, offset));
        sglc(glUniform4fv(glGetUniformLocation(ui->ui_program,"foreground_color"), 1, ui->foreground_color));
        sglc(glUniform4fv(glGetUniformLocation(ui->ui_program,"background_color"), 1, ui->background_color));
        sglc(glDrawArrays(GL_TRIANGLES, 0, point_count));

        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = 0.0f;

        sglc(glUniform1f(glGetUniformLocation(ui->ui_program,"depth"), -depth));
        sglc(glUniform3fv(glGetUniformLocation(ui->ui_program,"offset"), 1, offset));
        sglc(glUniform4fv(glGetUniformLocation(ui->ui_program,"foreground_color"), 1, ui->background_color));
        sglc(glUniform4fv(glGetUniformLocation(ui->ui_program,"background_color"), 1, ui->background_color));
        sglc(glDrawArrays(GL_TRIANGLES, 0, point_count));
    }
    else
    {    
        sglc(glUniform1f(glGetUniformLocation(ui->ui_program,"depth"), -depth));
        sglc(glUniform4fv(glGetUniformLocation(ui->ui_program,"background_color"), 1, ui->background_color));
        sglc(glUniform4fv(glGetUniformLocation(ui->ui_program,"foreground_color"), 1, ui->foreground_color));
        sglc(glUniform3fv(glGetUniformLocation(ui->ui_program,"offset"), 1, offset));
        sglc(glDrawArrays(GL_TRIANGLES, 0, point_count));    
    }

    ui->ui_elements++;
    ui->background_color[3] = old_background;

    profiler_end();
    #endif
}

bool ui_draw_button(struct ui_data* ui, float position_x, float position_y, float size_x, float size_y, int image, float depth)
{
    #ifndef HEADLESS
    if(ui->ui_elements > MAX_UI_ELEMENTS)
        return false;
    #endif    

    profiler_event("ui_draw_button");
    
    ui_draw_image(ui, position_x, position_y, size_x, size_y, image, depth);

    ADJUST_POSITION(ui);

    if(mouse_state.mouse_button_l && !ui->debounce)
    {
        float rel_m_px = mouse_position[0];
        float rel_m_py = ui->screen_size[1] - mouse_position[1];
        if(rel_m_px > position_x &&
           rel_m_px < position_x+size_x &&
           rel_m_py < position_y &&
           rel_m_py > position_y-size_y)
           {
                ui->debounce = true;
                profiler_end();
                return true;
           }
    }
    if(!mouse_state.mouse_button_l)
        ui->debounce = false;

    profiler_end();

    return false;
}

void ui_end_panel(struct ui_data* ui)
{
    if(ui->current_panel)
    {
        struct ui_panel* old_panel = ui->current_panel;
        glm_vec4_copy(ui->current_panel->oldfg, ui->foreground_color);
        glm_vec4_copy(ui->current_panel->oldbg, ui->background_color);
        ui->current_panel = ui->current_panel->parent_panel;
        free2(old_panel);
    }
}

void ui_draw_panel(struct ui_data* ui, float position_x, float position_y, float size_x, float size_y, float depth)
{
    #ifndef HEADLESS
    if(ui->ui_elements > MAX_UI_ELEMENTS)
        return false;
    #endif

    profiler_event("ui_draw_panel");

    ADJUST_POSITION(ui);

    struct ui_panel* new_panel = (struct ui_panel*)malloc2(sizeof(struct ui_panel));    
    new_panel->parent_panel = ui->current_panel;

    ui->current_panel = new_panel;
    ui->current_panel->position_x = position_x;
    ui->current_panel->position_y = position_y;
    ui->current_panel->size_x = size_x;
    ui->current_panel->size_y = size_y;

    glm_vec4_copy(ui->foreground_color, ui->current_panel->oldfg);
    glm_vec4_copy(ui->background_color, ui->current_panel->oldbg);

    ui->foreground_color[0] = 1.f;
    ui->foreground_color[1] = 1.f;
    ui->foreground_color[2] = 1.f;

    ui->background_color[0] = 0.f;
    ui->background_color[1] = 0.f;
    ui->background_color[2] = 0.f;
    ui->background_color[3] = 0.f;

    vec2 points[6][2] = {0};
    vec2 v_up_left    = {position_x,position_y};
    vec2 v_up_right   = {position_x+size_x,position_y};
    vec2 v_down_left  = {position_x,position_y-size_y};
    vec2 v_down_right = {position_x+size_x,position_y-size_y};

    // tri 1

    points[0][0][0] = v_up_left[0];
    points[0][0][1] = v_up_left[1];

    points[1][0][0] = v_down_left[0];
    points[1][0][1] = v_down_left[1];

    points[2][0][0] = v_up_right[0];
    points[2][0][1] = v_up_right[1];

    // tri 2

    points[3][0][0] = v_down_right[0];
    points[3][0][1] = v_down_right[1];

    points[4][0][0] = v_up_right[0];
    points[4][0][1] = v_up_right[1];

    points[5][0][0] = v_down_left[0];
    points[5][0][1] = v_down_left[1];

    vec2 uv_up_left    = {0.f,0.f};
    vec2 uv_up_right   = {1.f,0.f};
    vec2 uv_down_left  = {0.f,1.f};
    vec2 uv_down_right = {1.f,1.f};

    // tri 1

    points[0][1][0] = uv_up_left[0];
    points[0][1][1] = uv_up_left[1];

    points[1][1][0] = uv_down_left[0];
    points[1][1][1] = uv_down_left[1];

    points[2][1][0] = uv_up_right[0];
    points[2][1][1] = uv_up_right[1];

    // tri 2

    points[3][1][0] = uv_down_right[0];
    points[3][1][1] = uv_down_right[1];

    points[4][1][0] = uv_up_right[0];
    points[4][1][1] = uv_up_right[1];

    points[5][1][0] = uv_down_left[0];
    points[5][1][1] = uv_down_left[1];

    sglc(glBindVertexArray(ui->ui_vao));
    sglc(glBindBuffer(GL_ARRAY_BUFFER, ui->ui_vbo));
    sglc(glBufferSubData(GL_ARRAY_BUFFER, 0, 6*2*2*sizeof(float), points));
    sglc(glBindBuffer(GL_ARRAY_BUFFER, 0));

    sglc(glUseProgram(ui->ui_panel_program));
    vec3 offset = {0.f, 0.f};
    sglc(glUniform1f(glGetUniformLocation(ui->ui_panel_program,"depth"), -depth));
    sglc(glUniform1f(glGetUniformLocation(ui->ui_panel_program,"time"), (float)glfwGetTime()));
    sglc(glUniform1f(glGetUniformLocation(ui->ui_panel_program,"waviness"), ui->waviness));
    sglc(glUniformMatrix4fv(glGetUniformLocation(ui->ui_panel_program,"projection"), 1, GL_FALSE, ui->projection[0])); 
    sglc(glUniformMatrix4fv(glGetUniformLocation(ui->ui_panel_program,"transform"), 1, GL_FALSE, ui->transform[0])); 
    sglc(glUniform3fv(glGetUniformLocation(ui->ui_panel_program,"offset"), 1, offset));
    //sglc(glUniform4fv(glGetUniformLocation(ui->ui_panel_program,"foreground_color"), 1, ui->current_panel->top_color));
    sglc(glUniform4fv(glGetUniformLocation(ui->ui_panel_program,"background_color"), 1, ui->panel_background_color));
    sglc(glDrawArrays(GL_TRIANGLES, 0, 6));

    profiler_end();
}

void ui_draw_image(struct ui_data* ui, float position_x, float position_y, float size_x, float size_y, int image, float depth)
{
    #ifndef HEADLESS
    if(ui->ui_elements > MAX_UI_ELEMENTS)
        return false;
    #endif

    profiler_event("ui_draw_image");
    
    ADJUST_POSITION(ui);
    
    vec2 points[6][2] = {0};
    vec2 v_up_left    = {position_x,position_y};
    vec2 v_up_right   = {position_x+size_x,position_y};
    vec2 v_down_left  = {position_x,position_y-size_y};
    vec2 v_down_right = {position_x+size_x,position_y-size_y};

    // tri 1

    points[0][0][0] = v_up_left[0];
    points[0][0][1] = v_up_left[1];

    points[1][0][0] = v_down_left[0];
    points[1][0][1] = v_down_left[1];

    points[2][0][0] = v_up_right[0];
    points[2][0][1] = v_up_right[1];

    // tri 2

    points[3][0][0] = v_down_right[0];
    points[3][0][1] = v_down_right[1];

    points[4][0][0] = v_up_right[0];
    points[4][0][1] = v_up_right[1];

    points[5][0][0] = v_down_left[0];
    points[5][0][1] = v_down_left[1];

    vec2 uv_up_left    = {0.f,0.f};
    vec2 uv_up_right   = {1.f,0.f};
    vec2 uv_down_left  = {0.f,1.f};
    vec2 uv_down_right = {1.f,1.f};

    // tri 1

    points[0][1][0] = uv_up_left[0];
    points[0][1][1] = uv_up_left[1];

    points[1][1][0] = uv_down_left[0];
    points[1][1][1] = uv_down_left[1];

    points[2][1][0] = uv_up_right[0];
    points[2][1][1] = uv_up_right[1];

    // tri 2

    points[3][1][0] = uv_down_right[0];
    points[3][1][1] = uv_down_right[1];

    points[4][1][0] = uv_up_right[0];
    points[4][1][1] = uv_up_right[1];

    points[5][1][0] = uv_down_left[0];
    points[5][1][1] = uv_down_left[1];

    sglc(glBindVertexArray(ui->ui_vao));
    sglc(glBindBuffer(GL_ARRAY_BUFFER, ui->ui_vbo));
    sglc(glBufferSubData(GL_ARRAY_BUFFER, 0, 6*2*2*sizeof(float), points));
    sglc(glBindBuffer(GL_ARRAY_BUFFER, 0));

    sglc(glUseProgram(ui->ui_img_program));
    sglc(glActiveTexture(GL_TEXTURE0));
    sglc(glBindTexture(GL_TEXTURE_2D, image));
    vec3 offset = {0.f, 0.f};
    sglc(glUniform1f(glGetUniformLocation(ui->ui_img_program,"depth"), -depth));
    sglc(glUniform1f(glGetUniformLocation(ui->ui_img_program,"time"), (float)glfwGetTime()));
    sglc(glUniform1f(glGetUniformLocation(ui->ui_img_program,"waviness"), ui->waviness));
    sglc(glUniformMatrix4fv(glGetUniformLocation(ui->ui_img_program,"projection"), 1, GL_FALSE, ui->projection[0])); 
    sglc(glUniformMatrix4fv(glGetUniformLocation(ui->ui_img_program,"transform"), 1, GL_FALSE, ui->transform[0])); 
    sglc(glUniform3fv(glGetUniformLocation(ui->ui_img_program,"offset"), 1, offset));
    sglc(glUniform4fv(glGetUniformLocation(ui->ui_img_program,"foreground_color"), 1, ui->foreground_color));
    sglc(glUniform4fv(glGetUniformLocation(ui->ui_img_program,"background_color"), 1, ui->background_color));
    sglc(glDrawArrays(GL_TRIANGLES, 0, 6));

    profiler_end();
}

void ui_init(struct ui_data* ui)
{
#ifndef HEADLESS
    glm_mat4_identity(ui->transform);

    int ui_vertex = compile_shader("uiassets/shaders/ui.vs", GL_VERTEX_SHADER);
    int ui_frag = compile_shader("uiassets/shaders/ui.fs", GL_FRAGMENT_SHADER);
    ui->ui_program = link_program(ui_vertex, ui_frag);

    int ui_i_frag = compile_shader("uiassets/shaders/ui_picture.fs", GL_FRAGMENT_SHADER);
    ui->ui_img_program = link_program(ui_vertex, ui_i_frag);

    int ui_p_frag = compile_shader("uiassets/shaders/ui_panel.fs", GL_FRAGMENT_SHADER);
    ui->ui_panel_program = link_program(ui_vertex, ui_p_frag);

    int ui_ttf_vertex = compile_shader("uiassets/shaders/ui_font2.vs", GL_VERTEX_SHADER);
    ui->ui_ttf_program = link_program(ui_ttf_vertex, ui_frag);

    ui->default_font = ui_load_font("uiassets/font.png", 8, 16, 1, 256);

    ui->ui_font = ui->default_font;
    ui->current_panel = 0;

    sglc(glGenVertexArrays(1,&ui->ui_vao));    
    sglc(glGenBuffers(1,&ui->ui_vbo));
    sglc(glBindVertexArray(ui->ui_vao));

    sglc(glBindBuffer(GL_ARRAY_BUFFER, ui->ui_vbo));
    sglc(glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * MAX_CHARACTERS_STRING * 2, NULL, GL_DYNAMIC_DRAW));

    // vec2: position
    sglc(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2) * 2, (void*)0));
    sglc(glEnableVertexAttribArray(0));

    // vec2: uv
    sglc(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec2) * 2, (void*)(sizeof(float)*2)));
    sglc(glEnableVertexAttribArray(1));

    sglc(glBindVertexArray(0));

    ui->background_color[0] = 0.4f;
    ui->background_color[1] = 0.4f;
    ui->background_color[2] = 0.4f;
    ui->background_color[3] = 0.2f;

    ui->foreground_color[0] = 1.0f;
    ui->foreground_color[1] = 1.0f;
    ui->foreground_color[2] = 1.0f;
    ui->foreground_color[3] = 1.0f;
    
    ui->panel_background_color[0] = 0.0f;
    ui->panel_background_color[1] = 0.0f;
    ui->panel_background_color[2] = 0.0f;
    ui->panel_background_color[3] = 0.8f;

    ui->waviness = 0.f;
    ui->silliness = 0.f;
    ui->silliness_speed = 5.f;
    ui->ui_size = 0;
    
    if (FT_Init_FreeType(&ui->ui_freetype))
        printf("sglthing: couldnt init freetype\n");
    else
    {
        glGenVertexArrays(1, &ui->ui_ttf_vao);
        glGenBuffers(1, &ui->ui_ttf_vbo);
        glBindVertexArray(ui->ui_ttf_vao);
        glBindBuffer(GL_ARRAY_BUFFER, ui->ui_ttf_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    ui->shadow = true;
#endif
}

void ui_draw_text_3d(struct ui_data* ui, vec4 viewport, vec3 camera_position, vec3 camera_front, vec3 position, float fov, mat4 m, mat4 vp, char* text)
{
    #ifndef HEADLESS
    if(ui->ui_elements > MAX_UI_ELEMENTS || (keys_down[GLFW_KEY_GRAVE_ACCENT] && !ui->persist))
        return;

    profiler_event("ui_draw_text_3d");
        
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

        int txt_len = strlen(text);
        float txt_off = (float)txt_len;
        txt_off *= ui->ui_font->cy;
        txt_off /= 4.f;

        ui_draw_text(ui, dest_position[0] - txt_off, dest_position[1], text, dist);
    }

    profiler_end();
    #endif
}

struct ui_font* ui_load_font(char* file, float cx, float cy, float cw, float ch)
{
    #ifndef HEADLESS
    struct texture_load_info tex_info = load_texture(file);
    int font_texture = get_texture(file);
    if(font_texture)
    {
        struct ui_font* new_font = malloc2(sizeof(struct ui_font));

        new_font->font_texture = font_texture;
        new_font->cx = cx;
        new_font->cy = cy;
        new_font->cw = cw;
        new_font->ch = ch;
        new_font->tx = tex_info.texture_width;
        new_font->ty = tex_info.texture_height;
        new_font->chr_off = 0;

        return new_font;
    }
    #endif
    return NULL;
}

void ui_font2_render(struct ui_data* ui, struct ui_font2* font)
{
    #ifndef HEADLESS
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for(unsigned char c; c < MAX_FONT_GLYPHS; c++)
    {
        if(FT_Load_Char(font->face, c, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT))
        {
            printf("sglthing: cant load glyph %i\n", c);
            continue;
        }

        struct ui_font2_chara* chara = &font->characters[c];
        
        glGenTextures(1, &chara->texture);
        glBindTexture(GL_TEXTURE_2D, chara->texture);
        glTexImage2D(GL_TEXTURE_2D,
            0, GL_RED,
            font->face->glyph->bitmap.width,
            font->face->glyph->bitmap.rows,
            0, GL_RED,
            GL_UNSIGNED_BYTE,
            font->face->glyph->bitmap.buffer
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glm_vec2_copy((vec2){font->face->glyph->bitmap.width, font->face->glyph->bitmap.rows}, chara->size);
        glm_vec2_copy((vec2){font->face->glyph->bitmap_left, font->face->glyph->bitmap_top}, chara->bearing);
        chara->advance = font->face->glyph->advance.x;
    }
    #endif
}

struct ui_font2* ui_load_font2(struct ui_data* ui, char* file, int font_w, int font_h)
{
    #ifndef HEADLESS
    char path[256];
    if(file_get_path(path, 256, file) != -1)
    {
        FT_Face font_face;
        if(FT_New_Face(ui->ui_freetype, path, 0, &font_face))
        {
            printf("sglthing: failed to load font %s\n",file);
            return NULL;
        }

        struct ui_font2* font = (struct ui_font2*)malloc2(sizeof(struct ui_font2));
        font->face = font_face;
        font->size_x = font_w;
        font->size_y = font_h;
        FT_Set_Pixel_Sizes(font->face, font_w, font_h);  
        
        ui_font2_render(ui, font);

        printf("sglthing: loaded font %s\n", file);

        return font;
    }
    else
        printf("sglthing: couldn't find font file %s\n",file);
    #endif
    return NULL;
}

void ui_font2_text(struct ui_data* ui, float position_x, float position_y, struct ui_font2* font, char* text, float depth)
{
    #ifndef HEADLESS
    profiler_event("ui_font2_text");
    glUseProgram(ui->ui_ttf_program);

    ADJUST_POSITION(ui);

    sglc(glUniform1f(glGetUniformLocation(ui->ui_ttf_program,"time"), (float)glfwGetTime()));
    sglc(glUniform1f(glGetUniformLocation(ui->ui_ttf_program,"waviness"), ui->waviness));
    sglc(glUniformMatrix4fv(glGetUniformLocation(ui->ui_ttf_program,"projection"), 1, GL_FALSE, ui->projection[0]));      
    sglc(glUniformMatrix4fv(glGetUniformLocation(ui->ui_ttf_program,"transform"), 1, GL_FALSE, ui->transform[0]));      
    sglc(glUniform1f(glGetUniformLocation(ui->ui_ttf_program,"depth"), -depth));
    sglc(glUniform4fv(glGetUniformLocation(ui->ui_ttf_program,"background_color"), 1, ui->background_color));
    sglc(glUniform4fv(glGetUniformLocation(ui->ui_ttf_program,"foreground_color"), 1, ui->foreground_color));

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(ui->ui_ttf_vao);

    float old_position_x = position_x;

    for(int i = 0; i < strlen(text); i++)
    {
        char c = text[i];

        if(c == '\n')
        {
            position_x = old_position_x;
            position_y -= font->size_y;
            continue;
        }

        struct ui_font2_chara* chara = &font->characters[c];

        float xpos = position_x + chara->bearing[0];
        float ypos = position_y - (chara->size[1] - chara->bearing[1]);
        float w = chara->size[0];
        float h = chara->size[1];

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };

        glBindTexture(GL_TEXTURE_2D, chara->texture);
        glBindBuffer(GL_ARRAY_BUFFER, ui->ui_ttf_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        position_x += (chara->advance >> 6);
    }
    
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    profiler_end();
    #endif
}

float ui_font2_text_len(struct ui_font2* font, char* text)
{
    float text_len_pix = 0.f;
    for(int i = 0; i < strlen(text); i++)
    {
        char c = text[i];
        struct ui_font2_chara* chara = &font->characters[c];
        text_len_pix += (chara->advance >> 6);
    }
    return text_len_pix;
}