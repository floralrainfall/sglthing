#include "shader.h"
#include "graphic.h"
#include "io.h"
#include <stdio.h>
#include <string.h>
#include <glad/glad.h>
#include "sglthing.h"

int compile_shader(const char* shader_name, int type)
{
    void* shader_data = malloc(65535);
    memset(shader_data, 0, 65535);
    void* c_shader_data = malloc(65535);
    memset(c_shader_data, 0, 65535);
    FILE* common_shader_file = file_open("shaders/shared.glsl", "r");
    FILE* shader_file = file_open(shader_name, "r");
    if(shader_file && common_shader_file)
    {       
        int data_read = fread(shader_data, 1, 65535, shader_file);
        fclose(shader_file);
        int common_data_read = fread(c_shader_data, 1, 65535, common_shader_file);
        fclose(common_shader_file);

        printf("sglthing: compiling shader %s %04x\n", shader_name, type);
        int shader_id = glCreateShader(type);
        int error = glGetError();
        if(error)
            printf("sglthing: glCreateShader error: %i\n", error);
        ASSERT(error == 0);
        ASSERT(shader_id);
        int success;
        char *shader_sources[] = {c_shader_data, shader_data};  
        sglc(glShaderSource(shader_id, 2, shader_sources, NULL));      
        sglc(glCompileShader(shader_id));
        sglc(glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success));
        if(!success)
        {
            char info_log[512];
            sglc(glGetShaderInfoLog(shader_id, 512, NULL, info_log));
            printf("sglthing: shader compilation failed\n%s\n",info_log);
            return 0;
        }
        free(shader_data); // having 64K be allocated every time we compile a shader could cause memory leaks
        free(c_shader_data);
        printf("sglthing: compiled shader  %s %i\n", shader_name, shader_id);

        return shader_id;
    }
    else
    {
        free(shader_data); 
        free(c_shader_data);
        printf("sglthing: couldn't find shader '%s'\n", shader_name);
        return 0;
    }
}

int create_program()
{
    int program = glCreateProgram();
    ASSERT(program);
    return program;
}

int attach_program_shader(int program, int shader)
{
    if(shader)
        sglc(glAttachShader(program,shader));
}

int link_programv(int program)
{    
    int success;
    sglc(glLinkProgram(program));

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success) {
        char info_log[512];
        glGetProgramInfoLog(program, 512, NULL, info_log);
        printf("sglthing: program compilation failed\n%s\n",info_log);
    }
    printf("sglthing: linked program %i\n", program);

    return success;
}

int link_program(int vertex, int fragment)
{
    ASSERT(vertex && fragment);
    int program = create_program();
    if(vertex)
    {
        attach_program_shader(program, vertex);
    }
    else
        printf("sglthing: no vertex shader set\n");
    if(fragment)
    {
        attach_program_shader(program, fragment);
    }
    else
        printf("sglthing: no fragment shader set\n");
    link_programv(program);
    printf("sglthing: linked program %i; v: %i, f: %i\n", program, vertex, fragment);
    return program;
}
