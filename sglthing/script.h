#ifndef SCRIPT_H
#define SCRIPT_H
#include <stdbool.h>

struct script_system;

struct script_system* script_init(char* file, void* world);
void script_frame(struct script_system* system);
void script_frame_render(struct script_system* system, bool xtra_pass);
void script_frame_ui(struct script_system* system);
void* script_s7(struct script_system* system);
bool script_enabled(struct script_system* system);

#endif