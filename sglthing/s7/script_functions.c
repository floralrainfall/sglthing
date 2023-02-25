#include "script_functions.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "transform.h"

#include "../sglthing.h"

static s7_pointer __world_time(s7_scheme *sc, s7_pointer args)
{
    return s7_make_real(sc, glfwGetTime());
}

static s7_pointer __engine_print(s7_scheme *sc, s7_pointer args)
{
    if(!s7_is_string(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "engine-print", 1, s7_car(args), "a string"));
    const char* str = s7_string(s7_car(args));

    printf("s7: '%s'\n", str);
    return s7_nil(sc);
}

#include "../model.h"

static s7_pointer __load_model(s7_scheme *sc, s7_pointer args)
{
    if(!s7_is_string(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "load-model", 0, s7_car(args), "a model name"));
    const char* str = s7_string(s7_car(args));

    load_model((char*)str);
    return s7_nil(sc);
}

static s7_pointer __get_model(s7_scheme *sc, s7_pointer args)
{
    if(!s7_is_string(s7_car(args)))
        return (s7_wrong_type_arg_error(sc, "get-model", 0, s7_car(args), "a model name"));

    const char* str = s7_string(s7_car(args));
    return s7_make_c_pointer(sc, get_model(str));
}

#include "../texture.h"

static s7_pointer __load_texture(s7_scheme *sc, s7_pointer args)
{
    if(!s7_is_string(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "load-texture", 0, s7_car(args), "a texture name"));

    const char* str = s7_string(s7_car(args));
    load_texture((char*)str);
    return s7_nil(sc);
}

static s7_pointer __get_texture(s7_scheme *sc, s7_pointer args)
{
    if(!s7_is_string(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "get-texture", 0, s7_car(args), "a texture name"));

    const char* str = s7_string(s7_car(args));
    return s7_make_integer(sc, get_texture(str));
}

#include "../shader.h"

static s7_pointer __compile_shader(s7_scheme *sc, s7_pointer args)
{
    if(!s7_is_string(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "compile-shader", 0, s7_car(args), "a shader path"));
    const char* str = s7_string(s7_car(args));

    if(!s7_is_integer(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "compile-shader", 1, s7_cadr(args), "a shader id"));
    s7_int type = s7_integer(s7_cadr(args));

    return s7_make_integer(sc, compile_shader((char*)str,(int)type));
}

static s7_pointer __link_program(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_integer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "compile-shader", 0, s7_car(args), "vertex shader id"));
    s7_int shader_a = s7_integer(s7_car(args));
        
    if(!s7_is_integer(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "compile-shader", 1, s7_cadr(args), "a shader id"));
    s7_int shader_b = s7_integer(s7_cadr(args));

    return s7_make_integer(sc, link_program((int)shader_a,(int)shader_b));
}

#include "../world.h"
#include "../primitives.h"

static s7_pointer __draw_vao(s7_scheme* sc, s7_pointer args)
{

}

static s7_pointer __render_object(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "world-render-object", 0, s7_car(args), "world pointer"));
    struct world* world = (struct world*)s7_c_pointer(s7_car(args));

    if(!s7_is_integer(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "world-render-object", 1, s7_cadr(args), "shader program"));
    int program = s7_integer(s7_cadr(args));

    if(!s7_is_c_pointer(s7_caddr(args)))
        return(s7_wrong_type_arg_error(sc, "world-render-object", 2, s7_cadr(args), "model"));
    struct model* model = (struct model*)s7_c_pointer(s7_caddr(args));

    if(!s7_is_c_object(s7_cadddr(args)))
        return(s7_wrong_type_arg_error(sc, "world-render-primitive", 3, s7_cadddr(args), "transform"));
    struct transform* transform = (struct transform*)s7_c_object_value(s7_cadddr(args));

    world_draw_model(world, model, program, transform->transform_matrix, true);

    return s7_nil(sc);
}

static s7_pointer __render_primitive(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "world-render-primitive", 0, s7_car(args), "world pointer"));
    struct world* world = (struct world*)s7_c_pointer(s7_car(args));

    if(!s7_is_integer(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "world-render-primitive", 1, s7_cadr(args), "shader program"));
    int program = s7_integer(s7_cadr(args));

    if(!s7_is_integer(s7_caddr(args)))
        return(s7_wrong_type_arg_error(sc, "world-render-primitive", 2, s7_caddr(args), "primitive type"));
    int type = s7_integer(s7_caddr(args));

    if(!s7_is_c_object(s7_cadddr(args)))
        return(s7_wrong_type_arg_error(sc, "world-render-primitive", 3, s7_cadddr(args), "transform"));
    struct transform* transform = (struct transform*)s7_c_object_value(s7_cadddr(args));

    world_draw_primitive(world, program, GL_FILL, type, transform->transform_matrix, (vec4){1.f,1.f,1.f,1.f});

    return s7_nil(sc);
}

