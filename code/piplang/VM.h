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


#define STACK_MAX 4000

struct VM
{
    Chunk *chunk;
    u8 *ip; // instruction pointer
    TValue stack[STACK_MAX];
    TValue *sp; // stack pointer

    HashMap interned_strings;
};

extern VM vm;

InterpretResult PipLangVM_RunScript(const char *source);



/*

TODO

VM EPIC
 - Statements
 - Global variables
 - Local variables
 - Control flow & jumps
 - Call stacks & functions
 - Strings, Maps, Lists (MesaScript_GCObject types)
 - Reference counting and transiency handling

PIPLANG GENERAL
 - Unit testing https://docs.racket-lang.org/rackunit/quick-start.html
 - RefCount Tests
 - % BinOp
 - while
 - for
 - elif
 - +=, -=, /=, *=
 - Handle cyclic references https://en.wikipedia.org/wiki/Reference_counting
 - map initialization
 - access map elements via dot (e.g. map.x or map.f(param))
 - allow chaining list or map access like: list[4]["x"][2] (an access of an access of an access)
 - need way to delete a list/table entry (remember to release ref count) https://docs.python.org/3/tutorial/datastructures.html#the-del-statement Note this is not deleting the object. Deleting an entry is different from deleting the object stored in that entry. I want to avoid ability to delete entire objects, because then we are able to destroy objects that are still referenced by other objects or variables...which would require tracking down every reference and removing them (otherwise they would be pointing to a "deleted" or "null" GCObject).
 - math ops: flr, ceil, rnd -> all return integer
 - More string operations


NOTES

Revisit replacing std::string and std::unordered_map with custom implementation of string, hash table, and dynamic array and linear memory allocator instead of std::vector
https://craftinginterpreters.com/strings.html
https://craftinginterpreters.com/hash-tables.html




*/ 





