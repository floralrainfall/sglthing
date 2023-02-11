#ifndef SHADER_H
#define SHADER_H
#include "world.h"

int compile_shader(const char* shader_name, int type);
int link_program(int vertex, int fragment);

#endif