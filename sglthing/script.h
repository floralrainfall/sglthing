#ifndef SCRIPT_H
#define SCRIPT_H
#include <neko.h>

#define WORLD_TAG 0x11111

struct script_system;

struct script_system* script_init(char* file);
void script_frame(void* world, struct script_system* system);
void script_frame_render(void* world, struct script_system* system);

#endif