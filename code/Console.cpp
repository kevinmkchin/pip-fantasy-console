#include "Console.h"

#include "ConsoleBackendNoclip.h"

#include "MesaMain.h"
#include "MesaIMGUI.h"
#include "Timer.h"
#include "InputSystem.h"
#include "PrintLog.h"


#define MESSAGES_CHAR_CAPACITY 4000
static noclip::console sNoclipConsole;
static char sConsoleMessagesBuffer[MESSAGES_CHAR_CAPACITY] = { 0 };
static NiceArray<char, 128> sConsoleCommandInputBuffer;


void SendMessageToConsole(const char *msg, size_t len)
{
    memmove(sConsoleMessagesBuffer, sConsoleMessagesBuffer + len, MESSAGES_CHAR_CAPACITY - len);
    memcpy(sConsoleMessagesBuffer + MESSAGES_CHAR_CAPACITY - len, msg, len);
#if INTERNAL_BUILD
    printf("%s", msg);
#endif
}

static void ExecuteConsoleCommand(const char *cmd)
{
    if (*cmd == '\0') return;
    std::istringstream cmdInputStream(cmd);
    std::ostringstream cmdOutputStream;
    sNoclipConsole.execute(cmdInputStream, cmdOutputStream);
    SendMessageToConsole(cmdOutputStream.str().c_str(), cmdOutputStream.str().length());
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

    int zeros = 0;
    while (zeros < 4000 && *(sConsoleMessagesBuffer + zeros) == 0)
    {
        ++zeros;
    }
    //printf("%d\n", zeros);
    if (zeros < 4000)
    {
        MesaGUI::PrimitiveText(40, 40, 9, MesaGUI::TextAlignment::Left, sConsoleMessagesBuffer + zeros);
    }

    sConsoleCommandInputBuffer.PushBack('_');
    if (sConsoleCommandInputBuffer.count > 0)
    {
        MesaGUI::PrimitiveText(40, 344, 9, MesaGUI::TextAlignment::Left, sConsoleCommandInputBuffer.data);
    }
    sConsoleCommandInputBuffer.PopBack();
}

void DoSingleCommandLine()
{
    MesaGUI::PrimitivePanel(MesaGUI::UIRect(0, 0, EDITOR_FIXED_INTERNAL_RESOLUTION_W, 13), 
                            vec4(0.05f, 0.05f, 0.05f, 1));

    sConsoleCommandInputBuffer.PushBack('_');
    if (sConsoleCommandInputBuffer.count > 0)
    {
        MesaGUI::PrimitiveText(4, 11, 9, MesaGUI::TextAlignment::Left, sConsoleCommandInputBuffer.data);
    }
    sConsoleCommandInputBuffer.PopBack();
}

#include "MesaMain.h"
#include "MesaScript.h"

void ElephantJPG()
{
    const std::string elephantASCII = "    _    _\n"
                                      "   /=\\\"\"/=\\       This is 16 color, pleasant to\n"
                                      "  (=(0_0 |=)__    create. It would be unpleasant\n"
                                      "   \\_\\ _/_/   )   to draw with 16,777,216 colors.\n"
                                      "     /_/   _  /\\\n"
                                      "    |/ |\\ || |\n"
                                      "       ~ ~  ~\n\n";
    SendMessageToConsole(elephantASCII.c_str(), elephantASCII.size());
}

void SetupConsoleCommands()
{
    sNoclipConsole.bind_cmd("editor", StartEditor);
    sNoclipConsole.bind_cmd("elephant", ElephantJPG);
    sNoclipConsole.bind_cmd("exec", TemporaryRunMesaScriptInterpreterOnFile);

    PrintLog.Message("--Boot menu initialized.");
}

