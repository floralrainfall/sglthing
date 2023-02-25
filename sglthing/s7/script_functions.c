#include "script_functions.h"
#include <glad/glad.h>

#include "../model.h"

static s7_pointer __load_model(s7_scheme *sc, s7_pointer args)
{
    if(s7_is_string(s7_car(args)))
    {
        const char* str = s7_string(s7_car(args));
        load_model((char*)str);
        return s7_nil(sc);
    }
    return(s7_wrong_type_arg_error(sc, "load-model", 0, s7_car(args), "a model name"));
}

static s7_pointer __get_model(s7_scheme *sc, s7_pointer args)
{
    if(s7_is_string(s7_car(args)))
    {
        const char* str = s7_string(s7_car(args));
        return s7_make_integer(sc, get_model(str));
    }
    return(s7_wrong_type_arg_error(sc, "get-model", 0, s7_car(args), "a model name"));
}

#include "../texture.h"

static s7_pointer __load_texture(s7_scheme *sc, s7_pointer args)
{
    if(s7_is_string(s7_car(args)))
    {
        const char* str = s7_string(s7_car(args));
        load_texture((char*)str);
        return s7_nil(sc);
    }
    return(s7_wrong_type_arg_error(sc, "load-texture", 0, s7_car(args), "a texture name"));
}

static s7_pointer __get_texture(s7_scheme *sc, s7_pointer args)
{
    if(s7_is_string(s7_car(args)))
    {
        const char* str = s7_string(s7_car(args));
        return s7_make_integer(sc, get_texture(str));
    }
    return(s7_wrong_type_arg_error(sc, "get-texture", 0, s7_car(args), "a texture name"));
}

#include "../shader.h"

static s7_pointer __compile_shader(s7_scheme *sc, s7_pointer args)
{
    if(s7_is_string(s7_car(args)))
    {
        const char* str = s7_string(s7_car(args));
        if(s7_is_integer(s7_cadr(args)))
        {
            s7_int type = s7_integer(s7_cadr(args));
            return s7_make_integer(sc, compile_shader((char*)str,(int)type));
        }
        return(s7_wrong_type_arg_error(sc, "compile-shader", 1, s7_cadr(args), "a shader id"));
    }
    return(s7_wrong_type_arg_error(sc, "compile-shader", 0, s7_car(args), "a shader path"));
}

static s7_pointer __link_program(s7_scheme* sc, s7_pointer args)
{
    if(s7_is_integer(s7_car(args)))
    {
        s7_int shader_a = s7_integer(s7_car(args));
        if(s7_is_integer(s7_cadr(args)))
        {
            s7_int shader_b = s7_integer(s7_cadr(args));
            return s7_make_integer(sc, link_program((int)shader_a,(int)shader_b));
        }
        return(s7_wrong_type_arg_error(sc, "compile-shader", 1, s7_cadr(args), "a shader id"));
    }
    return(s7_wrong_type_arg_error(sc, "compile-shader", 0, s7_car(args), "vertex shader id"));
}

void sgls7_add_functions(s7_scheme* sc)
{
    s7_define_function(sc, "compile-shader", __compile_shader, 2, 0, false, "(load-model string) compile shader");
    s7_define_function(sc, "link-program", __link_program, 2, 0, false, "(get-model string) links 2 shaders together and returns a program");

    s7_define_function(sc, "load-texture", __load_texture, 1, 0, false, "(load-model string) loads texture string into texture cache");
    s7_define_function(sc, "get-texture", __get_texture, 1, 0, false, "(get-model string) returns texture string's id");

    s7_define_function(sc, "load-model", __load_model, 1, 0, false, "(load-model string) loads model string into model cache");
    s7_define_function(sc, "get-model", __get_model, 1, 0, false, "(get-model string) returns model string's pointer");
}