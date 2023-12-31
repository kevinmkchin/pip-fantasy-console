#include "VM.h"

#include <stdio.h>
#include <stdarg.h>

#include "Debug.h"
#include "Chunk.h"
#include "Scanner.h"
#include "Compiler.h"
#include "Object.h"

/// VM

VM vm;

static void Stack_Reset()
{
    vm.sp = vm.stack;
    vm.frameCount = 0;
}

static void Stack_Push(TValue value)
{
    *vm.sp = value;
    ++vm.sp;
}

static TValue Stack_Pop()
{
    --vm.sp;
    return *vm.sp;
}

static TValue Stack_Peek(int distance)
{
    return vm.sp[-1 - distance];
}

static void RuntimeError(const char *format, ...)
{
    CallFrame *frame = &vm.frames[vm.frameCount - 1];
    size_t instruction = frame->ip - frame->fn->chunk.bytecode->data() - 1;
    int line = frame->fn->chunk.linenumbers->at(instruction);
    fprintf(stderr, "[line %d] Runtime error: ", line);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    // stack trace
    fprintf(stderr, "=== Stack trace ===\n");
    for (int i = vm.frameCount - 1; i >= 0; --i)
    {
        CallFrame *frame = &vm.frames[i];
        PipFunction *fn = frame->fn;
        size_t instruction = frame->ip - fn->chunk.bytecode->data() - 1;
        fprintf(stderr, "[line %d] in ", fn->chunk.linenumbers->at(instruction));
        if (fn->name == NULL)
        {
            fprintf(stderr, "top-level script\n");
        }
        else
        {
            fprintf(stderr, "%s()\n", fn->name->text.c_str());
        }
    }

    Stack_Reset();
}

static bool IsFalsey(TValue v)
{
    return !AS_BOOL(v);
}

static void ConcatenateStrings()
{
    RCString *r = RCOBJ_AS_STRING(Stack_Pop());
    RCString *l = RCOBJ_AS_STRING(Stack_Pop());
    std::string temp = (l->text + r->text);
    Stack_Push(RCOBJ_VAL((RCObject*)CopyString(temp.c_str(), (int)temp.size(), false)));
}

static bool IsEqual(TValue l, TValue r)
{
    if (l.type != r.type) 
        return false;

    switch (l.type) 
    {
        case TValue::BOOLEAN: return AS_BOOL(l) == AS_BOOL(r);
        case TValue::REAL:    return AS_NUMBER(l) == AS_NUMBER(r);
        case TValue::RCOBJ:
        {
            return AS_RCOBJ(l) == AS_RCOBJ(r);
        }
        default: return false;
    }
}

static bool PushCallFrame(PipFunction *fn, u8 argc)
{
    if (argc != fn->arity)
    {
        RuntimeError("Expected %d arguments but got %d", fn->arity, argc);
        return false;
    }
    if (vm.frameCount == FRAMES_MAX) 
    {
        RuntimeError("Stack overflow. Exceeded max number of call frames.");
        return false;
    }

    CallFrame *frame = &vm.frames[vm.frameCount++];
    frame->fn = fn;
    frame->ip = fn->chunk.bytecode->data();
    frame->bp = vm.sp - argc - 1;
    return true;
}

static bool CallValue(TValue callee, u8 argc)
{
    if (IS_FUNCTION(callee))
    {
        return PushCallFrame(AS_FUNCTION(callee), argc);
    }
    else if (IS_NATIVEFN(callee))
    {
        NativeFn native = AS_NATIVEFN(callee);
        TValue result = native(argc, vm.sp - argc);
        vm.sp -= argc + 1;
        Stack_Push(result);
        return true;
    }
    RuntimeError("Invoked identifier does not map to a function.");
    return false;
}

static i32 IncrementRef(TValue v)
{
    return ++(AS_RCOBJ(v)->refCount);
}

static i32 DecrementRefButDontDestroy(TValue v)
{
    return --(AS_RCOBJ(v)->refCount);
}

static i32 DecrementRef(TValue v);
static void CheckRefCountAndDestroy(TValue v)
{
    RCObject *obj = AS_RCOBJ(v);
    if (obj->refCount <= 0)
    {
        if (RCOBJ_IS_MAP(v))
        {
            HashMap *map = RCOBJ_AS_MAP(v);
            HashMapEntry *entries = map->entries;
            int cap = map->capacity;
            for (int i = 0; i < cap; ++i)
            {
                if (entries->key)
                {
                    DecrementRef(RCOBJ_VAL((RCObject*)entries->key));
                    if (IS_RCOBJ(entries->value))
                    {
                        DecrementRef(entries->value);
                    }
                }
            }
        }
        FreeRCObject(obj);
    }
}

