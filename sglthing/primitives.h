#ifndef PRIMITIVES_H
#define PRIMITIVES_H

struct primitives
{
    int arrow_primitive;
    int arrow_vertex_count;
    int box_primitive;
    int box_vertex_count;
    int plane_primitive;
    int plane_vertex_count;

    int debug_arrays;
};

struct primitives create_primitives();

#endif