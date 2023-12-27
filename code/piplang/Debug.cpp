#include "Debug.h"

#include "Chunk.h"
#include "Object.h"

void PrintRCObject(TValue value)
{
    switch (RCOBJ_TYPE(value))
    {
        case RCObject::STRING:
            printf("%s", RCOBJ_AS_STRING(value)->text.c_str());
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
            printf("%g", AS_NUMBER(value));
            break;
        case TValue::RCOBJ:
            PrintRCObject(value); 
            break;
    }
}

int Debug_SimpleInstruction(const char *name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

int Debug_ConstantInstruction(const char *name, Chunk *chunk, int offset)
{
    u8 constantIndex = chunk->bytecode->at(offset + 1);
    printf("%-16s %4d '", name, constantIndex);
    PrintTValue(chunk->constants->at(constantIndex));
    printf("'\n");
    return offset + 2;
}

int Debug_ConstantLongInstruction(const char *name, Chunk *chunk, int offset)
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
