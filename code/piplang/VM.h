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
InterpretResult PipLangVM_RunScript(const char *source);
void PipLangVM_DefineNativeFn(const char *name, NativeFn fn);

/*

declaration : statement

statement : expression

expression : cond_or


TODO

VM EPIC
 - Strings, Maps, Lists (MesaScript_GCObject types)
 - Reference counting and transiency handling
 - 'continue' and 'break' in while and for-loops
    - A continue statement jumps directly to the top of the nearest enclosing loop, skipping the rest of the loop body. 
    Inside a for loop, a continue jumps to the increment clause, if there is one. It’s a compile-time error to have a 
    continue statement not enclosed in a loop. Make sure to think about scope. What should happen to local variables 
    declared inside the body of the loop or in blocks nested inside the loop when a continue is executed?

PIPLANG GENERAL
 - Unit testing https://docs.racket-lang.org/rackunit/quick-start.html
 - RefCount Tests
 - % BinOp
 - elif
 - +=, -=, /=, *=
 - Handle cyclic references https://en.wikipedia.org/wiki/Reference_counting
 - map initialization
 - access map elements via dot (e.g. map.x or map.f(param))
 - allow chaining list or map access like: list[4]["x"][2] (an access of an access of an access)
 - need way to delete a list/table entry (remember to release ref count) https://docs.python.org/3/tutorial/datastructures.html#the-del-statement Note this is not deleting the object. Deleting an entry is different from deleting the object stored in that entry. I want to avoid ability to delete entire objects, because then we are able to destroy objects that are still referenced by other objects or variables...which would require tracking down every reference and removing them (otherwise they would be pointing to a "deleted" or "null" GCObject).
 - math ops: flr, ceil, rnd -> all return integer
 - More string operations
 - Native function arity checking
 - Native function RuntimeErrors
 - Should we assert BOOLEAN type for JUMP_IF_FALSE?

NOTES
Goal: faster than Lua
Revisit 1 * 2 = 3 + 4 assignment bug to fire Compile error https://craftinginterpreters.com/global-variables.html#assignment
If I remove C++isms from PipLang and fully switch VM to C, then I can make instruction pointer a register.

I want to support all of the following:

map = {}
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

I don't think there's anything wrong with looking forward to inspect everything in the parentheses before compiling them...

*/ 





