#pragma once

#include "MesaCommon.h"



enum class InterpretResult
{
    OK,
    COMPILE_ERROR,
    RUNTIME_ERROR
};

InterpretResult Interpret(const char *source);


