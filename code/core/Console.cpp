#include "Console.h"

#include "ConsoleBackendNoclip.h"

#include "../MesaMain.h"
#include "MesaIMGUI.h"
#include "Timer.h"
#include "InputSystem.h"



static noclip::console sNoclipConsole;
// static char sConsoleMessagesBuffer[] = {};
static NiceArray<char, 128> sConsoleCommandInputBuffer;


static void ExecuteConsoleCommand(const char *cmd)
{
//     // TODO (Check if this bug still exists after switching to noclip) - FUCKING MEMORY BUG TEXT_COMMAND GETS NULL TERMINATED EARLY SOMETIMES
//     char text_command_buffer[CONSOLE_COLS_MAX];
// #if _WIN32
//     strcpy_s(text_command_buffer, CONSOLE_COLS_MAX, text_command);//because text_command might point to read-only data
// #elif __APPLE__
//     strcpy(text_command_buffer, text_command);//because text_command might point to read-only data
// #endif

    if (*cmd == '\0') return;
    std::istringstream cmd_input_str(cmd);
    std::ostringstream cmd_output_str;
    sNoclipConsole.execute(cmd_input_str, cmd_output_str);
    printf("%s\n", (cmd_output_str.str().c_str()));
}

void SendInputToConsole(SDL_KeyboardEvent& keyevent)
{
    SDL_Keycode keycode = keyevent.keysym.sym;

    switch(keycode)
    {
        // case SDLK_ESCAPE:
        //     break;

        case SDLK_RETURN:
            ExecuteConsoleCommand(sConsoleCommandInputBuffer.data);
            sConsoleCommandInputBuffer.ResetCount();
            sConsoleCommandInputBuffer.ResetToZero();
            break;
        case SDLK_BACKSPACE:
            if (sConsoleCommandInputBuffer.count > 0)
            {
                sConsoleCommandInputBuffer.PopBack();
            }
            break;

        // TODO Flip through previously entered commands and fill command buffer w previous command
        // TODO Move cursor left right...maybe

        // case SDLK_PAGEUP:
        //     break;
        // case SDLK_PAGEDOWN:
        //     break;
        // case SDLK_LEFT:
        //     break;
        // case SDLK_RIGHT:
        //     break;
        // case SDLK_UP:
        //     break;
        // case SDLK_DOWN:
        //     break;
    }

    keycode = ModifyASCIIBasedOnModifiers(keycode, keyevent.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT));

    // CHECK INPUT
    const int ASCII_SPACE = 32;
    const int ASCII_TILDE = 126;
    if((ASCII_SPACE <= keycode && keycode <= ASCII_TILDE))
    {
        if (sConsoleCommandInputBuffer.NotAtCapacity())
        {
            sConsoleCommandInputBuffer.PushBack(keycode);
        }
    }
}


void DoBootScreen()
{
    static float a = float(rand());
    static float b = float(rand());
    static float c = float(rand());
    MesaGUI::PrimitivePanel(MesaGUI::UIRect(0, 0, EDITOR_FIXED_INTERNAL_RESOLUTION_W, EDITOR_FIXED_INTERNAL_RESOLUTION_H), 
                            vec4(0.05f, 0.05f, 0.05f, 1));
    MesaGUI::PrimitivePanel(MesaGUI::UIRect(1, 1, 10, 10), 5, 
                            vec4((sinf(Time.time * 4.f + a) + 1.f) * 0.5f, 
                                 (sinf(Time.time * 4.f + b) + 1.f) * 0.5f, 
                                 (sinf(Time.time * 4.f + c) + 1.f) * 0.5f, 1));


    sConsoleCommandInputBuffer.PushBack('_');
    if (sConsoleCommandInputBuffer.count > 0)
    {
        MesaGUI::PrimitiveText(40, 400, 9, MesaGUI::TextAlignment::Left, sConsoleCommandInputBuffer.data);
    }
    sConsoleCommandInputBuffer.PopBack();
}

#include "../MesaMain.h"

void SetupConsoleCommands()
{
    sNoclipConsole.bind_cmd("editor", StartEditor);
    sNoclipConsole.bind_cmd("game", StartGameSpace);
}

