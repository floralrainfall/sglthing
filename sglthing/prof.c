#include "prof.h"
#include <glib-2.0/glib.h>
#include "ui.h"
#include "world.h"

#define PROFILER_ENABLE

static int event_id_last = 0;

struct profiler_event
{
    int id;
    struct profiler_event* parent_event;
    char name[64];
    double start_time;
};

static int record_id_last = 0;

struct profiler_event_record
{
    int id;
    uint64_t number_called;
    uint64_t number_called_last_second;
    double total_time;
    double total_time_last_second;
    double last_time;
};

struct {
    struct profiler_event* current_event;
    GHashTable* event_status;
    GTimer* timer;
    double next_second;
} global_profiler;

void profiler_global_init()
{
    global_profiler.timer = g_timer_new();
    global_profiler.current_event = 0;
    global_profiler.event_status = g_hash_table_new(g_str_hash, g_str_equal);
    event_id_last = 0;
    record_id_last = 0;
}

void profiler_end_frame()
{
//    if(global_profiler.current_event != 0)
//        printf("sglthing: prof current_event = %p\n", global_profiler.current_event);
}

void profiler_event(char* name)
{
#ifdef PROFILER_ENABLE
    struct profiler_event* new_event = (struct profiler_event*)malloc2(sizeof(struct profiler_event));
    new_event->start_time = g_timer_elapsed(global_profiler.timer, NULL);

    strncpy(new_event->name, name, 64);
    new_event->parent_event = global_profiler.current_event;
    new_event->id = event_id_last++;
    global_profiler.current_event = new_event;
#endif
}

void profiler_end()
{
#ifdef PROFILER_ENABLE
    if(!global_profiler.current_event)
        return;

    struct profiler_event* old_event = global_profiler.current_event;
    global_profiler.current_event = global_profiler.current_event->parent_event;

    gpointer key, event_data;
    g_hash_table_lookup_extended(global_profiler.event_status, old_event->name, &key, &event_data);    
    if(!key)
    {
        char* proc_name = (char*)malloc2(64);
        strncpy(proc_name, old_event->name, 64);
        struct profiler_event_record* new_record = (struct profiler_event_record*)malloc2(sizeof(struct profiler_event_record));
        printf("sglthing: adding new profiler record for %s (%p)\n", old_event->name, new_record);
        if(!g_hash_table_insert(global_profiler.event_status, proc_name, new_record))
            printf("sglthing: record already exists\n");
        event_data = new_record;
        new_record->id = record_id_last++;
        new_record->number_called = 0;
        new_record->total_time = 0;
    }
    if(event_data)
    {
        double elapsed = g_timer_elapsed(global_profiler.timer, NULL) - old_event->start_time;
        struct profiler_event_record* record = (struct profiler_event_record*)event_data;
        record->number_called++;
        record->number_called_last_second++;
        record->total_time += elapsed;
        record->total_time_last_second += elapsed;
        record->last_time = elapsed;
    }

    // printf("event %s lasted %f seconds\n", old_event->name, g_timer_elapsed(global_profiler.timer, NULL) - old_event->start_time);

    free2(old_event);
#endif
}

static void __profiler_ui_entry(gpointer key, gpointer value, gpointer user)
{
#ifdef PROFILER_ENABLE
    char* process = (char*)key;
    struct profiler_event_record* record = (struct profiler_event_record*)value;
    struct world* world = (struct world*)user;

    if(record)
    {
        char process_name[128];
        snprintf(process_name, 128, "%s - %0.2f/s, %0.2fs, %0.8fs avg, %f", process, record->number_called / world->time, record->total_time, record->total_time / record->number_called, record->last_time);
        ui_draw_text(world->ui, 8.f, 32.f + (record->id * 16.f), process_name, 0.2f);
    }
#endif
}

void profiler_ui(void* _world)
{
#ifdef PROFILER_ENABLE
    profiler_event("profiler_ui");
    struct world* world = (struct world*)_world;
    ui_draw_panel(world->ui, 8.f, world->gfx.screen_height - 8.f, world->gfx.screen_width - 16.f, world->gfx.screen_height - 16.f, 0.4f);

    char profiler_ui_title[64];
    snprintf(profiler_ui_title, 64, "Profiler (%i event types)", g_hash_table_size(global_profiler.event_status));
    ui_draw_text(world->ui, 8.f, 16.f, profiler_ui_title, 0.2f);

    g_hash_table_foreach(global_profiler.event_status, __profiler_ui_entry, world);

    ui_end_panel(world->ui);
    profiler_end();
#endif
}

void profiler_dump()
{
#ifdef PROFILER_ENABLE
    struct profiler_event* event = global_profiler.current_event;
    while(event != 0)
    {
        printf("sglthing: prof %s (%i)\n", event->name, event->id);
        event = event->parent_event;
    }
#endif
}