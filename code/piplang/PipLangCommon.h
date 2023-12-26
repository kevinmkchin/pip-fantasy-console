#pragma once

#include "../MesaCommon.h"

#define DEBUG_TRACE_EXECUTION
#define DEBUG_PRINT_CODE

enum class OpCode : u8
{
    RETURN,
    CONSTANT,
    CONSTANT_LONG,
    NEGATE,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    TRUE,
    FALSE,
    LOGICAL_NOT,
    RELOP_EQUAL,
    RELOP_GREATER,
    RELOP_LESSER,
};

struct TValue
{
    enum VType
    {
        BOOLEAN,
        REAL,
        //FUNC,
        //GCOBJ
    };

    VType type;

    union
    {
        bool boolean;
        double real;
    };

    static TValue Boolean(bool v)
    {
        TValue tv;
        tv.type = BOOLEAN;
        tv.boolean = v;
        return tv;
    }

    static TValue Number(double v)
    {
        TValue tv;
        tv.type = REAL;
        tv.real = v;
        return tv;
    }
};

#define BOOL_VAL(value)     (TValue::Boolean(value))
#define NUMBER_VAL(value)   (TValue::Number(value))
#define AS_BOOL(value)      ((value).boolean)
#define AS_NUMBER(value)    ((value).real)

#define IS_BOOL(value)      ((value).type == TValue::BOOLEAN)
#define IS_NUMBER(value)    ((value).type == TValue::REAL)
