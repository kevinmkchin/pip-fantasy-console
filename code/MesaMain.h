#pragma once

#include "MesaCommon.h"

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

extern struct SDL_Window *g_SDLWindow;

