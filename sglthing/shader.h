#ifndef SHADER_H
#define SHADER_H
#include "world.h"

int compile_shader(const char* shader_name, int type);
int link_program(int vertex, int fragment);

int create_program();
int attach_program_shader(int program, int shader);
int link_programv(int program);

#endif