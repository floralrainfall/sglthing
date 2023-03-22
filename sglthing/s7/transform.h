#ifndef S7_TRANSFORM_H
#define S7_TRANSFORM_H
#include "s7.h"
#include <cglm/cglm.h>

#define OLD_VERSION(n)   \
    s7_double n;         \
    s7_double n ## _old; \

struct transform
{
    OLD_VERSION(px);
    OLD_VERSION(py);
    OLD_VERSION(pz);

    mat4 translation_matrix;

    OLD_VERSION(rx);
    OLD_VERSION(ry);
    OLD_VERSION(rz);
    OLD_VERSION(rw);
    
    mat4 rotation_matrix;

    OLD_VERSION(sx);
    OLD_VERSION(sy);
    OLD_VERSION(sz);

    mat4 scale_matrix;
};

void transform_to_matrix(struct transform* transform);
void sgls7_transform_register(s7_scheme* sc);
int sgls7_transform_type();

#endif