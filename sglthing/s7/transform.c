#include "transform.h"
#include <stdlib.h>

static int transform_type_tag = 0;

#define CONCAT_(A, B) A ## B
#define CONCAT(A, B) CONCAT_(A, B)

#define SET_TRANSFORM(v)                                                            \
    static s7_pointer transform_##v(s7_scheme* sc, s7_pointer args) {               \
        if(!s7_is_c_object(s7_car(args)))                                               \
            return(s7_wrong_type_arg_error(sc, "transform-" #v, 1, s7_cadr(args), "transform")); \
        struct transform *o = (struct transform*)s7_c_object_value(s7_car(args));   \
        return (s7_make_real(sc, o->v));                                            \
    }                                                                               \
    static s7_pointer set_transform_##v(s7_scheme* sc, s7_pointer args) {           \
        if(!s7_is_c_object(s7_car(args)))                                               \
            return(s7_wrong_type_arg_error(sc, "set-transform-" #v, 1, s7_cadr(args), "transform")); \
        struct transform *o = (struct transform*)s7_c_object_value(s7_car(args));   \
        if(!s7_is_real(s7_cadr(args)))                                               \
            return(s7_wrong_type_arg_error(sc, "set-transform-" #v, 2, s7_cadr(args), "value")); \
        o->v = s7_real(s7_cadr(args));                                              \
        return s7_cadr(args);                                                       \
    };
#define SET_TRANSFORM_TYPE(s,v)                                                                                                                    \
    s7_define_variable(s,"transform-" #v, s7_dilambda(s, "transform-" #v, transform_##v, 1, 0, set_transform_##v, 2, 0, "transform " #v " field"));

static s7_pointer make_transform(s7_scheme *sc, s7_pointer args)
{
    struct transform *o = (struct transform *)malloc(sizeof(struct transform));
    o->px = 0.0;
    o->py = 0.0;
    o->pz = 0.0;
    o->rx = 0.0;
    o->ry = 0.0;
    o->rz = 0.0;
    o->rw = 1.0;
    o->sx = 1.0;
    o->sy = 1.0;
    o->sz = 1.0;
    glm_mat4_identity(o->translation_matrix);
    glm_mat4_identity(o->rotation_matrix);
    glm_mat4_identity(o->scale_matrix);
    return(s7_make_c_object(sc, transform_type_tag, (void *)o));
}

static s7_pointer mark_transform(s7_scheme *sc, s7_pointer obj)
{
    return(NULL);
}

static s7_pointer free_transform(s7_scheme *sc, s7_pointer obj)
{
    free(s7_c_object_value(obj));
    return(NULL);
}

static s7_pointer is_transform(s7_scheme *sc, s7_pointer args)
{
    return(s7_make_boolean(sc, 
                s7_is_c_object(s7_car(args)) &&
                s7_c_object_type(s7_car(args)) == transform_type_tag));
}

static s7_pointer update_transform(s7_scheme *sc, s7_pointer args)
{
    if(s7_is_c_object(s7_car(args)))
    {
        struct transform* transform = s7_c_object_value(s7_car(args));
        transform_to_matrix(transform);
    }
    return NULL;
}

SET_TRANSFORM(px)
SET_TRANSFORM(py)
SET_TRANSFORM(pz)
SET_TRANSFORM(rx)
SET_TRANSFORM(ry)
SET_TRANSFORM(rz)
SET_TRANSFORM(rw)
SET_TRANSFORM(sx)
SET_TRANSFORM(sy)
SET_TRANSFORM(sz)

#define SET_OLD_VERSION(t,v) \
    t->v ## _old = t->v;
#define CHK_OLD(t,v) \
    (t->v ## _old != t->v)

void transform_to_matrix(struct transform* transform)
{
    mat4 transformation;
    glm_mat4_identity(transformation);
    if(CHK_OLD(transform,px) || CHK_OLD(transform,py) || CHK_OLD(transform,pz))
    {
        glm_translate(transformation, (vec3){transform->px,transform->py,transform->pz});
        glm_mat4_copy(transformation, transform->translation_matrix);
        SET_OLD_VERSION(transform,px)
        SET_OLD_VERSION(transform,py)
        SET_OLD_VERSION(transform,pz)
    } 
    if(CHK_OLD(transform, rx) || CHK_OLD(transform, ry) || CHK_OLD(transform, rz) || CHK_OLD(transform, rw))
    {
        double a = cos(transform->rw/2.0);
        double a_sin = sin(transform->rw/2.0);
        double x = a_sin * transform->rx;
        double y = a_sin * transform->ry;
        double z = a_sin * transform->rz;
        glm_rotate(transformation, a, (vec3){x,y,z});
        glm_mat4_copy(transformation, transform->rotation_matrix);
        SET_OLD_VERSION(transform,rx)
        SET_OLD_VERSION(transform,ry)
        SET_OLD_VERSION(transform,rz)
        SET_OLD_VERSION(transform,rw)
    }
    if(CHK_OLD(transform, sx) || CHK_OLD(transform, sy) || CHK_OLD(transform, sz))
    {
        glm_scale(transformation, (vec3){transform->sx,transform->sy,transform->sz});
        glm_mat4_copy(transformation, transform->scale_matrix);
        SET_OLD_VERSION(transform,sx)
        SET_OLD_VERSION(transform,sy)
        SET_OLD_VERSION(transform,sz)
    }
}

void sgls7_transform_register(s7_scheme* sc)
{
    transform_type_tag = s7_make_c_type(sc, "transform");
    s7_c_type_set_gc_free(sc, transform_type_tag, free_transform);
    s7_c_type_set_gc_mark(sc, transform_type_tag, mark_transform);

    s7_define_function(sc, "make-transform", make_transform, 0, 0, false, "(make-transform) makes a new transform");
    s7_define_function(sc, "update-transform", update_transform, 1, 0, false, "(update-transform t) updates a transform");
    s7_define_function(sc, "transform?", is_transform, 1, 0, false, "(transform? anything) returns #t if its argument is a transform object");
    s7_define_function(sc, "transform-set-euler", is_transform, 4, 0, false, "(transform-set-euler t x y z)");

    SET_TRANSFORM_TYPE(sc,px);
    SET_TRANSFORM_TYPE(sc,py);
    SET_TRANSFORM_TYPE(sc,pz);
    SET_TRANSFORM_TYPE(sc,rx);
    SET_TRANSFORM_TYPE(sc,ry);
    SET_TRANSFORM_TYPE(sc,rz);
    SET_TRANSFORM_TYPE(sc,rw);
    SET_TRANSFORM_TYPE(sc,sx);
    SET_TRANSFORM_TYPE(sc,sy);
    SET_TRANSFORM_TYPE(sc,sz);
}


int sgls7_transform_type()
{
    return transform_type_tag;
}