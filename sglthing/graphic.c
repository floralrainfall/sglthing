#include "graphic.h"
#include <sglthing/memory.h>

static struct graphic_context* current_context = 0;

#define __gl_check_error() __gl_check_error2(__FILE__, __LINE__);
static void __gl_check_error2(char* file, int line)
{
    int error = glGetError();
    if(error)
    {
        printf("sglthing: gl error %04x (%s:%i)\n", error, file, line);
    }
}

static int __gl_slot_to_target(enum bref_slot slot)
{
    switch(slot)
    {
        case SLOT_VERTEX_ARRAY:
            return GL_ARRAY_BUFFER;
        case SLOT_ELEMENT_ARRAY:
            return GL_ELEMENT_ARRAY_BUFFER;
        default:
            break;
    }
    return slot; // if its not a known value then the program has probably passed in its own hw specific value
}

void graphic_varray_create(struct graphic_varray* varray_out, enum render_type render_type)
{
    glGenVertexArrays(1, &varray_out->gl_vao_id);
    __gl_check_error();
    varray_out->v_entries = g_array_new(true, true, sizeof(struct graphic_varray_entry));
    varray_out->render_type = render_type;
    varray_out->render_mode = RM_TRIANGLES;
    varray_out->bref_array = 0;
    varray_out->bref_elements = 0;
}

void graphic_varray_bind(struct graphic_varray* varray)
{
    glBindVertexArray(varray->gl_vao_id);
    __gl_check_error();
}

void graphic_varray_add(struct graphic_varray* varray, struct graphic_varray_entry entry)
{
    g_array_append_val(varray->v_entries, entry);
}

void graphic_varray_upload(struct graphic_varray* varray)
{
    graphic_varray_bind(varray);

    for(int i = 0; i < varray->v_entries->len; i++)
    {
        struct graphic_varray_entry entry = g_array_index(varray->v_entries, struct graphic_varray_entry, i);

        graphic_bref_bind(entry.buff);

        int gl_type = 0;
        switch(entry.type)
        {
            case DT_FLOAT:
                gl_type = GL_FLOAT;
                break;
            case DT_BYTE:
                gl_type = GL_UNSIGNED_BYTE;
                break;
            case DT_INT:
                gl_type = GL_INT;
                break;
            default:
                gl_type = GL_UNSIGNED_BYTE;
                break;
        }

        printf("sglthing: bound %i (e: %i) %i s: %i S: %i to buf %p\n", entry.layout_id, entry.buff_array_len, gl_type, entry.buff_array_size, entry.buff_array_stride, entry.buff);

        sglc(glVertexAttribPointer(entry.layout_id, entry.buff_array_len, gl_type, GL_FALSE, entry.buff_array_size, (void*)entry.buff_array_stride));
        __gl_check_error();
        sglc(glEnableVertexAttribArray(entry.layout_id));
        __gl_check_error();
    }
}

void graphic_bref_create(struct graphic_bref* bref_out, enum bref_slot slot)
{
    glGenBuffers(1, &bref_out->gl_buff_id);
    __gl_check_error();
    bref_out->buff_data = 0;
    bref_out->buff_len = 0;
    bref_out->slot = slot;
}

void graphic_bref_bind(struct graphic_bref* bref)
{           
    int target = 0;
    
    glBindBuffer(__gl_slot_to_target(bref->slot), bref->gl_buff_id);
    __gl_check_error();
}

void graphic_bref_upload(struct graphic_bref* bref, void* data, int data_len)
{
    graphic_bref_bind(bref);

    if(bref->buff_data)
        free2(bref->buff_data);

    bref->buff_data = malloc2(data_len);
    memcpy(bref->buff_data, data, data_len);
    bref->buff_len = data_len;

    glBufferData(__gl_slot_to_target(bref->slot), bref->buff_len, bref->buff_data, GL_STATIC_DRAW);
    __gl_check_error();
}

void graphic_bref_delete(struct graphic_bref* bref)
{
    glDeleteBuffers(1, &bref->gl_buff_id);
    __gl_check_error();

    free2(bref->buff_data);
}

void graphic_render_varray(struct graphic_varray* varray, int shader, mat4 model)
{
    graphic_varray_bind(varray);
    if(varray->bref_array)
        graphic_bref_bind(varray->bref_array);
    if(varray->bref_elements)
        graphic_bref_bind(varray->bref_elements);

    glUseProgram(shader);

    if(graphic_context_current())
    {
        graphic_context_upload(graphic_context_current(), shader);
    }
    glUniformMatrix4fv(glGetUniformLocation(shader,"model"), 1, GL_FALSE, model[0]);

    __gl_check_error();

    int render_mode = 0;
    switch(varray->render_mode)
    {
        case RM_TRIANGLES:
            render_mode = GL_TRIANGLES;
            break;
        case RM_QUADS:
            render_mode = GL_QUADS;
            break;
    }
    
    switch(varray->render_type)
    {
        case RT_ARRAYS:
            glDrawArrays(render_mode, 0, varray->gl_count);
            break;
        case RT_ELEMENTS:
            glDrawElements(render_mode, varray->gl_count, GL_UNSIGNED_INT, 0);
            break;
    }

    __gl_check_error();
}

