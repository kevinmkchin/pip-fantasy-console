#include "MesaMain.h"

#if MESA_WINDOWS
#include <windows.h>
#include <dwmapi.h>
#endif

#include <SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "singleheaders/stb_image.h"
#define STB_SPRINTF_IMPLEMENTATION
#include "singleheaders/stb_sprintf.h"

#include "UTILITY.H"
#include "GfxRenderer.h"
#include "Input.h"
#include "Console.h"
#include "GUI.H"
#include "editor/Editor.h"
#include "Game.h"

SDL_Window *g_SDLWindow;
static SDL_GLContext g_SDLGLContext;
static bool g_ProgramShouldShutdown = false;
static Gfx::CoreRenderer g_gfx;
static MesaProgramMode g_ProgramMode = MesaProgramMode::Invalid;

MesaProgramMode CurrentProgramMode()
{
    return g_ProgramMode;
}

static bool InitializeEverything()
{
    g_ProgramShouldShutdown = false;

    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system"); // https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/LearnWin32/dpi-and-device-independent-pixels.md#dwm-scaling
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "0"); // https://github.com/libsdl-org/SDL/commit/ab81a559f43abc0858c96788f8e00bbb352287e8

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) return false;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    g_SDLWindow = SDL_CreateWindow("pip",
                                   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOW_STARTING_SIZE_W,
                                   SDL_WINDOW_STARTING_SIZE_H,
                                   SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    g_SDLGLContext = SDL_GL_CreateContext(g_SDLWindow);

    if (g_SDLWindow == nullptr || g_SDLGLContext == nullptr) return false;

    SDL_SetWindowMinimumSize(g_SDLWindow, 100, 30);
    SDL_GL_SetSwapInterval(0);
    //SDL_SetWindowFullscreen(g_SDLWindow, SDL_WINDOW_FULLSCREEN);

    PrintLog.Message("MesaBIOS (" + std::string(PROJECT_BUILD_VERSION) + ")");
    PrintLog.Message("Booting from Disk...\n");
    // PrintLog.Message("--Screen size " + std::to_string(EDITOR_FIXED_INTERNAL_RESOLUTION_W) + 
    //                  "x" + std::to_string(EDITOR_FIXED_INTERNAL_RESOLUTION_H));

    g_gfx.Init();
    Gui::Init();

    SetupConsoleCommands();

    PrintLog.Message("Graphics loaded...");
    PrintLog.Message("Sound loaded...\n");

    PrintLog.Message("The Mesa Personal Computer");
    PrintLog.Message(std::string(PROJECT_BUILD_VERSION) + " (C)Copyright Kevin Chin 2023");

    PrintLog.Message("\n");

	return true;
}

static bool consoleActive = false;

static void ProcessSDLEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        // Lower level engine related input
        switch (event.type)
        {
            case SDL_WINDOWEVENT:
            {
                switch (event.window.event) 
                {
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                    case SDL_WINDOWEVENT_RESIZED:
                    {
                        g_gfx.WindowSizeChanged();
                        break;
                    }
                }
                break;
            }
            case SDL_QUIT:
            {
                g_ProgramShouldShutdown = true;
                break;
            }
            case SDL_KEYDOWN:
            {
                if (event.key.keysym.sym == SDLK_RETURN && SDL_GetModState() & KMOD_LALT)
                {
                    if (SDL_GetWindowFlags(g_SDLWindow) & SDL_WINDOW_FULLSCREEN_DESKTOP)
                        SDL_SetWindowFullscreen(g_SDLWindow, 0);
                    else
                        SDL_SetWindowFullscreen(g_SDLWindow, SDL_WINDOW_FULLSCREEN_DESKTOP); 
                    event.type = 0;
                }

                if (event.key.keysym.sym == SDLK_F5)
                    g_gfx.ChangeEditorIntegerScaleAndInvokeWindowSizeChanged(
                            Gfx::PixelPerfectRenderScale::OneHundredPercent);
                if (event.key.keysym.sym == SDLK_F6)
                    g_gfx.ChangeEditorIntegerScaleAndInvokeWindowSizeChanged(
                            Gfx::PixelPerfectRenderScale::TwoHundredPercent);
                if (event.key.keysym.sym == SDLK_F7)
                    g_gfx.ChangeEditorIntegerScaleAndInvokeWindowSizeChanged(
                            Gfx::PixelPerfectRenderScale::ThreeHundredPercent);
                if (event.key.keysym.sym == SDLK_F8)
                    g_gfx.ChangeEditorIntegerScaleAndInvokeWindowSizeChanged(
                            Gfx::PixelPerfectRenderScale::FourHundredPercent);

                if ((CurrentProgramMode() == MesaProgramMode::Editor || CurrentProgramMode() == MesaProgramMode::Game) && event.key.keysym.sym == SDLK_BACKQUOTE)
                {
                    consoleActive = !consoleActive;
                    continue;
                }

                if (CurrentProgramMode() == MesaProgramMode::BootScreen || consoleActive) 
                {
                    SendInputToConsole(event.key);
                    continue; // console eats all input
                }
                break;
            }
        }

        // Send SDL events to other systems
        Input.ProcessSDLEvent(event);
        if (g_ProgramMode == MesaProgramMode::Editor) EditorSDLProcessEvent(event);
        Gui::ProcessSDLEvent(event);
    }
}

