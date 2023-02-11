#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <cglm/cglm.h>
#include <glad/glad.h>

struct vertex_normal
{
    vec3 position;
    vec3 normal;
};

#define sglc(f) {\
    { f; };\ 
    int x = glGetError();\
    if(x)\
    {\
        printf("sglthing: gl error %i above from %s:%i\n", x, __FILE__, __LINE__);\
        exit(-1);\
    }\
}

#endif