#include "shader.h"
#include "graphic.h"
#include <stdio.h>
#include <string.h>
#include <glad/glad.h>

int compile_shader(const char* shader_name, int type)
{
    void* shader_data = malloc(65535);
    memset(shader_data, 0, 65535);
    FILE* shader_file = fopen(shader_name, "r");
    if(shader_file)
    {       
        int data_read = fread(shader_data, 1, 65535, shader_file);
        fclose(shader_file);

        int shader_id = glCreateShader(type);
        int success;
        sglc(glShaderSource(shader_id, 1, &shader_data, &data_read));        
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
        printf("sglthing: compiled shader %i %s\n", shader_id, shader_name);

        return shader_id;
    }
    else
    {
        printf("sglthing: couldn't find shader '%s'\n", shader_name);
        return 0;
    }
}

int link_program(int vertex, int fragment)
{
    int program = glCreateProgram();
    int success;
    sglc(glAttachShader(program, vertex));
    sglc(glAttachShader(program, fragment));
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
