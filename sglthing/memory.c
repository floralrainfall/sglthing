#include "memory.h"
#include <glib-2.0/glib.h>
#include <stdbool.h>
#include <stdio.h>
#include "prof.h"

#define SGL_MEMORY_GUARD_BYTE 0xf00dbabef00dbabe

size_t memory_allocated = 0;
size_t memory_leaked = 0;
int memory_allocations = 0;

typedef uint64_t alloc_guard_type;

struct __memory_alloc
{
    size_t len;
    void* offset;
    char caller[64];
    double time;
    bool dirty;
    int mem_base_off;

    alloc_guard_type* guard_byte_1;
    alloc_guard_type* guard_byte_2;
};

static GArray* memory_alloc_table;
static GTimer* memory_timer;

void m2_init(size_t memory)
{
    memory_alloc_table = g_array_new(true, true, sizeof(struct __memory_alloc*));
    memory_timer = g_timer_new();
}

void m2_frame()
{
    profiler_event("m2_frame");
    for(int i = 0; i < memory_alloc_table->len; i++)
    {
        struct __memory_alloc* entry = g_array_index(memory_alloc_table, struct __memory_alloc*, i);
        if(!entry->dirty)
        {
            double time = g_timer_elapsed(memory_timer, NULL) - entry->time;
            if(*entry->guard_byte_1 != SGL_MEMORY_GUARD_BYTE)
            {
                entry->dirty = true;
                printf("sglthing: guard byte 1 altered on alloc %s (m2_frame scan) (%08x @ %p, ov: %08x, d: %i)\n", entry->caller, *entry->guard_byte_1, entry->guard_byte_1, SGL_MEMORY_GUARD_BYTE, entry->dirty);
            }
            if(*entry->guard_byte_2 != SGL_MEMORY_GUARD_BYTE)
            {
                entry->dirty = true;
                printf("sglthing: guard byte 2 altered on alloc %s (m2_frame scan) (%08x @ %p, ov: %08x, d: %i)\n", entry->caller, *entry->guard_byte_2, entry->guard_byte_2, SGL_MEMORY_GUARD_BYTE, entry->dirty);
            }
        }
    }
    profiler_end();
}

void* _malloc2(size_t len, char* caller, int line)
{
    void* entry_offset = malloc(len + (sizeof(alloc_guard_type) * 2));
    memset(entry_offset, len, 0);
    if(!entry_offset)
    {
        printf("sglthing: out of memory!\n");
        return 0;
    }
    struct __memory_alloc* new_entry = malloc(sizeof(struct __memory_alloc));
    new_entry->len = len;
    new_entry->offset = entry_offset;
    new_entry->time = g_timer_elapsed(memory_timer, NULL);
    new_entry->dirty = false;
    memory_allocated += len;
    if(caller)
        snprintf(new_entry->caller, 64, "%s:%i", caller, line);
    else
        strncpy(new_entry->caller, "unknown_alloc", 64);
    new_entry->mem_base_off = sizeof(alloc_guard_type);
    new_entry->guard_byte_1 = new_entry->offset;
    new_entry->guard_byte_2 = new_entry->offset + len + new_entry->mem_base_off; 
    *new_entry->guard_byte_1 = SGL_MEMORY_GUARD_BYTE;
    *new_entry->guard_byte_2 = SGL_MEMORY_GUARD_BYTE;
    g_array_append_val(memory_alloc_table, new_entry);
    memory_allocations++;
    return new_entry->offset + new_entry->mem_base_off;
}

void _free2(void* blk, char* caller, int line)
{
    if(!blk)
        return;
    for(int i = 0; i < memory_alloc_table->len; i++)
    {
        struct __memory_alloc* entry = g_array_index(memory_alloc_table, struct __memory_alloc*, i);
        if(entry->offset == blk - entry->mem_base_off)
        {
            alloc_guard_type* guard_ptr = (alloc_guard_type*)entry->offset;
            bool leak = false;
            if(*entry->guard_byte_1 != SGL_MEMORY_GUARD_BYTE)
            {
                printf("sglthing: guard byte 1 altered on alloc %s (%08x @ %p, ov: %08x, d: %i)\n", entry->caller, *entry->guard_byte_1, entry->guard_byte_1, SGL_MEMORY_GUARD_BYTE, entry->dirty);
                leak = true;
            }
            if(*entry->guard_byte_2 != SGL_MEMORY_GUARD_BYTE)
            {
                printf("sglthing: guard byte 2 altered on alloc %s (%08x @ %p, ov: %08x, d: %i)\n", entry->caller, *entry->guard_byte_2, entry->guard_byte_2, SGL_MEMORY_GUARD_BYTE, entry->dirty);
                leak = true;
            }
            if(!leak)
            {
                memory_allocated -= entry->len;
                free(entry->offset);
                g_array_remove_index(memory_alloc_table, i);
                free(entry);
                memory_allocations--;
            }
            else
            {
                memory_leaked += entry->len;
                printf("sglthing: leaking %fkB (%s:%i)\n", caller, line, entry->len / 1000.f);
            }
            return;
        }
    }
#ifdef FREE_UNFOUND
    // just try freeing the address
    printf("sglthing: freeing un-m2 allocated %p\n",blk);
    free(blk);
#else 
    printf("sglthing: attempted to free un-m2 allocated %p\n",blk);
#endif
}