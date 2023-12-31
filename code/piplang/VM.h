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
 - compile time introspection
 - Clean up the way I'm emitting SET LOCAL SET GLOBAL SET MAP ENTRY instructions i think...
 - Arrays
 - % BinOp
 - elif
 - +=, -=, /=, *=
 - Unit testing https://docs.racket-lang.org/rackunit/quick-start.html
 - for (n in [1, 2, 3, 5, 7, 11])
 - for (k,v in [ a: 2, b: 3 ])

PIPLANG GENERAL
 - RefCount Tests
 - Handle cyclic references https://en.wikipedia.org/wiki/Reference_counting
 - math ops: flr, ceil, rnd -> all return integer
 - More string operations
 - Native function arity checking
 - Native function RuntimeErrors
 - Should we assert BOOLEAN type for JUMP_IF_FALSE?

NOTES
Goal: faster than Lua


- A continue statement jumps directly to the top of the nearest enclosing loop, skipping the rest of the loop body.
    Inside a for loop, a continue jumps to the increment clause, if there is one. It’s a compile-time error to have a
    continue statement not enclosed in a loop. Make sure to think about scope. What should happen to local variables
    declared inside the body of the loop or in blocks nested inside the loop when a continue is executed?


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

python del doesn't delete "the object" but rather that identifier
https://docs.python.org/3/tutorial/datastructures.html#the-del-statement
    mylist = [ ... ]
    copy = mylist
    del mylist
    # copy is still valid at this point and retains the original list reference


 - 'continue' and 'break' in while and for-loops


*/ 





