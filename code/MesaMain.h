#pragma once

#include "core/MesaCommon.h"

// This header is the program state.

enum class MesaProgramMode
{
    Invalid,
    BootScreen,
    Editor,
    Game,
};

MesaProgramMode CurrentProgramMode();
void StartEditor();
void StartGameSpace();


