#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <cglm/cglm.h>
#include <glad/glad.h>

#include <signal.h>

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
        printf("sglthing: gl error %04x above from %s:%i\n", x, __FILE__, __LINE__);\
        raise(SIGINT);\
        exit(-1);\
    }\
}

#define ASSIMP_TO_GLM(a,g)                                                                                                                                                                                            \
    g[0][0] = a.a1; g[0][1] = a.b1; g[0][2] = a.c1; g[0][3] = a.d1; \
    g[1][0] = a.a2; g[1][1] = a.b2; g[1][2] = a.c2; g[1][3] = a.d2; \
    g[2][0] = a.a3; g[2][1] = a.b3; g[2][2] = a.c3; g[2][3] = a.d3; \
    g[3][0] = a.a4; g[3][1] = a.b4; g[3][2] = a.c4; g[3][3] = a.d4; 

#endif