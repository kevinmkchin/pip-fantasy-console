#pragma once

#include <SDL.h>

#include "MesaCommon.h"
#include "MesaMath.h"
//#include "MesaUtility.h"

void SendInputToConsole(SDL_KeyboardEvent& keyevent);

void DoBootScreen();

void SetupConsoleCommands();
