#include <stdio.h>
#include "world.h"
#include "keyboard.h"
#include "shader.h"
#include "model.h"
#include "sglthing.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct world* world_init()
{
    struct world* world = malloc(sizeof(struct world));

    world->cam.position[0] = 0.f;
    world->cam.position[1] = 0.f;
    world->cam.position[2] = 0.f;
    world->cam.up[0] = 0.f;
    world->cam.up[1] = 1.f;
    world->cam.up[2] = 0.f;
    world->cam.fov = 90.f;

    int v = compile_shader("../shaders/normal.vs", GL_VERTEX_SHADER);
    int f = compile_shader("../shaders/fragment.fs", GL_FRAGMENT_SHADER);
    world->normal_shader = link_program(v,f);
    
    load_model("../resources/test.obj");
    world->test_object = get_model("../resources/test.obj");
    if(!world->test_object)
        printf("sglthing: model load fail\n");
        
    glEnable(GL_DEPTH_TEST);  
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    add_input((struct keyboard_mapping){.key_positive = GLFW_KEY_D, .key_negative = GLFW_KEY_A, .name = "x_axis"});
    add_input((struct keyboard_mapping){.key_positive = GLFW_KEY_Q, .key_negative = GLFW_KEY_E, .name = "y_axis"});
    add_input((struct keyboard_mapping){.key_positive = GLFW_KEY_W, .key_negative = GLFW_KEY_S, .name = "z_axis"});

    world->ui = (struct ui_data*)malloc(sizeof(struct ui_data));
    ui_init(world->ui);
    world->viewport[0] = 0.f;
    world->viewport[1] = 0.f;
    world->viewport[2] = 640.f;
    world->viewport[3] = 480.f;

    return world;
}

void world_frame(struct world* world)
{
    world->render_count = 0;
    world->cam.yaw = fmodf(world->cam.yaw, 360.f);

    glm_perspective(world->cam.fov, 640.f/480.f, 0.1f, 1000.f, world->p);

    world->cam.front[0] = cosf(world->cam.yaw * M_PI_180f) * cosf(world->cam.pitch * M_PI_180f);
    world->cam.front[1] = sinf(world->cam.pitch * M_PI_180f);
    world->cam.front[2] = sinf(world->cam.yaw * M_PI_180f) * cosf(world->cam.pitch * M_PI_180f);

    glm_normalize(world->cam.front);
    glm_cross(world->cam.front, (vec3){0.f, 1.f, 0.f}, world->cam.right);
    glm_normalize(world->cam.right);
    glm_cross(world->cam.right, world->cam.front, world->cam.up);
    glm_normalize(world->cam.up);

    vec3 target;
    glm_vec3_add(world->cam.position, world->cam.front, target);

    glm_lookat(world->cam.position, target, world->cam.up, world->v);

    mat4 test_model;
    glm_mat4_identity(test_model);
    glm_rotate_y(test_model,(float)glfwGetTime()/2.f,test_model);

    glm_mul(world->p, world->v, world->vp);

    world->cam.position[0] += cosf(world->cam.yaw * M_PI_180f + M_PI_2f) * get_input("x_axis") / 60.f;
    world->cam.position[2] += sinf(world->cam.yaw * M_PI_180f + M_PI_2f) * get_input("x_axis") / 60.f;
    world->cam.position[0] += cosf(world->cam.yaw * M_PI_180f) * get_input("z_axis") / 60.f;
    world->cam.position[2] += sinf(world->cam.yaw * M_PI_180f) * get_input("z_axis") / 60.f;

    world->cam.position[1] += get_input("y_axis") / 60.f;

    if(get_focus())
    {
        world->cam.yaw += mouse_position[0] / 20.f;
        world->cam.pitch -= mouse_position[1] / 20.f;
    }
    if(world->cam.pitch > 85.f)
        world->cam.pitch = 85.f;
    if(world->cam.pitch < -85.f)
        world->cam.pitch = -85.f;
    //glm_lookat(world->cam.position, (vec3){0.f,16.f,0.f}, world->cam.up, world->v);

    world_draw_model(world, world->test_object, world->normal_shader, test_model);

    ui_draw_text(world->ui, 0.f, 480.f-16.f, "sglthing dev (c) endoh.ca", 1.f);
}

static void __easy_uniforms(struct world* world, int shader_program, mat4 model_matrix)
{
    sglc(glUniformMatrix4fv(glGetUniformLocation(shader_program,"model"), 1, GL_FALSE, model_matrix[0]));
    sglc(glUniformMatrix4fv(glGetUniformLocation(shader_program,"view"), 1, GL_FALSE, world->v[0]));
    sglc(glUniformMatrix4fv(glGetUniformLocation(shader_program,"projection"), 1, GL_FALSE, world->p[0]));
    sglc(glUniform1f(glGetUniformLocation(shader_program,"time"), (float)glfwGetTime()));
    sglc(glUniform3fv(glGetUniformLocation(shader_program,"camera_position"), 1, world->cam.position));
}

void world_draw(struct world* world, int count, int vertex_array, int shader_program, mat4 model_matrix)
{
    ASSERT(count != 0);
    ASSERT(vertex_array != 0);
    ASSERT(shader_program != 0);
    sglc(glUseProgram(shader_program));
    __easy_uniforms(world, shader_program, model_matrix);
    sglc(glBindVertexArray(vertex_array));
    sglc(glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0));
    world->render_count++;
}

void world_draw_model(struct world* world, struct model* model, int shader_program, mat4 model_matrix)
{
    mat4 mvp;

    ASSERT(model != 0);
    ASSERT(shader_program != 0);
    sglc(glUseProgram(shader_program));

    __easy_uniforms(world, shader_program, model_matrix);
    int last_vertex_array = 0;
    char txbuf[64];
    for(int i = 0; i < model->mesh_count; i++)
    {
        struct mesh* mesh_sel = &model->meshes[i];
        vec3 src_position = {0.f, 1.f, 0.f};
        
        for(int j = 0; j < mesh_sel->diffuse_textures; j++)
        {
            glActiveTexture(GL_TEXTURE0 + j);
            glBindTexture(GL_TEXTURE_2D, mesh_sel->diffuse_texture[i]);
        }
        for(int j = 0; j < mesh_sel->specular_textures; j++)
        {
            glActiveTexture(GL_TEXTURE0 + j + 2);
            glBindTexture(GL_TEXTURE_2D, mesh_sel->specular_texture[i]);
        }
        if(mesh_sel->vertex_array != last_vertex_array)
        {
            last_vertex_array = mesh_sel->vertex_array;
            sglc(glBindVertexArray(mesh_sel->vertex_array));
        }
        //glBindVertexBuffer(0, mesh_sel->vertex_buffer, 0, 3 * sizeof(float));
        sglc(glDrawElements(GL_TRIANGLES, mesh_sel->element_count, GL_UNSIGNED_INT, 0));
        world->render_count++;

        snprintf(txbuf,64,"%s:%i", model->name, i);
        ui_draw_text_3d(world->ui, world->viewport, world->cam.position, world->cam.front, (vec3){0.f,1.f,0.f}, world->cam.fov, world->vp, txbuf);
    }
}