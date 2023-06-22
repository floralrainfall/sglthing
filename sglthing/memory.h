#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define SGL_MEMORY_GUARD_BYTE 0xf00dbabef00dbabe

typedef uint64_t alloc_guard_type;

struct __memory_alloc
{
    size_t len;
    void* offset;
    char caller[64];
    double time;
    bool dirty;
    int mem_base_off;

    int mem_id;

    alloc_guard_type* guard_byte_1;
    alloc_guard_type* guard_byte_2;
};

extern size_t memory_allocated;
extern size_t memory_leaked;
extern int memory_allocations;

void m2_init(size_t memory);
void m2_draw_dbg(void* world_fk);
void m2_frame();
struct __memory_alloc m2_allocation(void* blk);

void* _malloc2(size_t len, char* caller, int line);
void _free2(void* blk, char* caller, int line);

// for now do not rely on this for aligned memory allocations
#define malloc2(l) _malloc2(l,__FILE__,__LINE__)
#define malloc2_s(l,n) _malloc2(l,n ## __FILE__,__LINE__)
#define free2(l) _free2(l,__FILE__,__LINE__)

#endif