#include <glad/glad.h>

static s7_pointer __gl_no_depth(s7_scheme* sc, s7_pointer args)
{
    glDisable(GL_DEPTH_TEST);
    return s7_nil(sc);
}

static s7_pointer __gl_yes_depth(s7_scheme* sc, s7_pointer args)
{
    glEnable(GL_DEPTH_TEST);
    return s7_nil(sc);
}

#include "../ui.h"

static s7_pointer __world_get_ui(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "world-get-ui", 0, s7_car(args), "world pointer"));
    struct world* world = (struct world*)s7_c_pointer(s7_car(args));
    return s7_make_c_pointer(sc, world->ui);
}

static s7_pointer __ui_draw_text(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "ui-draw-text", 0, s7_car(args), "ui data pointer"));
    struct ui_data* ui = (struct ui_data*)s7_c_pointer(s7_car(args));
    
    if(!s7_is_real(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "ui-draw-text", 1, s7_car(args), "position x"));
    float x_position = s7_real(s7_cadr(args));

    if(!s7_is_real(s7_caddr(args)))
        return(s7_wrong_type_arg_error(sc, "ui-draw-text", 2, s7_car(args), "position y"));
    float y_position = s7_real(s7_caddr(args));

    if(!s7_is_string(s7_cadddr(args)))
        return(s7_wrong_type_arg_error(sc, "ui-draw-text", 3, s7_car(args), "text"));
    const char* text = s7_string(s7_cadddr(args));

    ui_draw_text(ui, x_position, y_position, (char*)text, 1.0f);
}

void sgls7_add_functions(s7_scheme* sc)
{
    sgls7_transform_register(sc);

    s7_define_variable(sc, "GL_FRAGMENT_SHADER", s7_make_integer(sc, GL_FRAGMENT_SHADER));
    s7_define_variable(sc, "GL_VERTEX_SHADER", s7_make_integer(sc, GL_VERTEX_SHADER));
    s7_define_variable(sc, "math-pi", s7_make_real(sc, M_PIf));
    s7_define_variable(sc, "math-pi-2", s7_make_real(sc, M_PI_2f));
    s7_define_variable(sc, "math-pi-180", s7_make_real(sc, M_PI_180f));
    s7_define_variable(sc, "nil", s7_nil(sc));

    s7_define_function(sc, "engine-print", __engine_print, 1, 0, false, "(engine-print string) prints string to log");
    s7_define_function(sc, "world-time", __world_time, 0, 0, false, "(world-time) gets time of glfw window");

    s7_define_function(sc, "compile-shader", __compile_shader, 2, 0, false, "(compile-shader string type) compile shader");
    s7_define_function(sc, "link-program", __link_program, 2, 0, false, "(link-program v f) links 2 shaders together and returns a program");
    s7_define_function(sc, "world-draw-object", __render_object, 4, 0, false, "(world-render-object w p m t)");
    s7_define_function(sc, "world-draw-primitive", __render_primitive, 4, 0, false, "(world-render-object w p m t)");
    s7_define_function(sc, "world-get-ui", __world_get_ui, 1, 0, false, "(world-get-ui w) returns ui data pointer");

    s7_define_function(sc, "gl-no-depth", __gl_no_depth, 0, 0, false, "(gl-no-depth) disables depth testing");
    s7_define_function(sc, "gl-yes-depth", __gl_yes_depth, 0, 0, false, "(gl-yes-depth) enables depth testing");

    s7_define_function(sc, "ui-draw-text", __ui_draw_text, 4, 0, false, "(ui-draw-text u x y t)");

    s7_define_function(sc, "load-texture", __load_texture, 1, 0, false, "(load-model string) loads texture string into texture cache");
    s7_define_function(sc, "get-texture", __get_texture, 1, 0, false, "(get-model string) returns texture string's id");

    s7_define_function(sc, "load-model", __load_model, 1, 0, false, "(load-model string) loads model string into model cache");
    s7_define_function(sc, "get-model", __get_model, 1, 0, false, "(get-model string) returns model string's pointer");
}