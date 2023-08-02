#include "core/MesaCommon.h"
#include "core/MesaUtility.h"
#include "core/Timer.h"
#include "core/GfxRenderer.h"

#include <SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "singleheaders/stb_image.h"
#define STB_SPRINTF_IMPLEMENTATION
#include "singleheaders/stb_sprintf.h"
#include "core/InputSystem.h"
#include "core/MesaIMGUI.h"

#include "game/Game.h"
#include "game/script/MesaScript.h"
#include "game/EditorGUI.h"

static SDL_Window* g_SDLWindow;
static SDL_GLContext g_SDLGLContext;
static bool g_ProgramShouldShutdown = false;
static Gfx::CoreRenderer g_gfx;
static bool s_IsEditor = false;

static bool InitializeEverything()
{
    g_ProgramShouldShutdown = false;

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) return false;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    g_SDLWindow = SDL_CreateWindow("Mesa Fantasy Console",
                                   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SDL_WINDOW_STARTING_SIZE_W, SDL_WINDOW_STARTING_SIZE_H,
                                   SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE); // TODO(Kevin): fix MyImGui mouse input error with resizing 

    g_SDLGLContext = SDL_GL_CreateContext(g_SDLWindow);

    if (g_SDLWindow == nullptr || g_SDLGLContext == nullptr) return false;

    SDL_GL_SetSwapInterval(1);

    g_gfx.Init();
    MesaGUI::Init();

    InitializeLanguageCompilerAndRuntime();

	return true;
}

static void ProcessSDLEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        Input.ProcessAllSDLInputEvents(event);
        MesaGUI::SDLProcessEvent(&event);

        // Lower level engine related input
        switch (event.type)
        {
            case SDL_WINDOWEVENT: {
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED: {
                        g_gfx.UpdateBackBufferSize();
                    }break;
                }
            }break;
            case SDL_QUIT: {
                g_ProgramShouldShutdown = true;
            }break;
        }
    }
}

static void StartEditor()
{
    s_IsEditor = true;
    g_gfx.SetGameResolution(EDITOR_FIXED_INTERNAL_RESOLUTION_W, EDITOR_FIXED_INTERNAL_RESOLUTION_H);
    SDL_SetWindowMinimumSize(g_SDLWindow, EDITOR_FIXED_INTERNAL_RESOLUTION_W, EDITOR_FIXED_INTERNAL_RESOLUTION_H);
}

//static void StartGameFile()
static void StartGameSpace()
{
    s_IsEditor = false;
    // get game w and game h from game file
    // g_gfx.SetGameResolution(w, h);
    g_gfx.SetGameResolution(EDITOR_FIXED_INTERNAL_RESOLUTION_W, EDITOR_FIXED_INTERNAL_RESOLUTION_H);
    SDL_SetWindowMinimumSize(g_SDLWindow, EDITOR_FIXED_INTERNAL_RESOLUTION_W, EDITOR_FIXED_INTERNAL_RESOLUTION_H);

    TemporaryGameInit();
}

static void LoadFantasyConsole()
{
    StartEditor();
}

int main(int argc, char* argv[])
{
	InitializeEverything(); 
    TemporaryRunMesaScriptInterpreterOnFile("fib.ms");

    LoadFantasyConsole();

    while (!g_ProgramShouldShutdown)
    {
        if (Time.UpdateDeltaTime() > 0.1f) { continue; } // if delta time is too large, will cause glitches

        MesaGUI::NewFrame();
        ProcessSDLEvents();

        // console_update(Time.unscaledDeltaTime);

        if (s_IsEditor)
            DoEditorGUI();
        else
            TemporaryGameLoop();

/*
        auto sty = MesaGUI::GetActiveUIStyleCopy();
        sty.textColor = vec4(0.f,0.f,0.f,1.f);
        //MesaGUI::PushUIStyle(sty);
        MesaGUI::DoTextUnformatted(30, 30, 8, MesaGUI::TextAlignment::Left, "JOURNEY");
        MesaGUI::DoTextUnformatted(130, 30, 8, MesaGUI::TextAlignment::Left, "COMET");
        MesaGUI::DoTextUnformatted(230, 30, 16, MesaGUI::TextAlignment::Left, "MESA");
        MesaGUI::DoTextUnformatted(330, 30, 8, MesaGUI::TextAlignment::Left, "STAR");
        MesaGUI::DoTextUnformatted(430, 30, 8, MesaGUI::TextAlignment::Left, "HEART");
        MesaGUI::DoTextUnformatted(30, 60, 16, MesaGUI::TextAlignment::Left, "Start Editor");
        MesaGUI::DoTextUnformatted(30, 90, 8, MesaGUI::TextAlignment::Left, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!");
        //MesaGUI::PopUIStyle();
        //MesaGUI::DoButton(MesaGUI::FreshID(), MesaGUI::UIRect(30, 90, 100, 30), vec4(0,0,0,1), vec4(0.5,0.5,0.5,1), vec4(1.f, 1.f, 1.f, 1.f));
        MesaGUI::EditorBeginWindow(MesaGUI::UIRect(30, 120, 200, 200));
        MesaGUI::EditorLabelledButton("Insert Cartridge");
        MesaGUI::EditorEndWindow();
*/

        if (s_IsEditor && MesaGUI::LabelledButton(MesaGUI::UIRect(100, 2, 80, 16), "Start Space", MesaGUI::TextAlignment::Center))
        {
            StartGameSpace();
        }
        else if (!s_IsEditor && MesaGUI::LabelledButton(MesaGUI::UIRect(100, 2, 100, 16), "Back to Editor", MesaGUI::TextAlignment::Center))
        {
            StartEditor();
        }

        static float lastFPSShowTime = Time.time;
        static float framerate = 0.f;
        if (Time.time - lastFPSShowTime > 0.25f)
        {
            framerate = (1.f / Time.deltaTime);
            lastFPSShowTime = Time.time;
        }
        MesaGUI::PrimitiveTextFmt(0, 18, 18, MesaGUI::TextAlignment::Left, "FPS: %d", int(framerate));

        // DrawProfilerGUI();
        g_gfx.Render();
        SDL_GL_SwapWindow(g_SDLWindow);
        Input.ResetInputStatesAtEndOfFrame();
    }

    SDL_DestroyWindow(g_SDLWindow);
    SDL_GL_DeleteContext(g_SDLGLContext);
    SDL_Quit();
    return EXIT_SUCCESS;
}