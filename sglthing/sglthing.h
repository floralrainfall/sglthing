#ifndef SGLTHING_H
#define SGLTHING_H

#include "_sglthing.h"

void __sglthing_assert_failed();

#define ASSERT(x) if(!(x)) {                                                      \
        printf("sglthing: assert '%s' failed (%s:%i)\n", #x, __FILE__, __LINE__); \
        __sglthing_assert_failed();                                               \
        exit(-1);                                                                 \
    }

#define M_PIf 3.14159265359f
#define M_PI_180f 0.01745329251f
#define M_PI_2f 1.57079632679f

#define PUSH_ARRAY(a,c,x) \
    a[c] = x; \
    c++;

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define STRUCT_SUB_SZ(t,x) sizeof(*((t *)0)->x);

#endif