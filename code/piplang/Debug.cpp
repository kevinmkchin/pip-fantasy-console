#include "Debug.h"

#include "Chunk.h"
#include "Object.h"

static void PrintFunction(PipFunction *fn)
{
    if (fn->name == NULL)
    {
        printf("<pip top-level script>");
        return;
    }
    printf("<fn %s>", fn->name->text.c_str());
}

static void PrintRCObject(TValue value)
{
    switch (RCOBJ_TYPE(value))
    {
        case RCObject::STRING:
            printf("%s", RCOBJ_AS_STRING(value)->text.c_str());
            break;
        case RCObject::MAP:
            // entries include tombstones
            printf("<map{%d entries}", RCOBJ_AS_MAP(value)->count);
            printf(" : %d ref>", AS_RCOBJ(value)->refCount);
            break;
    }
}

void PrintTValue(TValue value)
{
    switch (value.type)
    {
        case TValue::BOOLEAN:
            printf(AS_BOOL(value) ? "True" : "False");
            break;
        case TValue::REAL:
            printf("%lf", AS_NUMBER(value));
            break;
        case TValue::FUNC:
            PrintFunction(AS_FUNCTION(value));
            break;
        case TValue::NATIVEFN:
            printf("<nativefn>");
            break;
        case TValue::RCOBJ:
            PrintRCObject(value); 
            break;
    }
}

static int Debug_SimpleInstruction(const char *name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

static int Debug_ConstantInstruction(const char *name, Chunk *chunk, int offset)
{
    u8 constantIndex = chunk->bytecode->at(offset + 1);
    printf("%-16s %4d '", name, constantIndex);
    PrintTValue(chunk->constants->at(constantIndex));
    printf("'\n");
    return offset + 2;
}

static int Debug_ConstantLongInstruction(const char *name, Chunk *chunk, int offset)
{
    u32 byte2 = chunk->bytecode->at(offset + 1);
    u32 byte1 = chunk->bytecode->at(offset + 2);
    u32 byte0 = chunk->bytecode->at(offset + 3);
    u32 constantIndex = byte2 << 16 | byte1 << 8 | byte0;
    printf("%-16s %4d '", name, constantIndex);
    PrintTValue(chunk->constants->at(constantIndex));
    printf("'\n");
    return offset + 4;
}

static int Debug_ByteInstruction(const char *name, Chunk *chunk, int offset)
{
    u8 byte = chunk->bytecode->at(offset + 1);
    printf("%-16s %4d\n", name, byte);
    return offset + 2;
}

static int Debug_JumpInstruction(const char *name, int sign, Chunk *chunk, int offset) {
    u16 jump = (u16)(chunk->bytecode->at(offset + 1) << 8);
    jump |= chunk->bytecode->at(offset + 2);
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

int DisassembleInstruction(Chunk *chunk, int offset)
{
    printf("%04d ", offset);

    if (offset > 0 && chunk->linenumbers->at(offset) == chunk->linenumbers->at(offset - 1))
    {
        printf("   | ");
    }
    else
    {
        printf("%4d ", chunk->linenumbers->at(offset));
    }

    OpCode instruction = (OpCode)chunk->bytecode->at(offset);
    switch (instruction)
    {
    case OpCode::RETURN:
        return Debug_SimpleInstruction("RETURN", offset);
    case OpCode::PRINT:
        return Debug_SimpleInstruction("PRINT", offset);
    case OpCode::CONSTANT:
        return Debug_ConstantInstruction("CONSTANT", chunk, offset);
    case OpCode::CONSTANT_LONG:
        return Debug_ConstantLongInstruction("CONSTANT_LONG", chunk, offset);
    case OpCode::NEGATE:
        return Debug_SimpleInstruction("NEGATE", offset);
    case OpCode::ADD:
        return Debug_SimpleInstruction("ADD", offset);
    case OpCode::SUBTRACT:
        return Debug_SimpleInstruction("SUBTRACT", offset);
    case OpCode::MULTIPLY:
        return Debug_SimpleInstruction("MULTIPLY", offset);
    case OpCode::DIVIDE:
        return Debug_SimpleInstruction("DIVIDE", offset);
    case OpCode::OP_TRUE:
        return Debug_SimpleInstruction("TRUE", offset);
    case OpCode::OP_FALSE:
        return Debug_SimpleInstruction("FALSE", offset);
    case OpCode::LOGICAL_NOT:
        return Debug_SimpleInstruction("LOGICAL_NOT", offset);
    case OpCode::RELOP_EQUAL:
        return Debug_SimpleInstruction("RELOP_EQUAL", offset);
    case OpCode::RELOP_GREATER:
        return Debug_SimpleInstruction("RELOP_GREATER", offset);
    case OpCode::RELOP_LESSER:
        return Debug_SimpleInstruction("RELOP_LESSER", offset);
    case OpCode::POP:
        return Debug_SimpleInstruction("POP", offset);
    case OpCode::POP_LOCAL:
        return Debug_SimpleInstruction("POP_LOCAL", offset);
    case OpCode::DEFINE_GLOBAL:
        return Debug_ConstantLongInstruction("DEFINE_GLOBAL", chunk, offset);
    case OpCode::GET_GLOBAL:
        return Debug_ConstantLongInstruction("GET_GLOBAL", chunk, offset);
    case OpCode::SET_GLOBAL:
        return Debug_ConstantLongInstruction("SET_GLOBAL", chunk, offset);
    case OpCode::GET_LOCAL:
        return Debug_ByteInstruction("GET_LOCAL", chunk, offset);
    case OpCode::SET_LOCAL:
        return Debug_ByteInstruction("SET_LOCAL", chunk, offset);
    case OpCode::JUMP:
        return Debug_JumpInstruction("JUMP", 1, chunk, offset);
    case OpCode::JUMP_BACK:
        return Debug_JumpInstruction("JUMP_BACK", -1, chunk, offset);
    case OpCode::JUMP_IF_FALSE:
        return Debug_JumpInstruction("JUMP_IF_FALSE", 1, chunk, offset);
    case OpCode::CALL:
        return Debug_ByteInstruction("CALL", chunk, offset);
    case OpCode::NEW_HASHMAP:
        return Debug_SimpleInstruction("NEW_HASHMAP", offset);
    case OpCode::INCREMENT_REF_IF_RCOBJ:
        return Debug_SimpleInstruction("INCREMENT_REF_IF_RCOBJ", offset);
    case OpCode::SET_MAP_ENTRY:
        return Debug_SimpleInstruction("SET_MAP_ENTRY", offset);
    case OpCode::GET_MAP_ENTRY:
        return Debug_SimpleInstruction("GET_MAP_ENTRY", offset);
    case OpCode::DEL_MAP_ENTRY:
        return Debug_SimpleInstruction("DEL_MAP_ENTRY", offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}

void DisassembleChunk(Chunk *chunk, const char *name)
{
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->bytecode->size();)
    {
        offset = DisassembleInstruction(chunk, offset);
    }
}
