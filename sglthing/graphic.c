#include "graphic.h"
#include <sglthing/memory.h>

static struct graphic_context* current_context = 0;

#define __gl_check_error() __gl_check_error2(__FILE__, __LINE__);
static void __gl_check_error2(char* file, int line)
{
    int error = glGetError();
    if(error)
    {
        printf("sglthing: gl error %i (%s:%i)\n", error, file, line);
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

void graphic_varray_create(struct graphic_varray* varray_out)
{
    glGenVertexArrays(1, &varray_out->gl_vao_id);
    __gl_check_error();
    varray_out->v_entries = g_array_new(true, true, sizeof(struct graphic_varray_entry));
}

void graphic_varray_bind(struct graphic_varray* varray)
{
    glBindVertexArray(varray->gl_vao_id);
    __gl_check_error();
}

void graphic_varray_add(struct graphic_varray* varray, struct graphic_varray_entry entry)
{
    g_array_append_val(varray, entry);
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

        sglc(glVertexAttribPointer(entry.layout_id, entry.buff_array_len, gl_type, GL_FALSE, entry.buff_array_size, (void*)entry.buff_array_stride));
        __gl_check_error();
        sglc(glEnableVertexAttribArray(entry.layout_id));
        __gl_check_error();
    }
}

void graphic_bref_create(struct graphic_bref* bref_out, enum bref_slot slot)
{
    glGenBuffers(1, &bref_out);
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

void graphic_render_varray(struct graphic_varray* varray, int shader)
{
    graphic_varray_bind(varray);

    glUseProgram(shader);
    __gl_check_error();
}