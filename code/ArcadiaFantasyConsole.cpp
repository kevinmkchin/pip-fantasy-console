#include "core/CoreCommon.h"
#include "core/ArcadiaUtility.h"
#include "core/ArcadiaTimer.h"
#include "core/CoreRenderer.h"

#include <SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "singleheaders/stb_image.h"
#define STB_SPRINTF_IMPLEMENTATION
#include "singleheaders/stb_sprintf.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "singleheaders/stb_truetype.h"
#define VERTEXT_IMPLEMENTATION
#include "singleheaders/vertext.h"

static SDL_Window* g_SDLWindow;
static SDL_GLContext g_SDLGLContext;
static bool g_ProgramShouldShutdown = false;
static CoreRenderer g_gfx;

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

    g_SDLWindow = SDL_CreateWindow("ARCADIA Fantasy Console",
                                   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, EDITOR_FIXED_INTERNAL_RESOLUTION_W, EDITOR_FIXED_INTERNAL_RESOLUTION_H,
                                   SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    g_SDLGLContext = SDL_GL_CreateContext(g_SDLWindow);

    if (g_SDLWindow == nullptr || g_SDLGLContext == nullptr) return false;

    SDL_GL_SetSwapInterval(1);

    g_gfx.Init();



	return true;
}

static void ProcessSDLEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
//        Input.ProcessAllSDLInputEvents(event);
//        KevGui::SDLProcessEvent(&event);

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
    g_gfx.SetGameResolution(EDITOR_FIXED_INTERNAL_RESOLUTION_W, EDITOR_FIXED_INTERNAL_RESOLUTION_H);
}

static void StartGameFile()
{
    // get game w and game h from game file
    // g_gfx.SetGameResolution(w, h);
}

static void LoadFantasyConsole()
{
    StartEditor();
}

int main()
{
	InitializeEverything();

    LoadFantasyConsole();

    while (!g_ProgramShouldShutdown)
    {
        if (Time.UpdateDeltaTime() > 0.1f) { continue; } // if delta time is too large, will cause glitches

        //KevGui::NewFrame();

        ProcessSDLEvents();

//        console_update(Time.unscaledDeltaTime);
//
//        editorRuntime.UpdateEditor(); //game->Update();
//        DrawProfilerGUI();
        g_gfx.Render();

        SDL_GL_SwapWindow(g_SDLWindow);

        //Input.ResetInputStatesAtEndOfFrame();
    }

    SDL_DestroyWindow(g_SDLWindow);
    SDL_GL_DeleteContext(g_SDLGLContext);
    SDL_Quit();
    return EXIT_SUCCESS;
}