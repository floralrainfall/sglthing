#ifndef PROF_H
#define PROF_H

void profiler_global_init();
void profiler_event(char* name);
void profiler_end();
void profiler_end_frame();
void profiler_dump();
// ughhh
void profiler_ui(void* world);

#endif