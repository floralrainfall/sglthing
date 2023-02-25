#ifndef S7_TRANSFORM_H
#define S7_TRANSFORM_H
#include "s7.h"
#include <cglm/cglm.h>

struct transform
{
    s7_double px;
    s7_double py;
    s7_double pz;

    s7_double rx;
    s7_double ry;
    s7_double rz;
    s7_double rw;
    
    s7_double sx;
    s7_double sy;
    s7_double sz;

    mat4 transform_matrix;
};

void transform_to_matrix(mat4* dest, struct transform transform);
void sgls7_transform_register(s7_scheme* sc);
int sgls7_transform_type();

#endif