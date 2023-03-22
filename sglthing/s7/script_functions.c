#include "script_functions.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "transform.h"
#include "netbundle.h"
#include "script_networking.h"

#include "../sglthing.h"

static s7_pointer __engine_print(s7_scheme *sc, s7_pointer args)
{
    if(!s7_is_string(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "engine-print", 1, s7_car(args), "a string"));
    const char* str = s7_string(s7_car(args));

    printf("sglthing: (engine-print '%s')\n", str);
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
    return s7_make_integer(sc, get_texture((char*)str));
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

static s7_pointer __world_time(s7_scheme *sc, s7_pointer args)
{
    return s7_make_real(sc, glfwGetTime());
}

static s7_pointer __world_delta_time(s7_scheme *sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "world-delta-time", 0, s7_car(args), "world pointer"));
    struct world* world = (struct world*)s7_c_pointer(s7_car(args));

    return s7_make_real(sc, world->delta_time);
}

static s7_pointer __world_lighting_shader(s7_scheme *sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "world-lighting-shader", 0, s7_car(args), "world pointer"));
    struct world* world = (struct world*)s7_c_pointer(s7_car(args));

    return s7_make_integer(sc, world->gfx.lighting_shader);
}

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

    mat4 model_matrix;
    glm_mat4_identity(model_matrix);
    glm_mul(model_matrix, transform->translation_matrix, model_matrix);
    glm_mul(model_matrix, transform->rotation_matrix, model_matrix);
    glm_mul(model_matrix, transform->scale_matrix, model_matrix);

    world_draw_model(world, model, program, model_matrix, true);

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

    mat4 model_matrix;
    glm_mat4_identity(model_matrix);
    glm_mul(model_matrix, transform->translation_matrix, model_matrix);
    glm_mul(model_matrix, transform->rotation_matrix, model_matrix);
    glm_mul(model_matrix, transform->scale_matrix, model_matrix);

    world_draw_primitive(world, program, GL_FILL, type, model_matrix, (vec4){1.f,1.f,1.f,1.f});

    return s7_nil(sc);
}

#include "../io.h"

static s7_pointer __io_add_directory(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_string(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "io-add-directory", 0, s7_car(args), "path"));
    const char* str = s7_string(s7_car(args));
    fs_add_directory(str);
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

static s7_pointer __gl_bind_texture(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_integer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "gl-bind-texture", 0, s7_car(args), "texture slot"));
    int texture_slot = s7_integer(s7_car(args));

    if(!s7_is_integer(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "gl-bind-texture", 1, s7_cadr(args), "texture id"));
    int texture_id = s7_integer(s7_cadr(args));

    //tbd
    //if(!s7_is_integer(s7_cadr(args)))
    //    return(s7_wrong_type_arg_error(sc, "gl-bind-texture", 1, s7_cadr(args), "sampler name"));
    //const char* sampler_name = s7_string(s7_cadr(args));

    glActiveTexture(GL_TEXTURE0 + texture_slot);
    glBindTexture(GL_TEXTURE_2D, texture_id);
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

#include "../keyboard.h"

static s7_pointer __input_add_axis(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_string(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "input-get-axis", 0, s7_car(args), "axis name"));
    const char* axis = s7_string(s7_car(args));
    if(!s7_is_integer(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "input-get-axis", 1, s7_cadr(args), "positive key"));
    int positive_key = s7_integer(s7_car(args));
    if(!s7_is_integer(s7_caddr(args)))
        return(s7_wrong_type_arg_error(sc, "input-get-axis", 2, s7_caddr(args), "negative key"));
    int negative_key = s7_integer(s7_cadr(args));
    struct keyboard_mapping k;
    strncpy(k.name,axis,16);
    k.key_positive = positive_key;
    k.key_negative = negative_key;
    add_input(k);
    return s7_nil(sc);
}

static s7_pointer __input_get_axis(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_string(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "input-get-axis", 0, s7_car(args), "axis name"));
    const char* axis = s7_string(s7_car(args));
    return s7_make_real(sc, get_input((char*)axis));
}

