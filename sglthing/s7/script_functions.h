#ifndef S7_SCRIPT_FUNCTIONS_H
#define S7_SCRIPT_FUNCTIONS_H
#include "s7.h"

typedef enum
{
    UNKNOWN = 0x4000,
    MODEL_POINTER,
} C_Type;

void sgls7_add_functions(s7_scheme* sc);

#endif