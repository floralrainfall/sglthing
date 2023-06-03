#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdlib.h>

void m2_init();

void* _malloc2(size_t len, char* caller, int line);
void free2(void* blk);

#define malloc2(l) _malloc2(l,__FILE__,__LINE__)

#endif