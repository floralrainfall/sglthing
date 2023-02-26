#include <stdio.h>
#include "world.h"
#include "keyboard.h"
#include "shader.h"
#include "model.h"
#include "sglthing.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "io.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

static void world_physics_callback(void* data, dGeomID o1, dGeomID o2)
{
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);
    struct world* world = (struct world*)data;
    dContact contact;
    contact.surface.mode = dContactBounce | dContactSoftCFM;
    contact.surface.mu = dInfinity;
    contact.surface.mu2 = 0;
    contact.surface.bounce = 0.9;
    contact.surface.bounce_vel = 0.1;
    contact.surface.soft_cfm = 0.000001;
    if (dCollide (o1,o2,1,&contact.geom,sizeof(dContact))) {
        dJointID c = dJointCreateContact (world->physics.world,world->physics.contactgroup,&contact);
        dJointAttach (c,b1,b2);
        world->physics.collisions_in_frame++;
    }
}

struct world* world_init()
{
    struct world* world = malloc(sizeof(struct world));

    world->cam.position[0] = 0.f;
    world->cam.position[1] = 0.f;
    world->cam.position[2] = 0.f;
    world->cam.up[0] = 0.f;
    world->cam.up[1] = 1.f;
    world->cam.up[2] = 0.f;
    world->cam.fov = 60.f;
    world->cam.yaw = 0.f;

    world->script = script_init("scripts/game.scm");
        
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

    world->delta_time = 0.0;

    world->gfx.clear_color[0] = 250.f/255.f;
    world->gfx.clear_color[1] = 214.f/255.f;
    world->gfx.clear_color[2] = 165.f/255.f; 
    world->gfx.clear_color[3] = 1.f;

    world->gfx.fog_color[0] = world->gfx.clear_color[0];
    world->gfx.fog_color[1] = world->gfx.clear_color[1];
    world->gfx.fog_color[2] = world->gfx.clear_color[2];
    world->gfx.fog_color[3] = world->gfx.clear_color[3];

    world->gfx.fog_maxdist = 128.f;
    world->gfx.fog_mindist = 128.f-32.f;
    world->gfx.banding_effect = 0xff8;

    world->primitives = create_primitives();

    world->physics.world = dWorldCreate();
    dWorldSetContactSurfaceLayer(world->physics.world, 0.001);
    dWorldSetAutoDisableFlag(world->physics.world, 1);
    dWorldSetAutoDisableSteps(world->physics.world, 10);
    dWorldSetGravity(world->physics.world, 0, -9.28, 0);
    dWorldSetCFM(world->physics.world, 1e-5);
    dWorldSetERP(world->physics.world, 0.2);

    world->physics.space = dHashSpaceCreate(0);
    world->physics.contactgroup = dJointGroupCreate (0);

    world->render_area = 0;

    return world;
}

void world_frame_render(struct world* world)
{    
    script_frame_render(world,world->script);
    //world_draw_model(world, world->test_object, world->normal_shader, test_model, true);
}