static s7_pointer __input_get_key(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_integer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "input-get-key", 0, s7_car(args), "key glfw id"));
    int key = s7_integer(s7_car(args));
    return s7_make_boolean(sc, keys_down[key]);
}

static s7_pointer __input_get_mouse(s7_scheme* sc, s7_pointer args)
{
    return s7_cons(sc, s7_make_real(sc, mouse_position[0]), s7_cons(sc, s7_make_real(sc, mouse_position[1]), s7_nil(sc)));
}

static s7_pointer __input_get_focus(s7_scheme* sc, s7_pointer args)
{
    return s7_make_boolean(sc, get_focus());
}

#include <ode/ode.h>

static s7_pointer __physics_set_transform(s7_scheme* sc, s7_pointer args)
{

}

static s7_pointer __physics_create_body(s7_scheme* sc, s7_pointer args)
{

}

static s7_pointer __physics_create_geom(s7_scheme* sc, s7_pointer args)
{

}

#include "../animator.h"

// TODO: finish

static s7_pointer __animator_set_uniforms(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "animator-set-uniforms", 0, s7_car(args), "animator"));
    struct animator* animator = (struct animator*)s7_c_pointer(s7_car(args));

    if(!s7_is_integer(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "animator-set-uniforms", 1, s7_car(args), "shader program"));
    int shader_program = s7_integer(s7_cadr(args));

    animator_set_bone_uniform_matrices(animator, shader_program);
    return s7_nil(sc);
}

static s7_pointer __animator_set_animation(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "animator-set-animation", 0, s7_car(args), "animator"));
    struct animator* animator = (struct animator*)s7_c_pointer(s7_car(args));

    if(!s7_is_c_pointer(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "animator-set-animation", 1, s7_car(args), "animation"));
    struct animation* animation = (struct animation*)s7_c_pointer(s7_cadr(args));

    animator_set_animation(animator, animation);
    return s7_nil(sc);
}

static s7_pointer __animator_current_time(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "animator-current-time", 0, s7_car(args), "animator"));
    struct animator* animator = (struct animator*)s7_c_pointer(s7_car(args));

    return s7_make_real(sc, animator->current_time / animator->animation->ticks_per_second);
}

static s7_pointer __animator_update(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "animator-update", 0, s7_car(args), "animator"));
    struct animator* animator = (struct animator*)s7_c_pointer(s7_car(args));

    if(!s7_is_real(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "animator-update", 1, s7_car(args), "delta time"));
    float delta_time = s7_real(s7_cadr(args));

    animator_update(animator, delta_time);
}

static s7_pointer __animator_create(s7_scheme* sc, s7_pointer args)
{
    struct animator* animator = (struct animator*)malloc(sizeof(struct animator));
    animator_create(animator);
    return s7_make_c_pointer(sc, animator);
}

static s7_pointer __animation_create(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_string(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "animation-create", 0, s7_car(args), "animation path"));
    if(!s7_is_c_pointer(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "animation-create", 1, s7_cadr(args), "model"));
    struct model* model = s7_c_pointer(s7_cadr(args));
    struct animation* animation = (struct animation*)malloc(sizeof(struct animation));
    int animation_id = 0;
    if(s7_is_integer(s7_caddr(args)))
        animation_id = s7_integer(s7_caddr(args));
    animation_create((char*)s7_string(s7_car(args)), &model->meshes[0], animation_id, animation);
    return s7_make_c_pointer(sc, animation);
}

static s7_pointer __animation_bundle_create(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_string(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "animation-bundle-create", 0, s7_car(args), "animation path"));
    if(!s7_is_c_pointer(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "animation-bundle-create", 1, s7_cadr(args), "model"));
    struct model* model = s7_c_pointer(s7_cadr(args));
    struct animation_bundle* animation = (struct animation_bundle*)malloc(sizeof(struct animation_bundle));
    animation_bundle_create((char*)s7_string(s7_car(args)), &model->meshes[0], animation);
    return s7_make_c_pointer(sc, animation);
}

