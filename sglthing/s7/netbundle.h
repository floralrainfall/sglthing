#ifndef S7_NETBUNDLE_H
#define S7_NETBUNDLE_H
#include "s7.h"
#include "netbundle.h"

struct net_bundle_entry 
{
    enum {
        STRING,
        INTEGER,
        FLOAT,
        TRANSFORM,
    } type;
    union {
        struct {
            char data[64];
            int length;
        } string;
        struct {
            s7_int value;
        } integer;
        struct {
            s7_double value;
        } _float;
        struct {
            float p_x, p_y, p_z;
            float r_x, r_y, r_z, r_w;
            float s_x, s_y, s_z;
        } transform;
    } data;
};

struct net_bundle
{
    struct net_bundle_entry entries[16];
    int entry_count;
};

void sgls7_netbundle_register(s7_scheme* sc);
int sgls7_netbundle_type();

#endif