static i32 DecrementRef(TValue v)
{
    i32 ref = DecrementRefButDontDestroy(v);
    CheckRefCountAndDestroy(v);
    return ref;
}

static InterpretResult Run()
{
    CallFrame *frame = &vm.frames[vm.frameCount - 1];

#define VM_READ_BYTE() (*frame->ip++) // read byte and move pointer along
#define VM_READ_WORD() (frame->ip += 2, (u16)((frame->ip[-2] << 8) | frame->ip[-1]))
#define VM_READ_THREE_BYTES() (frame->ip += 3, (u32)((frame->ip[-3] << 8) | (frame->ip[-2] << 8) | frame->ip[-1]))
#define VM_READ_CONSTANT() (frame->fn->chunk.constants->at(VM_READ_BYTE()))
#define VM_READ_CONSTANT_LONG() (frame->fn->chunk.constants->at(VM_READ_THREE_BYTES()))
#define VM_BINARY_OP(resultValueConstructor, op) \
    do { \
        if (!IS_NUMBER(Stack_Peek(0)) || !IS_NUMBER(Stack_Peek(1))) \
        { \
            RuntimeError("Operands to BINOP must be number values."); \
            return InterpretResult::RUNTIME_ERROR; \
        } \
        double r = Stack_Pop().real; \
        double l = Stack_Pop().real; \
        Stack_Push(resultValueConstructor(l op r)); \
    } while (false)


    for (;;)
    {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (TValue *slot = vm.stack; slot < vm.sp; ++slot)
        {
            printf("[ ");
            PrintTValue(*slot);
            printf(" ]");
        }
        printf("\n");
        DisassembleInstruction(&frame->fn->chunk, (int)(frame->ip - frame->fn->chunk.bytecode->data()));
#endif
        OpCode op;
        switch (op = (OpCode)VM_READ_BYTE())
        {
            case OpCode::NEW_HASHMAP:
            {
                RCObject* map = NewRCObject(RCObject::MAP);
                Stack_Push(RCOBJ_VAL(map));
                break;
            }

            case OpCode::PRINT:
                PrintTValue(Stack_Pop());
                printf("\n");
                break;

            case OpCode::RETURN:
            {
                TValue result = Stack_Pop();
                --vm.frameCount;
                if (vm.frameCount == 0)
                {
                    Stack_Pop();
                    return InterpretResult::OK;
                }

                bool isResultRefCounted = IS_RCOBJ(result);

                // Need to sweep all locals to decrement ref
                // This sweeps from sp to bp so works even for returns mid lexical-scope (i.e. mid for-loop)
                {
                    while (vm.sp > frame->bp + 1) // Note(Kevin): +1 is because first local is reserved for fn itself
                    {
                        TValue localOrParam = Stack_Pop();
                        if (IS_RCOBJ(localOrParam))
                        {
                            if (isResultRefCounted && localOrParam.rcobj == result.rcobj)
                                DecrementRefButDontDestroy(
                                        localOrParam); // return value is transient until captured or OpCode::POP
                            else
                                DecrementRef(localOrParam);
                        }
                    }
                    --vm.sp;
                }

                Stack_Push(result);
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }

            case OpCode::CALL:
            {
                u8 argc = VM_READ_BYTE();
                if (!CallValue(Stack_Peek(argc), argc))
                {
                    return InterpretResult::RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1]; // Move to callee frame
                break;
            }

            case OpCode::CONSTANT:
                Stack_Push(VM_READ_CONSTANT());
                break;

            case OpCode::CONSTANT_LONG:
                Stack_Push(VM_READ_CONSTANT_LONG());
                break;

            case OpCode::POP:
            {
                TValue v = Stack_Pop();
                if (IS_RCOBJ(v)) CheckRefCountAndDestroy(v);
                break;
            }

            case OpCode::POP_LOCAL:
            {
                TValue v = Stack_Pop();
                if (IS_RCOBJ(v)) DecrementRef(v);
                break;
            }

            case OpCode::INCREMENT_REF_IF_RCOBJ:
            {
                TValue v = Stack_Peek(0);
                if (IS_RCOBJ(v)) IncrementRef(v);
                break;
            }

            case OpCode::DEFINE_GLOBAL:
            {
                RCString *name = RCOBJ_AS_STRING(VM_READ_CONSTANT_LONG());
                TValue value = Stack_Peek(0);
                HashMapSet(&vm.globals, name, value, NULL);
                IncrementRef(RCOBJ_VAL((RCObject*)name));
                if (IS_RCOBJ(value)) IncrementRef(value);
                Stack_Pop();
                break;
            }

            case OpCode::GET_GLOBAL:
            {
                RCString *name = RCOBJ_AS_STRING(VM_READ_CONSTANT_LONG());
                TValue value;
                if (!HashMapGet(&vm.globals, name, &value))
                {
                    RuntimeError("Undefined variable '%s'.", name->text.c_str());
                    return InterpretResult::RUNTIME_ERROR;
                }
                Stack_Push(value);
                break;
            }

            case OpCode::SET_GLOBAL:
            {
                RCString *name = RCOBJ_AS_STRING(VM_READ_CONSTANT_LONG());
                TValue replaced;
                TValue value = Stack_Peek(0);
                bool isNewKey = HashMapSet(&vm.globals, name, value, &replaced);
                if (isNewKey)
                {
                    HashMapDelete(&vm.globals, name);
                    RuntimeError("Undefined variable '%s'.", name->text.c_str());
                    return InterpretResult::RUNTIME_ERROR;
                }
                if (IS_RCOBJ(value)) IncrementRef(value);
                if (IS_RCOBJ(replaced)) DecrementRef(replaced);
                Stack_Pop();
                break;
            }

            case OpCode::GET_LOCAL:
            {
                u8 bpOffset = VM_READ_BYTE();
                Stack_Push(frame->bp[bpOffset]);
                break;
            }

            case OpCode::SET_LOCAL:
            {
                u8 bpOffset = VM_READ_BYTE();
                TValue value = Stack_Pop();
                TValue replaced = frame->bp[bpOffset];
                frame->bp[bpOffset] = value;
                if (IS_RCOBJ(value)) IncrementRef(value);
                if (IS_RCOBJ(replaced)) DecrementRef(replaced);
                break;
            }

            case OpCode::SET_MAP_ENTRY:
            {
                TValue v = Stack_Pop();
                TValue k = Stack_Pop();
                TValue m = Stack_Peek(0);
                if (!RCOBJ_IS_MAP(m) || !RCOBJ_IS_STRING(k))
                {
                    RuntimeError("Provided invalid map or key when setting map entry.");
                    return InterpretResult::RUNTIME_ERROR;
                }
                TValue replaced;
                bool isNewKey = HashMapSet(RCOBJ_AS_MAP(m), RCOBJ_AS_STRING(k), v, &replaced);
                if (isNewKey) IncrementRef(k);
                if (IS_RCOBJ(v)) IncrementRef(v);
                if (!isNewKey && IS_RCOBJ(replaced)) DecrementRef(replaced);
                break;
            }

            case OpCode::GET_MAP_ENTRY:
            {
                TValue k = Stack_Pop();
                TValue m = Stack_Pop();
                if (!RCOBJ_IS_MAP(m) || !RCOBJ_IS_STRING(k))
                {
                    RuntimeError("Provided invalid map or key when getting map entry.");
                    return InterpretResult::RUNTIME_ERROR;
                }
                TValue v;
                if (!HashMapGet(RCOBJ_AS_MAP(m), RCOBJ_AS_STRING(k), &v))
                {
                    RuntimeError("Provided key does not exist in map.");
                    return InterpretResult::RUNTIME_ERROR;
                }
                Stack_Push(v);
                break;
            }

            case OpCode::DEL_MAP_ENTRY:
            {
                TValue k = Stack_Pop();
                TValue m = Stack_Peek(0);
                if (!RCOBJ_IS_MAP(m) || !RCOBJ_IS_STRING(k))
                {
                    RuntimeError("Provided invalid map to 'insert' contextual keyword.");
                    return InterpretResult::RUNTIME_ERROR;
                }
                TValue v;
                if (!HashMapGet(RCOBJ_AS_MAP(m), RCOBJ_AS_STRING(k), &v))
                {
                    RuntimeError("Provided key does not exist in map"); // TODO probably just make this a warning
                    return InterpretResult::RUNTIME_ERROR;
                }
                HashMapDelete(RCOBJ_AS_MAP(m), RCOBJ_AS_STRING(k));
                DecrementRef(k);
                if (IS_RCOBJ(v)) IncrementRef(v);
                break;
            }

            case OpCode::NEGATE:
            {
                if (!IS_NUMBER(Stack_Peek(0)))
                {
                    RuntimeError("Operand to NEGATE op must be a number value.");
                    return InterpretResult::RUNTIME_ERROR;
                }
                Stack_Push(TValue::Number(-AS_NUMBER(Stack_Pop())));
                break;
            }
            case OpCode::ADD: 
            {
                if (RCOBJ_IS_STRING(Stack_Peek(0)) && RCOBJ_IS_STRING(Stack_Peek(1)))
                {
                    ConcatenateStrings();
                }
                else if (IS_NUMBER(Stack_Peek(0)) && IS_NUMBER(Stack_Peek(1)))
                {
                    double r = Stack_Pop().real;
                    double l = Stack_Pop().real;
                    Stack_Push(NUMBER_VAL(l + r));
                }
                else
                {
                    RuntimeError("Operands to BINOP must be number values.");
                    return InterpretResult::RUNTIME_ERROR;
                }
                break;
            }
            case OpCode::SUBTRACT: VM_BINARY_OP(NUMBER_VAL, -); break;
            case OpCode::MULTIPLY: VM_BINARY_OP(NUMBER_VAL, *); break;
            case OpCode::DIVIDE: VM_BINARY_OP(NUMBER_VAL, /); break;

            case OpCode::OP_TRUE: Stack_Push(BOOL_VAL(true)); break;
            case OpCode::OP_FALSE: Stack_Push(BOOL_VAL(false)); break;
            case OpCode::LOGICAL_NOT:
                if (!IS_BOOL(Stack_Peek(0)))
                {
                    RuntimeError("Operand to LOGICAL NOT op must be a boolean value.");
                    return InterpretResult::RUNTIME_ERROR;
                }
                Stack_Push(BOOL_VAL(IsFalsey(Stack_Pop())));
                break;
            case OpCode::RELOP_EQUAL:
                Stack_Push(BOOL_VAL(IsEqual(Stack_Pop(), Stack_Pop())));
                break;
            case OpCode::RELOP_GREATER: VM_BINARY_OP(BOOL_VAL, >); break;
            case OpCode::RELOP_LESSER: VM_BINARY_OP(BOOL_VAL, <); break;

            case OpCode::JUMP_IF_FALSE: 
            {
                u16 jumpOffset = VM_READ_WORD();
                if (IsFalsey(Stack_Peek(0))) frame->ip += jumpOffset;
                break;
            }

            case OpCode::JUMP:
            {
                u16 jumpOffset = VM_READ_WORD();
                frame->ip += jumpOffset;
                break;
            }

            case OpCode::JUMP_BACK:
            {
                u16 jumpOffset = VM_READ_WORD();
                frame->ip -= jumpOffset;
                break;
            }

        }
    }


#undef VM_READ_BYTE
#undef VM_READ_WORD
#undef VM_READ_THREE_BYTES
#undef VM_READ_CONSTANT
#undef VM_READ_CONSTANT_LONG
#undef VM_BINARY_OP
}