static s7_pointer __animation_bundle_get(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "animation-bundle-get", 0, s7_car(args), "animation bundle"));
    if(!s7_is_integer(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "animation-bundle-get", 1, s7_cadr(args), "id"));
    struct animation_bundle* bundle = (struct animation_bundle*)s7_c_pointer(s7_car(args));
    int animation_id = s7_integer(s7_cadr(args));
    return s7_make_c_pointer(sc, animation_bundle_get(bundle, animation_id));
}

#include "../light.h"

static s7_pointer __lightarea_create(s7_scheme* sc, s7_pointer args)
{
    return s7_make_c_pointer(sc, light_create_area());
}

static s7_pointer __lightarea_update(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "lightarea-update", 1, s7_car(args), "lightarea"));
    struct light_area* lightarea = s7_c_pointer(s7_car(args));

    if(!s7_is_real(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "lightarea-update", 1, s7_cadr(args), "position X"));
    float position_x = s7_real(s7_cadr(args));

    if(!s7_is_real(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "lightarea-update", 1, s7_caddr(args), "position Y"));
    float position_y = s7_real(s7_caddr(args));

    if(!s7_is_real(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "lightarea-update", 1, s7_cadddr(args), "position Z"));
    float position_z = s7_real(s7_cadddr(args));

    light_update(lightarea,(vec3){position_x,position_y,position_z});

    return s7_nil(sc);
}

static s7_pointer __lightarea_use(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "lightarea-update", 1, s7_car(args), "lightarea"));
    struct light_area* lightarea = s7_c_pointer(s7_car(args));

    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "lightarea-update", 1, s7_cadr(args), "world"));
    struct world* world = s7_c_pointer(s7_cadr(args));

    world->render_area = lightarea;

    return s7_nil(sc);
}

static s7_pointer __light_create(s7_scheme* sc, s7_pointer args)
{
    struct light* light = malloc(sizeof(struct light));
    light_add(light);
    return s7_make_c_pointer(sc, light);
}

