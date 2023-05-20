#include "prof.h"
#include <glib-2.0/glib.h>
#include "ui.h"
#include "world.h"

struct profiler_event
{
    struct profiler_event* parent_event;
    char name[64];
    double start_time;
};

static int event_id_last = 0;

struct profiler_event_record
{
    int id;
    uint64_t number_called;
    uint64_t number_called_last_second;
    double total_time;
    double total_time_last_second;
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
}

void profiler_end_frame()
{
    
}

void profiler_event(char* name)
{
    struct profiler_event* new_event = (struct profiler_event*)malloc(sizeof(struct profiler_event));
    new_event->start_time = g_timer_elapsed(global_profiler.timer, NULL);

    strncpy(new_event->name, name, 64);
    new_event->parent_event = global_profiler.current_event;
    global_profiler.current_event = new_event;
}

void profiler_end()
{
    if(!global_profiler.current_event)
        return;

    struct profiler_event* old_event = global_profiler.current_event;
    global_profiler.current_event = global_profiler.current_event->parent_event;

    gpointer key, event_data;
    g_hash_table_lookup_extended(global_profiler.event_status, old_event->name, &key, &event_data);    
    if(!key)
    {
        char* proc_name = (char*)malloc(64);
        strncpy(proc_name, old_event->name, 64);
        struct profiler_event_record* new_record = (struct profiler_event_record*)malloc(sizeof(struct profiler_event_record));
        printf("sglthing: adding new profiler record for %s (%p)\n", old_event->name, new_record);
        if(!g_hash_table_insert(global_profiler.event_status, proc_name, new_record))
            printf("sglthing: record already exists\n");
        event_data = new_record;
        new_record->id = event_id_last++;
        new_record->number_called = 0;
        new_record->total_time = 0;
    }
    if(event_data)
    {
        struct profiler_event_record* record = (struct profiler_event_record*)event_data;
        record->number_called++;
        record->number_called_last_second++;
        record->total_time += g_timer_elapsed(global_profiler.timer, NULL) - old_event->start_time;
        record->total_time_last_second += g_timer_elapsed(global_profiler.timer, NULL) - old_event->start_time;
    }

    // printf("event %s lasted %f seconds\n", old_event->name, g_timer_elapsed(global_profiler.timer, NULL) - old_event->start_time);



    free(old_event);
}

static void __profiler_ui_entry(gpointer key, gpointer value, gpointer user)
{
    char* process = (char*)key;
    struct profiler_event_record* record = (struct profiler_event_record*)value;
    struct world* world = (struct world*)user;

    if(record)
    {
        char process_name[128];
        snprintf(process_name, 128, "%s - %0.2f/s, %0.2fs, %0.8fs avg", process, record->number_called / world->time, record->total_time, record->total_time / record->number_called);
        ui_draw_text(world->ui, 8.f, 32.f + (record->id * 16.f), process_name, 0.2f);
    }
}

void profiler_ui(void* _world)
{
    profiler_event("profiler_ui");
    struct world* world = (struct world*)_world;
    ui_draw_panel(world->ui, 8.f, world->gfx.screen_height - 8.f, world->gfx.screen_width - 16.f, world->gfx.screen_height - 16.f, 0.4f);

    char profiler_ui_title[64];
    snprintf(profiler_ui_title, 64, "Profiler (%i event types)", g_hash_table_size(global_profiler.event_status));
    ui_draw_text(world->ui, 8.f, 16.f, profiler_ui_title, 0.2f);

    g_hash_table_foreach(global_profiler.event_status, __profiler_ui_entry, world);

    ui_end_panel(world->ui);
    profiler_end();
}