#pragma once


enum class InterpretResult
{
    OK,
    COMPILE_ERROR,
    RUNTIME_ERROR
};

InterpretResult PipLangVM_RunScript(const char *source);


