#pragma once

#include "../MesaCommon.h"

#include <unordered_map>
#include <vector>


#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

struct RCObject;

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
    OP_TRUE,
    OP_FALSE,
    LOGICAL_NOT,
    RELOP_EQUAL,
    RELOP_GREATER,
    RELOP_LESSER,
    POP,
    DEFINE_GLOBAL,
    GET_GLOBAL,
    SET_GLOBAL
};

struct TValue
{
    enum VType
    {
        BOOLEAN,
        REAL,
        //FUNC,
        RCOBJ
    };

    VType type;

    union
    {
        bool boolean;
        double real;
        RCObject *rcobj;
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
    
    static TValue RCObject(RCObject *ptr)
    {
        TValue tv;
        tv.type = RCOBJ;
        tv.rcobj = ptr;
        return tv;
    }
};

#define BOOL_VAL(value)     (TValue::Boolean(value))
#define NUMBER_VAL(value)   (TValue::Number(value))
#define RCOBJ_VAL(value)    (TValue::RCObject(value))

#define AS_BOOL(value)      ((value).boolean)
#define AS_NUMBER(value)    ((value).real)
#define AS_RCOBJ(value)     ((value).rcobj)

#define IS_BOOL(value)      ((value).type == TValue::BOOLEAN)
#define IS_NUMBER(value)    ((value).type == TValue::REAL)
#define IS_RCOBJ(value)     ((value).type == TValue::RCOBJ)
