#ifndef SGLTHING_H
#define SGLTHING_H

#define ASSERT(x) if(!(x)) { printf("sglthing: assert '%s' failed\n", #x); exit(-1); }

#define M_PIf 3.14159265359f
#define M_PI_180f 0.01745329251f
#define M_PI_2f 1.57079632679f

#define PUSH_ARRAY(a,c,x) \
    a[c] = x; \
    c++;

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#endif