static s7_pointer __light_set_position(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-position", 1, s7_car(args), "light"));
    struct light* light = (struct light*)s7_c_pointer(s7_car(args));

    if(!s7_is_real(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-position", 1, s7_cadr(args), "position X"));
    float position_x = s7_real(s7_cadr(args));

    if(!s7_is_real(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-position", 1, s7_caddr(args), "position Y"));
    float position_y = s7_real(s7_caddr(args));

    if(!s7_is_real(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-position", 1, s7_cadddr(args), "position Z"));
    float position_z = s7_real(s7_cadddr(args));

    light->position[0] = position_x;
    light->position[1] = position_y;
    light->position[2] = position_z;

    return s7_nil(sc);
}

static s7_pointer __light_set_clq(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-clq", 1, s7_car(args), "light"));
    struct light* light = (struct light*)s7_c_pointer(s7_car(args));

    if(!s7_is_real(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-clq", 2, s7_cadr(args), "constant"));
    float constant = s7_real(s7_cadr(args));

    if(!s7_is_real(s7_caddr(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-clq", 3, s7_caddr(args), "linear"));
    float linear = s7_real(s7_caddr(args));

    if(!s7_is_real(s7_cadddr(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-clq", 4, s7_cadddr(args), "quadratic"));
    float quadratic = s7_real(s7_cadddr(args));

    light->constant = constant;
    light->linear = linear;
    light->quadratic = quadratic;
    light->intensity = 1.0;

    return s7_nil(sc);
}

static s7_pointer __light_set_ambient(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-ambient", 1, s7_car(args), "light"));
    struct light* light = (struct light*)s7_c_pointer(s7_car(args));

    if(!s7_is_real(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-ambient", 2, s7_cadr(args), "red"));
    float r = s7_real(s7_cadr(args));

    if(!s7_is_real(s7_caddr(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-ambient", 3, s7_caddr(args), "green"));
    float g = s7_real(s7_caddr(args));

    if(!s7_is_real(s7_cadddr(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-ambient", 4, s7_cadddr(args), "blue"));
    float b = s7_real(s7_cadddr(args));

    light->ambient[0] = r;
    light->ambient[1] = g;
    light->ambient[2] = b;

    return s7_nil(sc);
}

static s7_pointer __light_set_diffuse(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-diffuse", 1, s7_car(args), "light"));
    struct light* light = (struct light*)s7_c_pointer(s7_car(args));

    if(!s7_is_real(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-diffuse", 2, s7_cadr(args), "red"));
    float r = s7_real(s7_cadr(args));

    if(!s7_is_real(s7_caddr(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-diffuse", 3, s7_caddr(args), "green"));
    float g = s7_real(s7_caddr(args));

    if(!s7_is_real(s7_cadddr(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-diffuse", 4, s7_cadddr(args), "blue"));
    float b = s7_real(s7_cadddr(args));

    light->diffuse[0] = r;
    light->diffuse[1] = g;
    light->diffuse[2] = b;

    return s7_nil(sc);
}

static s7_pointer __light_set_specular(s7_scheme* sc, s7_pointer args)
{
    if(!s7_is_c_pointer(s7_car(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-specular", 1, s7_car(args), "light"));
    struct light* light = (struct light*)s7_c_pointer(s7_car(args));

    if(!s7_is_real(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-specular", 2, s7_cadr(args), "red"));
    float r = s7_real(s7_cadr(args));

    if(!s7_is_real(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-specular", 3, s7_caddr(args), "green"));
    float g = s7_real(s7_caddr(args));

    if(!s7_is_real(s7_cadr(args)))
        return(s7_wrong_type_arg_error(sc, "light-set-specular", 4, s7_cadddr(args), "blue"));
    float b = s7_real(s7_cadddr(args));

    light->specular[0] = r;
    light->specular[1] = g;
    light->specular[2] = b;

    return s7_nil(sc);
}

void sgls7_add_functions(s7_scheme* sc)
{
    sgls7_transform_register(sc);
    sgls7_registernetworking(sc);


    s7_define_variable(sc, "GL_FRAGMENT_SHADER", s7_make_integer(sc, GL_FRAGMENT_SHADER));
    s7_define_variable(sc, "GL_VERTEX_SHADER", s7_make_integer(sc, GL_VERTEX_SHADER));
    s7_define_variable(sc, "math-pi", s7_make_real(sc, M_PIf));
    s7_define_variable(sc, "math-pi-2", s7_make_real(sc, M_PI_2f));
    s7_define_variable(sc, "math-pi-180", s7_make_real(sc, M_PI_180f));
    s7_define_variable(sc, "nil", s7_nil(sc));
    s7_define_variable(sc, "nullptr", s7_make_c_pointer(sc, NULL));

    s7_define_function(sc, "engine-print", __engine_print, 1, 0, false, "(engine-print string) prints string to log");

    s7_define_function(sc, "world-draw-object", __render_object, 4, 0, false, "(world-render-object w p m t)");
    s7_define_function(sc, "world-draw-primitive", __render_primitive, 4, 0, false, "(world-render-object w p m t)");
    s7_define_function(sc, "world-get-ui", __world_get_ui, 1, 0, false, "(world-get-ui w) returns ui data pointer");
    s7_define_function(sc, "world-time", __world_time, 0, 0, false, "(world-time) gets time of glfw");
    s7_define_function(sc, "world-delta-time", __world_delta_time, 1, 0, false, "(world-delta-time w) gets delta time of glfw window");
    s7_define_function(sc, "world-lighting-shader", __world_lighting_shader, 1, 0, false, "(world-lighting-shader w) gets the worlds shadow pass shader");

    s7_define_function(sc, "io-add-directory", __io_add_directory, 1, 0, false, "(io-add-directory d)");

    s7_define_function(sc, "gl-no-depth", __gl_no_depth, 0, 0, false, "(gl-no-depth) disables depth testing");
    s7_define_function(sc, "gl-yes-depth", __gl_yes_depth, 0, 0, false, "(gl-yes-depth) enables depth testing");
    s7_define_function(sc, "gl-bind-texture", __gl_bind_texture, 2, 0, false, "(gl-bind-texture s t) sets texture slot s to texture id t");

    s7_define_function(sc, "ui-draw-text", __ui_draw_text, 4, 0, false, "(ui-draw-text u x y t)");

    s7_define_function(sc, "input-add-axis", __input_add_axis, 3, 0, false, "(input-get-mouse s p n) creates axis named s using positive glfw key p and negative glfw key n");
    s7_define_function(sc, "input-get-mouse", __input_get_mouse, 0, 0, false, "(input-get-mouse) returns list, x = 1st element, y = 2nd element");
    s7_define_function(sc, "input-get-axis", __input_get_axis, 1, 0, false, "(input-get-axis a) returns real num");
    s7_define_function(sc, "input-get-key", __input_get_key, 1, 0, false, "(input-get-mouse k) returns #t/#f");
    s7_define_function(sc, "input-get-focus", __input_get_focus, 0, 0, false, "(input-get-focus) returns #t/#f");

    s7_define_function(sc, "load-texture", __load_texture, 1, 0, false, "(load-model string) loads texture string into texture cache");
    s7_define_function(sc, "get-texture", __get_texture, 1, 0, false, "(get-model string) returns texture string's id");

    s7_define_function(sc, "load-model", __load_model, 1, 0, false, "(load-model string) loads model string into model cache");
    s7_define_function(sc, "get-model", __get_model, 1, 0, false, "(get-model string) returns model string's pointer");

    s7_define_function(sc, "compile-shader", __compile_shader, 2, 0, false, "(compile-shader string type) compile shader");
    s7_define_function(sc, "link-program", __link_program, 2, 0, false, "(link-program v f) links 2 shaders together and returns a program");

    s7_define_function(sc, "animator-create", __animator_create, 0, 0, false, "(animator-create)");
    s7_define_function(sc, "animator-set-animation", __animator_set_animation, 2, 0, false, "(animator-set-animation a A)");
    s7_define_function(sc, "animator-set-uniforms", __animator_set_uniforms, 2, 0, false, "(animator-set-uniforms a p)");
    s7_define_function(sc, "animator-update", __animator_update, 2, 0, false, "(animator-update a d)");
    s7_define_function(sc, "animator-current-time", __animator_current_time, 1, 0, false, "(animator-current-time a)");
    
    // TODO: these
    // s7_define_function(sc, "animator-set-speed", __animator_set_speed, 2, 0, false, "(animator-set-speed a s)");
    // s7_define_function(sc, "animator-set-tick", __animator_set_tick, 2, 0, false, "(animator-get-tick a t)");
    // s7_define_function(sc, "animator-get-tick", __animator_get_tick, 1, 0, false, "(animator-get-tick a)");

    s7_define_function(sc, "animation-create", __animation_create, 2, 1, false, "(animation-create f m i)");

    s7_define_function(sc, "animation-bundle-create", __animation_bundle_create, 2, 0, false, "(animation-bundle-create f m)");   
    s7_define_function(sc, "animation-bundle-get", __animation_bundle_get, 2, 0, false, "(animation-bundle-get b i)");   
    
    s7_define_function(sc, "lightarea-create", __lightarea_create, 0, 0, false, "(lightarea-create)");
    s7_define_function(sc, "lightarea-update", __lightarea_update, 4, 0, false, "(lightarea-update a x y z)");
    s7_define_function(sc, "lightarea-use", __lightarea_use, 2, 0, false, "(lightarea-use a w)");

    s7_define_function(sc, "light-create", __light_create, 0, 0, false, "(light-create)");
    s7_define_function(sc, "light-set-clq", __light_set_clq, 4, 0, false, "(light-set-clq a c l q)");
    s7_define_function(sc, "light-set-position", __light_set_position, 4, 0, false, "(light-set-position a x y z)");
    s7_define_function(sc, "light-set-ambient", __light_set_ambient, 4, 0, false, "(light-set-position a r g b)");
    s7_define_function(sc, "light-set-diffuse", __light_set_diffuse, 4, 0, false, "(light-set-position a r g b)");
    s7_define_function(sc, "light-set-specular", __light_set_specular, 4, 0, false, "(light-set-position a r g b)");
}