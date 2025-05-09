#include "Console.h"


#include "MesaMain.h"
#include "GUI.H"
#include "Input.h"


#define MESSAGES_CHAR_CAPACITY 4000
static noclip::console sNoclipConsole;
static char sConsoleMessagesBuffer[MESSAGES_CHAR_CAPACITY + 1] = { 0 };
static NiceArray<char, 128> sConsoleCommandInputBuffer;

noclip::console *GiveMeTheConsole()
{
    return &sNoclipConsole;
}

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


static Gui::ALH *consoleLayout = NULL;
static vec4 s_ConsoleBlack = vec4(0.05f, 0.05f, 0.05f, 1);

void DoBootScreen()
{
    bool doOnce = false;
    if (!doOnce)
    {
        doOnce = true;
        consoleLayout = Gui::NewALH(true);
    }
    Gui::UpdateMainCanvasALH(consoleLayout);

    static float a = float(rand());
    static float b = float(rand());
    static float c = float(rand());
    Gui::PrimitivePanel(Gui::UIRect(consoleLayout), s_ConsoleBlack);
    // Gui::PrimitivePanel(Gui::UIRect(1, 1, 10, 10), 5,
    //                         vec4((sinf(Time.time * 4.f + a) + 1.f) * 0.5f, 
    //                              (sinf(Time.time * 4.f + b) + 1.f) * 0.5f, 
    //                              (sinf(Time.time * 4.f + c) + 1.f) * 0.5f, 1));

    int zeros = 0;
    while (zeros < 4000 && *(sConsoleMessagesBuffer + zeros) == 0)
    {
        ++zeros;
    }
    //printf("%d\n", zeros);
    if (zeros < 4000)
    {
        Gui::PrimitiveText(40, 40, 9, Gui::Align::Left, sConsoleMessagesBuffer + zeros);
    }

    bool showCursor = int(Time.time * 3.6f) % 2 == 0;
    if (showCursor)
        sConsoleCommandInputBuffer.PushBack('_');
    if (sConsoleCommandInputBuffer.count > 0)
    {
        Gui::PrimitiveText(40, consoleLayout->h - 60, 9, Gui::Align::Left, sConsoleCommandInputBuffer.data);
    }
    if (showCursor)
        sConsoleCommandInputBuffer.PopBack();
}

void DoSingleCommandLine()
{
    Gui::PrimitivePanel(Gui::UIRect(0, 0, 4000, 15), s_ConsoleBlack);

    bool showCursor = int(Time.time * 3.6f) % 2 == 0;
    if (showCursor)
        sConsoleCommandInputBuffer.PushBack('_');
    if (sConsoleCommandInputBuffer.count > 0 )
    {
        Gui::PrimitiveText(4, 13, 9, Gui::Align::Left, sConsoleCommandInputBuffer.data);
    }
    if (showCursor)
        sConsoleCommandInputBuffer.PopBack();
}

#include "MesaMain.h"

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
    sNoclipConsole.bind_cmd("play", StartGameSpace);
    sNoclipConsole.bind_cmd("elephant", ElephantJPG);

    PrintLog.Message("Boot menu initialized...");
}

