#pragma once

struct Chunk;

#include "PipLangCommon.h"
#include "Object.h"

enum class InterpretResult
{
    OK,
    COMPILE_ERROR,
    RUNTIME_ERROR
};

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * 256)

struct CallFrame
{
    PipFunction *fn;
    u8 *ip;     // instruction pointer
    TValue *bp; // base pointer
};

struct VM
{
    CallFrame frames[FRAMES_MAX];
    int frameCount;

    TValue stack[STACK_MAX];
    TValue *sp; // stack pointer

    HashMap interned_strings;
    HashMap globals;
};

extern VM vm;

void PipLangVM_InitVM();
void PipLangVM_FreeVM();
void PipLangVM_DefineNativeFn(HashMap *mapToAddTo, const char *name, NativeFn fn);
void PipLangVM_NativeRuntimeError(const char *format, ...);
InterpretResult PipLangVM_RunGameCode(const char *source);
InterpretResult PipLangVM_RunGameFunction(const std::string& name);


InterpretResult PipLangVM_RunScript(const char *source);

/*

Declaration : Variable declaration
              Function declaration
              Statement

Statement : Block
            IfStatement
            WhileStatement
            ForStatement
            ReturnStatement
            PrintStatement
            Effect

Effect : Assignment
         ExpressionEffect

Expression : MesaScript "cond_or"

TODO

VM EPIC
 - Arrays
 - % BinOp
 - elif
 - +=, -=, /=, *=
 - for (n in [1, 2, 3, 5, 7, 11])
 - for (k,v in [ a: 2, b: 3 ])
 - 'continue' and 'break' in while and for-loops

PIPLANG GENERAL
 - More tests
 - math ops: flr, ceil, rnd -> all return integer
 - More string operations
 - Should we assert BOOLEAN type for JUMP_IF_FALSE?

NOTES
Goal: faster than Lua

- A continue statement jumps directly to the top of the nearest enclosing loop, skipping the rest of the loop body.
    Inside a for loop, a continue jumps to the increment clause, if there is one. It’s a compile-time error to have a
    continue statement not enclosed in a loop. Make sure to think about scope. What should happen to local variables
    declared inside the body of the loop or in blocks nested inside the loop when a continue is executed?

If I remove C++isms from PipLang and fully switch VM to C, then I can make instruction pointer a register.

Maybe I just dont handle cyclic references https://en.wikipedia.org/wiki/Reference_counting. Swift doesn't really.

I want to support all of the following:
list = []
for (n in [1, 2, 3, 5, 7, 11])
{
    // we want to provide a local var n which we update on every loop
    so maybe we first create the array from which we extract n, populate it by evaluating
    each expression, then once its populated we can start the for loop.
    once the for loop is done, we pop n, we pop the array
}
for (k,v in [ a: 2, b: 3 ])
for (,,)
for (mut i = 0, i < 10, ++i)

python del doesn't delete "the object" but rather that identifier
https://docs.python.org/3/tutorial/datastructures.html#the-del-statement
    mylist = [ ... ]
    copy = mylist
    del mylist
    # copy is still valid at this point and retains the original list reference

*/ 





