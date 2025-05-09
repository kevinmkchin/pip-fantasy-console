#pragma once

#include <SDL.h>

#include "ConsoleBackendNoclip.h"

#include "MesaCommon.h"
#include "MesaMath.h"

/** Usage: print messages */
void SendMessageToConsole(const char *msg, size_t len);

/** Usage: only send input when the console is active.  */
void SendInputToConsole(SDL_KeyboardEvent& keyevent);

void SetupConsoleCommands();

noclip::console *GiveMeTheConsole();

/* -- Front ends -- */
void DoBootScreen();
void DoSingleCommandLine();
