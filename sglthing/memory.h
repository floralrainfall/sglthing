#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

extern size_t memory_allocated;
extern size_t memory_leaked;
extern int memory_allocations;

void m2_init(size_t memory);

void m2_frame();

void* _malloc2(size_t len, char* caller, int line);
void _free2(void* blk, char* caller, int line);

#define malloc2(l) _malloc2(l,__FILE__,__LINE__)
#define malloc2_s(l,n) _malloc2(l,n ## __FILE__,__LINE__)
#define free2(l) _free2(l,__FILE__,__LINE__)

#endif