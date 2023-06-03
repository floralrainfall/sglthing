#include "memory.h"
#include <glib-2.0/glib.h>
#include <stdbool.h>

#define GUARD_BYTE 0xBABE

struct __memory_alloc
{
    size_t len;
    void* offset;
    char caller[64];
    double time;
};

static GArray* memory_alloc_table;
static GTimer* memory_timer;

void m2_init()
{
    memory_alloc_table = g_array_new(true, true, sizeof(struct __memory_alloc));
    memory_timer = g_timer_new();
}

void* _malloc2(size_t len, char* caller, int line)
{
    struct __memory_alloc new_entry;
    new_entry.len = len;
    new_entry.offset = malloc(len + 4);
    new_entry.time = g_timer_elapsed(memory_timer, NULL);
    if(caller)
        snprintf(new_entry.caller, 64, "%s:%i", caller, line);
    else
        strncpy(new_entry.caller, "???", 64);
    if(new_entry.offset)
    {
        uint16_t* guard_ptr = (uint16_t*)new_entry.offset;
        *guard_ptr = GUARD_BYTE;
        *(guard_ptr + len) = GUARD_BYTE;
        g_array_append_val(memory_alloc_table, new_entry);
        return new_entry.offset + 2;
    }
    else
    {
        printf("sglthing: out of memory!\n");
        return 0;
    }
}

void free2(void* blk)
{
    if(!blk)
        return;
    blk -= 2;
    for(int i = 0; i < memory_alloc_table->len; i++)
    {
        struct __memory_alloc entry = g_array_index(memory_alloc_table, struct __memory_alloc, i);
        if(entry.offset == blk)
        {
            uint16_t* guard_ptr = (uint16_t*)entry.offset;
            if(*guard_ptr != GUARD_BYTE)
                printf("sglthing: guard byte 1 altered (%04x @ %p)\n", *guard_ptr, guard_ptr);
            if(*(guard_ptr + entry.len) != GUARD_BYTE)
                printf("sglthing: guard byte 2 altered (%04x @ %p)\n", *(guard_ptr + entry.len), (guard_ptr + entry.len));
            free(entry.offset);
            g_array_remove_index(memory_alloc_table, i);
            return;
        }
    }
    printf("sglthing: unable to free %p\n", blk);
}