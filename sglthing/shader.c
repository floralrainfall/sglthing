#include "shader.h"
#include "graphic.h"
#include "io.h"
#include <stdio.h>
#include <string.h>
#include <glad/glad.h>

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

        int shader_id = glCreateShader(type);
        int success;
        char *shader_sources[] = {c_shader_data, shader_data};
        sglc(glShaderSource(shader_id, 2, shader_sources, NULL));        
        printf("sglthing: compiling shader  %s\n", shader_name);
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
        printf("sglthing: compiled shader %i %s\n", shader_id, shader_name);

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

int link_program(int vertex, int fragment)
{
    int program = glCreateProgram();
    int success;
    if(vertex)
    {
        sglc(glAttachShader(program,vertex));
    }
    else
        printf("sglthing: no vertex shader set\n");
    if(fragment)
    {
        sglc(glAttachShader(program,fragment));
    }
    else
        printf("sglthing: no fragment shader set\n");
    sglc(glLinkProgram(program));

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success) {
        char info_log[512];
        glGetProgramInfoLog(program, 512, NULL, info_log);
        printf("sglthing: program compilation failed\n%s\n",info_log);
        return 0;
    }
    printf("sglthing: linked program %i; v: %i, f: %i\n", program, vertex, fragment);
    return program;
}