void world_frame(struct world* world)
{
    world->render_count = 0;

    glViewport(0, 0, world->gfx.screen_width, world->gfx.screen_height);
    world->viewport[2] = (float)world->gfx.screen_width;
    world->viewport[3] = (float)world->gfx.screen_height;
    
    //glm_ortho(-10.f,10.f,-10.f,10.f,1.0f,7.5f,world->p);
    //glm_lookat((vec3){-2.f,4.f,-1.f},(vec3){0.f,0.f,0.f},(vec3){0.f,1.f,0.f},world->v);

    glClearColor(world->gfx.clear_color[0], world->gfx.clear_color[1], world->gfx.clear_color[2], world->gfx.clear_color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    sglc(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    glClear(GL_DEPTH_BUFFER_BIT);

    script_frame(world, world->script);
    
    // shadow pass
    //glViewport(0,0,SHADOW_WIDTH,SHADOW_HEIGHT);
    //glBindFramebuffer(GL_FRAMEBUFFER, world->gfx.depth_map_fbo);
    //glClear(GL_DEPTH_BUFFER_BIT);
    //world_frame_render(world);
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);

    
    if(world->cam.pitch > 85.f)
        world->cam.pitch = 85.f;
    if(world->cam.pitch < -85.f)
        world->cam.pitch = -85.f;

    glm_perspective(world->cam.fov * M_PI_180f, world->gfx.screen_width/world->gfx.screen_height, 0.1f, world->gfx.fog_maxdist, world->p);
    glm_ortho(0.f, world->viewport[2], 0.f, world->viewport[3], 0.1f, 1000.f, world->ui->projection);

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
    //glm_lookat((vec3){world->cam.position[0]-10.f,world->cam.position[1]+10.f,world->cam.position[2]-10.f},(vec3){world->cam.position[0],world->cam.position[1],world->cam.position[2]},(vec3){0.f,1.f,0.f},world->v);

    glm_mul(world->p, world->v, world->vp);

    //if(get_focus())
    //{
    //    world->cam.yaw += mouse_position[0] * world->delta_time * 10.f;
    //    world->cam.pitch -= mouse_position[1] * world->delta_time * 10.f;
    //}

    //glm_lookat(world->cam.position, (vec3){0.f,16.f,0.f}, world->cam.up, world->v);

    // render pass
    /*glDisable(GL_DEPTH_TEST);
    world_draw_model(world, world->gfx.sky_ball, world->sky_shader, sky_matrix, false);
    world_draw_primitive(world, world->cloud_shader, GL_FILL, PRIMITIVE_PLANE, cloud_matrix, (vec4){1.f,1.f,1.f,1.f});
    glEnable(GL_DEPTH_TEST);*/
    world_frame_render(world);

    // world_draw_model(world, world->test_object, world->normal_shader, test_model2, true);

    ui_draw_text(world->ui, 0.f, world->gfx.screen_height-16.f, "sglthing dev", 1.f);

    char dbg_info[256];
    int old_elements = world->ui->ui_elements;
    world->ui->ui_elements = 0;
    snprintf(dbg_info, 256, "DEBUG\n\ncam: V=(%.2f,%.2f,%.2f)\nY=%.2f,P=%.2f\nU=(%.2f,%.2f,%.2f)\nF=(%.2f,%.2f,%.2f)\nR=(%.f,%.f,%.f)\nF=%.f\nv=(%i,%i)\nr (scene)=%i,r (ui)=%i\nt=%f, d=%f\n\nphysics pause=%s\nphysics geoms=%i\ncollisions in frame=%i\n",
        world->cam.position[0], world->cam.position[1], world->cam.position[2],
        world->cam.yaw, world->cam.pitch,
        world->cam.up[0], world->cam.up[1], world->cam.up[2],
        world->cam.front[0], world->cam.front[1], world->cam.front[2],
        world->cam.right[0], world->cam.right[1], world->cam.right[2],
        world->cam.fov,
        (int)world->viewport[2], (int)world->viewport[3],
        world->render_count,
        old_elements,
        glfwGetTime(), world->delta_time,
        world->physics.paused?"true":"false",
        dSpaceGetNumGeoms(world->physics.space),
        world->physics.collisions_in_frame);
    ui_draw_text(world->ui, 0.f, world->gfx.screen_height-(16.f*3), dbg_info, 1.f);

    script_frame_ui(world, world->script);

    for(int i = 0; i < archives_loaded; i++)
    {
        snprintf(dbg_info, 256, "FS Archive %i: %s", i, archives[i].directory);
        ui_draw_text(world->ui, world->gfx.screen_width/2, world->gfx.screen_height-(16.f*3)-(i*16.f), dbg_info, 1.f);
    }

    world->physics.collisions_in_frame = 0;
    if(!world->physics.paused && world->delta_time != 0.0 && !keys_down[GLFW_KEY_GRAVE_ACCENT])
    {
        dSpaceCollide(world->physics.space,(void*)world,&world_physics_callback);
        dWorldQuickStep(world->physics.world, MIN(0.03f,world->delta_time));
        dJointGroupEmpty(world->physics.contactgroup);
    }

    if(keys_down[GLFW_KEY_GRAVE_ACCENT])
    {
        char* pixels = (char*)malloc(3*world->gfx.screen_width*world->gfx.screen_height);
        sglc(glReadPixels(0, 0, world->gfx.screen_width, world->gfx.screen_height, GL_RGB, GL_UNSIGNED_BYTE, pixels));
        stbi_flip_vertically_on_write(true);
        stbi_write_png("screenshot.png", world->gfx.screen_width, world->gfx.screen_height, 3, pixels, 0);
        free(pixels);
    }
}

static void __easy_uniforms(struct world* world, int shader_program, mat4 model_matrix)
{
    // camera
    glUniformMatrix4fv(glGetUniformLocation(shader_program,"model"), 1, GL_FALSE, model_matrix[0]);
    glUniformMatrix4fv(glGetUniformLocation(shader_program,"view"), 1, GL_FALSE, world->v[0]);
    glUniformMatrix4fv(glGetUniformLocation(shader_program,"projection"), 1, GL_FALSE, world->p[0]);
    glUniform3fv(glGetUniformLocation(shader_program,"camera_position"), 1, world->cam.position);

    // fog
    glUniform1f(glGetUniformLocation(shader_program,"fog_maxdist"), world->gfx.fog_maxdist);
    glUniform1f(glGetUniformLocation(shader_program,"fog_mindist"), world->gfx.fog_mindist);
    glUniform4fv(glGetUniformLocation(shader_program,"fog_color"), 1, world->gfx.fog_color);

    // misc
    glUniform4fv(glGetUniformLocation(shader_program,"viewport"), 1, world->viewport);
    glUniform1f(glGetUniformLocation(shader_program,"time"), (float)glfwGetTime());
    glUniform1i(glGetUniformLocation(shader_program,"banding_effect"), world->gfx.banding_effect);

    while(glGetError()!=0);
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

void world_draw_model(struct world* world, struct model* model, int shader_program, mat4 model_matrix, bool set_textures)
{
    mat4 mvp;

    ASSERT(model != 0);
    ASSERT(shader_program != 0);
    int last_vertex_array = 0;
    char txbuf[64];
    for(int i = 0; i < model->mesh_count; i++)
    {
        sglc(glUseProgram(shader_program));

        __easy_uniforms(world, shader_program, model_matrix);
        if(world->render_area)
            light_area_set_uniforms(world->render_area, shader_program);
        struct mesh* mesh_sel = &model->meshes[i];
        vec3 src_position = {0.f, 1.f, 0.f};
        
        if(set_textures)
        {
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
        }
        if(mesh_sel->vertex_array != last_vertex_array)
        {
            last_vertex_array = mesh_sel->vertex_array;
            sglc(glBindVertexArray(mesh_sel->vertex_array));
        }
        //glBindVertexBuffer(0, mesh_sel->vertex_buffer, 0, 3 * sizeof(float));
        sglc(glDrawElements(GL_TRIANGLES, mesh_sel->element_count, GL_UNSIGNED_INT, 0));
        world->render_count++;

        //snprintf(txbuf,64,"%s:%i\n(%.2f,%.2f,%.2f)\n%p", model->name, i, model_matrix[3][0],model_matrix[3][1],model_matrix[3][2],model);
        //ui_draw_text_3d(world->ui, world->viewport, world->cam.position, world->cam.front, (vec3){0.f,1.f,0.f}, world->cam.fov, model_matrix, world->vp, txbuf);
        //world_draw_primitive(world, PRIMITIVE_BOX, model_matrix, (vec4){1.f, 1.f, 1.f, 1.f});
    }
}

void world_draw_primitive(struct world* world, int shader, int fill, enum primitive_type type, mat4 model_matrix, vec4 color)
{
    sglc(glUseProgram(shader));
    __easy_uniforms(world, shader, model_matrix);
    glUniform4fv(glGetUniformLocation(shader,"color"), 1, color);
    glGetError();
    sglc(glBindVertexArray(world->primitives.debug_arrays));
    switch(type)
    {
        case PRIMITIVE_BOX:
            sglc(glPolygonMode(GL_FRONT_AND_BACK,fill));
            sglc(glBindBuffer(GL_ARRAY_BUFFER, world->primitives.box_primitive));
            sglc(glDrawArrays(GL_TRIANGLES, 0, world->primitives.box_vertex_count));
            sglc(glPolygonMode(GL_FRONT_AND_BACK,GL_FILL));
            break;
        case PRIMITIVE_ARROW:
            sglc(glBindBuffer(GL_ARRAY_BUFFER, world->primitives.arrow_primitive));
            sglc(glDrawArrays(GL_LINES, 0, world->primitives.arrow_vertex_count));
            break;
        case PRIMITIVE_PLANE:
            sglc(glPolygonMode(GL_FRONT_AND_BACK,fill));
            sglc(glBindBuffer(GL_ARRAY_BUFFER, world->primitives.plane_primitive));
            sglc(glDrawArrays(GL_TRIANGLES, 0, world->primitives.plane_vertex_count));
            sglc(glPolygonMode(GL_FRONT_AND_BACK,GL_FILL));
            break;
    }
}