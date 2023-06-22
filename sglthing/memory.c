#include "memory.h"
#include <glib-2.0/glib.h>
#include <stdio.h>
#include "prof.h"
#include "world.h"

size_t memory_allocated = 0;
size_t memory_leaked = 0;
int memory_allocations = 0;

static GArray* memory_alloc_table;
static GTimer* memory_timer;
static int memory_id;

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

#define ALIGN 8

void* _malloc2(size_t len, char* caller, int line)
{
    void* entry_offset = malloc(len + (sizeof(alloc_guard_type) * 2) + ALIGN);
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
    new_entry->mem_id = memory_id++;
    memory_allocated += len;
    if(caller)
        snprintf(new_entry->caller, 64, "%s:%i", caller, line);
    else
        strncpy(new_entry->caller, "unknown_alloc", 64);
    new_entry->mem_base_off = sizeof(alloc_guard_type) + ALIGN;
    new_entry->guard_byte_1 = new_entry->offset;
    new_entry->guard_byte_2 = new_entry->offset + len + new_entry->mem_base_off; 
    *new_entry->guard_byte_1 = SGL_MEMORY_GUARD_BYTE;
    *new_entry->guard_byte_2 = SGL_MEMORY_GUARD_BYTE;
    g_array_append_val(memory_alloc_table, new_entry);
    memory_allocations++;
    return new_entry->offset + new_entry->mem_base_off;
}

struct __memory_alloc m2_allocation(void* blk)
{
    for(int i = 0; i < memory_alloc_table->len; i++)
    {
        struct __memory_alloc* entry = g_array_index(memory_alloc_table, struct __memory_alloc*, i);
        if(entry->offset == blk - entry->mem_base_off)
        {
            return *entry;
        }
    }
    return (struct __memory_alloc){};
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

void m2_draw_dbg(void* world_fk)
{
    struct world* world = (struct world*)world_fk;
    float screen_ptg_wt = world->gfx.screen_width;
    float screen_ptg_x = 0;
    vec4 old_panel_background;
    glm_vec4_copy(world->ui->panel_background_color, old_panel_background);
    for(int i = 0; i < memory_alloc_table->len; i++)
    {
        struct __memory_alloc* entry = g_array_index(memory_alloc_table, struct __memory_alloc*, i);

        int id = entry->mem_id % 255;

        int data_r = abs((id / 16) + 1); // im using the same color system as i did in rdm for convenience
        int data_g = abs((id / 4 % 4) + 1);
        int data_b = abs((id % 4) + 1);
        world->ui->panel_background_color[0] = (((float)data_r)/4.f);
        world->ui->panel_background_color[1] = (((float)data_g)/4.f);
        world->ui->panel_background_color[2] = (((float)data_b)/4.f);

        float mem_ptg = ((float)entry->len) / memory_allocated;
        float scn_ptg = mem_ptg * screen_ptg_wt;
        ui_draw_panel(world->ui, screen_ptg_x, 64.f, scn_ptg, 64.f, 0.7f);
        ui_end_panel(world->ui);

        char meminfo[255];
        snprintf(meminfo, 255, "%s (%i bytes)", entry->caller, entry->len);

        ui_draw_panel(world->ui, 0.f, (i + 5) * 16.f, scn_ptg, 16.f, 0.5f);
        ui_draw_text(world->ui, 0.f, 16.f, meminfo, 0.25f);
        ui_end_panel(world->ui);
        screen_ptg_x += scn_ptg;
    }
    glm_vec4_copy(old_panel_background, world->ui->panel_background_color);
}