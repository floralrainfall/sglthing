#include "primitives.h"
#include "sglthing.h"
#include "graphic.h"
#ifndef HEADLESS
#include <glad/glad.h>
#endif

float box_data[] = {
    -0.5f,-0.5f,-0.5f, // triangle 1 : begin
    -0.5f,-0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f, // triangle 1 : end
    0.5f, 0.5f,-0.5f, // triangle 2 : begin
    -0.5f,-0.5f,-0.5f,
    -0.5f, 0.5f,-0.5f, // triangle 2 : end
    0.5f,-0.5f, 0.5f,
    -0.5f,-0.5f,-0.5f,
    0.5f,-0.5f,-0.5f,
    0.5f, 0.5f,-0.5f,
    0.5f,-0.5f,-0.5f,
    -0.5f,-0.5f,-0.5f,
    -0.5f,-0.5f,-0.5f,
    -0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f,-0.5f,
    0.5f,-0.5f, 0.5f,
    -0.5f,-0.5f, 0.5f,
    -0.5f,-0.5f,-0.5f,
    -0.5f, 0.5f, 0.5f,
    -0.5f,-0.5f, 0.5f,
    0.5f,-0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f,-0.5f,-0.5f,
    0.5f, 0.5f,-0.5f,
    0.5f,-0.5f,-0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f,-0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f,-0.5f,
    -0.5f, 0.5f,-0.5f,
    0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f,-0.5f,
    -0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f,
    0.5f,-0.5f, 0.5f
};

float arrow_data[] = {
    0.f, 0.f, -0.5f,
    0.f, 0.f, 0.5f,

    0.f, 0.5f, -0.5f,
    0.f, 0.f, -2.f,

    0.f, -0.5f, -0.5f,
    0.f, 0.f, -2.f,
};

float plane_data[] = {
    0.5f, 0.f, -0.5f, // top left corner
    0.5f, 0.f, 0.5f, // bottom left corner
    -0.5f, 0.f, 0.5f, // top right corner


    0.5f, 0.f, -0.5f, // top left corner
    -0.5f, 0.f, -0.5f, // bottom right corner
    -0.5f, 0.f, 0.5f, // bottom left corner
};

// BUG: only the last primitive set up is rendered
#define PRIM_SETUP(x) \
    sglc(glGenBuffers(1, &prims.x##_primitive)); \
    sglc(glBindVertexArray(prims.debug_arrays)); \
    sglc(glBindBuffer(GL_ARRAY_BUFFER, prims.x##_primitive)); \
    sglc(glBufferData(GL_ARRAY_BUFFER, sizeof(x##_data), &x##_data[0], GL_STATIC_DRAW)); \
    prims.x##_vertex_count = sizeof(x##_data)/sizeof(float);
//    sglc(glBindBuffer(GL_ARRAY_BUFFER, 0)); \
//    sglc(glBindVertexArray(0));

struct primitives create_primitives()
{
    struct primitives prims;
    sglc(glGenVertexArrays(1, &prims.debug_arrays));

    PRIM_SETUP(arrow);
    PRIM_SETUP(box);
    PRIM_SETUP(plane);

    sglc(glBindVertexArray(prims.debug_arrays));

    // vec3: position
    sglc(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0));
    sglc(glEnableVertexAttribArray(0));
    sglc(glBindVertexArray(0));

    return prims;
}