void StartEditor()
{
    if (consoleActive)
        consoleActive = false;

    if (g_ProgramMode == MesaProgramMode::Game)
    {
        TemporaryGameShutdown();
    }

    g_ProgramMode = MesaProgramMode::Editor;

    // TODO(Kevin): get editor w editor h editor s from cached editor data or .ini
    int winw = SDL_WINDOW_STARTING_SIZE_W;
    int winh = SDL_WINDOW_STARTING_SIZE_H;
    int s = 3;
    SDL_SetWindowSize(g_SDLWindow, winw, winh);
    g_gfx.ChangeEditorIntegerScaleAndInvokeWindowSizeChanged((Gfx::PixelPerfectRenderScale)s);
}

//static void StartGameFile()
void StartGameSpace()
{
    if (consoleActive)
        consoleActive = false;

    g_ProgramMode = MesaProgramMode::Game;

    // get game w game h game s from game file
    int w = 320;
    int h = 180;
    SDL_SetWindowSize(g_SDLWindow, 800, 600);
    g_gfx.WindowSizeChanged();
    Gfx::UpdateBasicFrameBufferSize(&g_gfx.renderTargetGame, 320, 240);
    Gfx::UpdateBasicFrameBufferSize(&g_gfx.renderTargetGUI, 320, 240);

    if (!TemporaryGameInit())
    {
        TemporaryGameShutdown();
        StartEditor();
    }
}

static void LoadFantasyConsole()
{
    g_ProgramMode = MesaProgramMode::Editor;//MesaProgramMode::BootScreen;
    std::string welcome = std::string("type 'help'\n");
    SendMessageToConsole(welcome.c_str(), welcome.size());
    // TODO (Kevin): read settings from .ini
    SDL_SetWindowSize(g_SDLWindow, SDL_WINDOW_STARTING_SIZE_W, SDL_WINDOW_STARTING_SIZE_H);
    g_gfx.ChangeEditorIntegerScaleAndInvokeWindowSizeChanged((Gfx::PixelPerfectRenderScale)3);
}

int main(int argc, char* argv[])
{
	InitializeEverything();

    LoadFantasyConsole();

    while (!g_ProgramShouldShutdown)
    {
        if (Time.UpdateDeltaTime() > 0.1f) { continue; } // if delta time is too large, will cause glitches

//        float beforeUpdate = Time.TimeStamp();

        Gui::NewFrame();
        ProcessSDLEvents();

        switch (g_ProgramMode)
        {
            case MesaProgramMode::BootScreen:
                DoBootScreen();
                break;
            case MesaProgramMode::Editor:
                EditorDoGUI();
                break;
            case MesaProgramMode::Game:
                TemporaryGameLoop();
                break;
        }

        static float lastFPSShowTime = Time.time;
        static float framerate = 0.f;
        if (Time.time - lastFPSShowTime > 0.25f)
        {
            framerate = (1.f / Time.deltaTime);
            lastFPSShowTime = Time.time;
        }
//        Gui::PrimitiveTextFmt(0, 9, 9, Gui::Align::Left, "FPS: %d", int(framerate));

        if (consoleActive)
            DoSingleCommandLine();

//        float updateDuration = Time.TimeStamp() - beforeUpdate;
//        printf("update took: %f\n", updateDuration);
//
//        float beforeRender = Time.TimeStamp();
        // RENDER
        {
            switch (g_ProgramMode)
            {
                case MesaProgramMode::BootScreen:
                    g_gfx.RenderEditor();
                    break;
                case MesaProgramMode::Editor:
                    g_gfx.RenderEditor();
                    break;
                case MesaProgramMode::Game:
                    g_gfx.RenderGame();
                    break;
            }
            SDL_GL_SwapWindow(g_SDLWindow);
#if MESA_WINDOWS
            if (SDL_GL_GetSwapInterval() == 1)
            {
                DwmFlush(); // https://github.com/love2d/love/blob/5175b0d1b599ea4c7b929f6b4282dd379fa116b8/src/modules/window/sdl/Window.cpp#L1024
            }
#endif
        }
//        float renderDuration = Time.TimeStamp() - beforeRender;
//        printf("render took: %f\n", renderDuration);

        Input.ResetInputStatesAtEndOfFrame();
    }

    SDL_DestroyWindow(g_SDLWindow);
    SDL_GL_DeleteContext(g_SDLGLContext);
    SDL_Quit();
    return EXIT_SUCCESS;
}