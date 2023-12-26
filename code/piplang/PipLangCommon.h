#pragma once

#include "../MesaCommon.h"

enum class OpCode : u8
{
    RETURN,
    CONSTANT,
    CONSTANT_LONG,
    NEGATE,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE
};

struct TValue
{
    union
    {
        double real;
    };
};

#define DEBUG_TRACE_EXECUTION
#define DEBUG_PRINT_CODE