#include "../Timer.h"

static InterpretResult Interpret(const char *source)
{
    PipFunction *script = Compile(source);
    if (script == NULL) return InterpretResult::COMPILE_ERROR;

    Stack_Push(FUNCTION_VAL(script)); // Why does first value of frame stack have to be the function itself?
    PushCallFrame(script, 0);

    double t = Time.TimeSinceProgramStartInSeconds();
    InterpretResult result = Run();
    printf("vm took %lf\n", Time.TimeSinceProgramStartInSeconds() - t);
    return result;
}

void PipLangVM_InitVM()
{
    Stack_Reset();
    AllocateHashMap(&vm.interned_strings);
    AllocateHashMap(&vm.globals);
}

void PipLangVM_FreeVM()
{
    // TODO CLEAN UP MEMORY UPON SUCCESSFUL EXIT OR RUNTIME ERROR
    // Sweep all locals from vm.sp to vm.stack[0]
    //      If RuntimeError, locals are still living on stack
    // Sweep all globals
    // All unique sweeped NON STRING RCObjects AND PipFunctions must be freed
    // All strings in interned_strings (they're all unique) must be freed
    // FreeHashMap(globals)
    // FreeHashMap(interned_strings)

    FreeHashMap(&vm.globals);
    FreeHashMap(&vm.interned_strings);
}

static TValue PrintGlobals(int argc, TValue *argv)
{

    printf("======================\nPrinting GLOBALS\n");
    for (int i = 0; i < vm.globals.capacity; ++i)
    {
        if (vm.globals.entries[i].key)
        {
            printf("    %-16s", vm.globals.entries[i].key->text.c_str());
            PrintTValue(vm.globals.entries[i].value);
            printf("\n");
        }
    }
    printf("======================\n");

    return BOOL_VAL(false);
}

InterpretResult PipLangVM_RunScript(const char *source)
{
    PipLangVM_DefineNativeFn("printglobals", PrintGlobals);
    return Interpret(source);
}

void PipLangVM_DefineNativeFn(const char *name, NativeFn fn)
{
    if (vm.globals.entries == NULL)
    {
        printf("pip vm: attempting to define native function before vm is initialized.");
        return;
    }

    Stack_Push(RCOBJ_VAL((RCObject*)CopyString(name, (int)strlen(name), true)));
    Stack_Push(NATIVEFN_VAL((void*)fn));
    HashMapSet(&vm.globals, RCOBJ_AS_STRING(vm.stack[0]), vm.stack[1], NULL);
    Stack_Pop();
    Stack_Pop();
}












