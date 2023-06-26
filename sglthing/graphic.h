#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <cglm/cglm.h>
#ifndef HEADLESS
#include <glad/glad.h>
#endif

#include <glib.h>
#include <signal.h>

struct vertex_normal
{
    vec3 position;
    vec3 normal;
};

#ifdef HEADLESS
#define sglc(f)
#define GL_VERTEX_SHADER 0
#define GL_FRAGMENT_SHADER 0
#define GL_GEOMETRY_SHADER 0 
#define GL_FILL 0
#else
#define sglc(f) {\
    { f; };\
    int x = glGetError();\
    if(x)\
    {\
        printf("sglthing: gl error %04x above from %s:%i\n", x, __FILE__, __LINE__);\
        raise(SIGINT);\
        exit(-1);\
    }\
}
#endif

struct graphic_context
{
    struct graphic_context* parent_context;
};

enum uniform_type
{
    UT_INTEGER,
    UT_FLOAT,
    UT_VECTOR2,
    UT_VECTOR2_INT,
    UT_VECTOR3,
    UT_VECTOR3_INT,
    UT_VECTOR4,
    UT_VECTOR4_INT,
    UT_MATRIX4,    
};

struct graphic_context_uniform
{
    enum uniform_type type;
    char uniform_name[64];
};

// Vertex Array
struct graphic_varray
{
    GArray* v_entries;
    int gl_vao_id;
};

enum bref_slot
{
    SLOT_VERTEX_ARRAY,
    SLOT_ELEMENT_ARRAY,
};

// Buffer Reference
struct graphic_bref
{
    void* buff_data;
    int buff_len;
    int gl_buff_id;
    enum bref_slot slot;
};

enum data_type
{
    DT_FLOAT,
    DT_INT,
    DT_BYTE,
};

struct graphic_varray_entry
{
    int layout_id;
    struct graphic_bref* buff;
    int buff_array_len;
    int buff_array_size;
    int buff_array_stride;
    enum data_type type;
};

// create vertex array
void graphic_varray_create(struct graphic_varray* varray_out);
// bind vertex array to hw
void graphic_varray_bind(struct graphic_varray* varray);
// add entry to vertex array
void graphic_varray_add(struct graphic_varray* varray, struct graphic_varray_entry entry);
// upload entries to vertex array (call after using graphic_varray_add, otherwise new entries wont take effect)
void graphic_varray_upload(struct graphic_varray* varray);

// create buffer reference
void graphic_bref_create(struct graphic_bref* bref_out, enum bref_slot slot);
// bind buffer to its slot
void graphic_bref_bind(struct graphic_bref* bref_out);
// upload buffer to buffer reference
void graphic_bref_upload(struct graphic_bref* bref, void* data, int data_len);
// delete buffer reference
void graphic_bref_delete(struct graphic_bref* bref_out);

// render vertex array
void graphic_render_varray(struct graphic_varray* varray, int shader);

#define ASSIMP_TO_GLM(a,g)                                                                                                                                                                                            \
    g[0][0] = a.a1; g[0][1] = a.b1; g[0][2] = a.c1; g[0][3] = a.d1; \
    g[1][0] = a.a2; g[1][1] = a.b2; g[1][2] = a.c2; g[1][3] = a.d2; \
    g[2][0] = a.a3; g[2][1] = a.b3; g[2][2] = a.c3; g[2][3] = a.d3; \
    g[3][0] = a.a4; g[3][1] = a.b4; g[3][2] = a.c4; g[3][3] = a.d4; 

#endif