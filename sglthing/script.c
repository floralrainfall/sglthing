#include "script.h"
#include "io.h"
#include "s7/s7.h"
#include <stdlib.h>
#include "sglthing.h"
#include "s7/script_functions.h"

struct script_system
{
    s7_scheme* scheme;
};

static s7_pointer __s7_engine_print(s7_scheme *sc, s7_pointer args)
{
    if(s7_is_string(s7_car(args)))
    {
        const char* str = s7_string(s7_car(args));
        printf("s7: '%s'\n", str);
        return s7_make_integer(sc, 1);
    }
  return(s7_wrong_type_arg_error(sc, "engine-print", 1, s7_car(args), "a string"));
}

struct script_system* script_init(char* file)
{
    char rsc_path[64];
    if(file_get_path(rsc_path,64,file))
    {
        printf("sglthing: starting script system on file %s\n", rsc_path);
        struct script_system* system = malloc(sizeof(struct script_system));
        system->scheme = s7_init();

        s7_define_function(system->scheme, "engine-print", __s7_engine_print, 1, 0, false, "(engine-print string) prints string to log");
        s7_define_variable(system->scheme, "math-pi", s7_make_real(system->scheme, M_PIf));

        sgls7_add_functions(system->scheme);

        s7_pointer code = s7_load(system->scheme, rsc_path);
        s7_pointer response;
        s7_eval(system->scheme, code, s7_rootlet(system->scheme));

        return system;
    }
    else
    {
        printf("sglthing: script %s not found\n", file);
        return NULL;
    }
}

void script_frame(void* world, struct script_system* system)
{
    ASSERT(system);
    s7_call(system->scheme, s7_name_to_value(system->scheme, "script-frame"), s7_cons(system->scheme, s7_make_integer(system->scheme, world), s7_nil(system->scheme)));
}

void script_frame_render(void* world, struct script_system* system)
{
    ASSERT(system);
    s7_call(system->scheme, s7_name_to_value(system->scheme, "script-frame-render"), s7_cons(system->scheme, s7_make_integer(system->scheme, world), s7_nil(system->scheme)));
}