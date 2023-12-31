#pragma once

#include "../MesaCommon.h"


#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

struct RCObject;
struct PipFunction;

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
    POP_LOCAL,
    DEFINE_GLOBAL,
    GET_GLOBAL,
    SET_GLOBAL,
    GET_LOCAL,
    SET_LOCAL,
    JUMP,
    JUMP_BACK,
    JUMP_IF_FALSE,
    CALL,
    PRINT,
    NEW_HASHMAP,
    SET_MAP_ENTRY,
    GET_MAP_ENTRY,
    DEL_MAP_ENTRY,
    INCREMENT_REF_IF_RCOBJ
};

struct TValue
{
    enum VType
    {
        BOOLEAN,
        REAL,
        FUNC,
        NATIVEFN,
        RCOBJ
    };

    VType type = BOOLEAN;

    union
    {
        bool boolean{};
        double real;
        RCObject *rcobj;
        PipFunction *fn;
        void *nativefn;
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
    
    static TValue Function(PipFunction *pipfn)
    {
        TValue tv;
        tv.type = FUNC;
        tv.fn = pipfn;
        return tv;
    }

    static TValue NativeFunction(void *nativefn)
    {
        TValue tv;
        tv.type = NATIVEFN;
        tv.nativefn = nativefn;
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

typedef TValue (*NativeFn)(int argc, TValue *argv);

#define BOOL_VAL(boolean)      (TValue::Boolean(boolean))
#define NUMBER_VAL(real)       (TValue::Number(real))
#define FUNCTION_VAL(pipfn)    (TValue::Function(pipfn))
#define NATIVEFN_VAL(nativefn) (TValue::NativeFunction(nativefn))
#define RCOBJ_VAL(rcobj)       (TValue::RCObject(rcobj))

#define AS_BOOL(value)         ((value).boolean)
#define AS_NUMBER(value)       ((value).real)
#define AS_FUNCTION(value)     ((value).fn)
#define AS_NATIVEFN(value)     ((NativeFn)(value).nativefn)
#define AS_RCOBJ(value)        ((value).rcobj)

#define IS_BOOL(value)         ((value).type == TValue::BOOLEAN)
#define IS_NUMBER(value)       ((value).type == TValue::REAL)
#define IS_FUNCTION(value)     ((value).type == TValue::FUNC)
#define IS_NATIVEFN(value)     ((value).type == TValue::NATIVEFN)
#define IS_RCOBJ(value)        ((value).type == TValue::RCOBJ)


#if (defined _MSC_VER)
#define PipLangAssert(predicate) if(!(predicate)) { __debugbreak(); }
#else
#define PipLangAssert(predicate) if(!(predicate)) {  }
#endif