void graphic_render_varray_instanced(struct graphic_varray* varray, int shader, mat4 model, int instances)
{
    graphic_varray_bind(varray);
    if(varray->bref_array)
        graphic_bref_bind(varray->bref_array);
    if(varray->bref_elements)
        graphic_bref_bind(varray->bref_elements);

    glUseProgram(shader);

    if(graphic_context_current())
    {
        graphic_context_upload(graphic_context_current(), shader);
    }
    glUniformMatrix4fv(glGetUniformLocation(shader,"model"), 1, GL_FALSE, model[0]);

    __gl_check_error();

    int render_mode = 0;
    switch(varray->render_mode)
    {
        case RM_TRIANGLES:
            render_mode = GL_TRIANGLES;
            break;
        case RM_QUADS:
            render_mode = GL_QUADS;
            break;
    }
    
    switch(varray->render_type)
    {
        case RT_ARRAYS:
            glDrawArraysInstanced(render_mode, 0, varray->gl_count, instances);
            break;
        case RT_ELEMENTS:
            glDrawElementsInstanced(render_mode, varray->gl_count, GL_UNSIGNED_INT, 0, instances);
            break;
    }

    __gl_check_error();
}

void graphic_context_push()
{
    struct graphic_context* new_context = malloc2(sizeof(struct graphic_context));
    new_context->parent_context = current_context;
    current_context = new_context;
}

void graphic_context_push_copy()
{
    graphic_context_push();
    struct graphic_context* parent_context = current_context->parent_context;
    memcpy(current_context, parent_context, sizeof(struct graphic_context));
    current_context->parent_context = parent_context;
}

struct graphic_context* graphic_context_current()
{
    return current_context;
}

void graphic_context_pop()
{
    if(current_context)
    {
        struct graphic_context* parent_context = current_context->parent_context;
        free2(current_context);
        current_context = parent_context;
    }
    else
        printf("sglthing: attempted to pop null context\n");
}

void graphic_context_upload(struct graphic_context* context, int shader_program)
{
    glUniformMatrix4fv(glGetUniformLocation(shader_program,"view"), 1, GL_FALSE, context->cam.view[0]);
    glUniformMatrix4fv(glGetUniformLocation(shader_program,"projection"), 1, GL_FALSE, context->cam.projection[0]);
    glUniformMatrix4fv(glGetUniformLocation(shader_program,"lsm"), 1, GL_FALSE, context->cam.lsm[0]);
    glUniformMatrix4fv(glGetUniformLocation(shader_program,"lsm_far"), 1, GL_FALSE, context->cam.lsm_far[0]);
    glUniform3fv(glGetUniformLocation(shader_program,"camera_position"), 1, context->cam.position);
    glUniform1f(glGetUniformLocation(shader_program,"fog_maxdist"), context->gfx.fog_maxdist);
    glUniform1f(glGetUniformLocation(shader_program,"fog_mindist"), context->gfx.fog_mindist);
    glUniform4fv(glGetUniformLocation(shader_program,"fog_color"), 1, context->gfx.fog_color);
    glUniform4fv(glGetUniformLocation(shader_program,"viewport"), 1, context->gfx.viewport);
    glUniform3fv(glGetUniformLocation(shader_program,"sun_direction"), 1, context->gfx.sun_direction);
    glUniform3fv(glGetUniformLocation(shader_program,"sun_position"), 1, context->gfx.sun_position);
    glUniform3fv(glGetUniformLocation(shader_program,"diffuse"), 1, context->gfx.diffuse);
    glUniform3fv(glGetUniformLocation(shader_program,"ambient"), 1, context->gfx.ambient);
    glUniform3fv(glGetUniformLocation(shader_program,"specular"), 1, context->gfx.specular);
    glUniform1f(glGetUniformLocation(shader_program,"time"), context->time);
    glUniform1i(glGetUniformLocation(shader_program,"banding_effect"), context->gfx.banding_effect);
    glUniform1i(glGetUniformLocation(shader_program,"sel_map"), context->gfx.current_map);

    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, context->gfx.depth_map_texture);
    glUniform1i(glGetUniformLocation(shader_program,"depth_map"), 7);
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, context->gfx.depth_map_texture_far);
    glUniform1i(glGetUniformLocation(shader_program,"depth_map_far"), 8);

    while(glGetError()!=0);
}

void graphic_texture_bind(int shader, enum texture_type type, int slot, int id)
{
    char* text_type_str[] = {
        "diffuse",
        "specular",
    };
    char text_uniform[64];
    snprintf(text_uniform, 64, "%s%i", text_type_str[type], slot);
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, id);
    glUniform1i(glGetUniformLocation(shader,text_uniform), slot);
}