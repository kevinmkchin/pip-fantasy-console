#include "Console.h"

#include "../MesaMain.h"
#include "MesaIMGUI.h"
#include "Timer.h"
#include "InputSystem.h"

void DoBootScreen()
{
    MesaGUI::PrimitivePanel(MesaGUI::UIRect(0, 0, EDITOR_FIXED_INTERNAL_RESOLUTION_W, EDITOR_FIXED_INTERNAL_RESOLUTION_H), 
                            vec4(sinf(Time.time) * 0.5f, cosf(Time.time) * 0.5f, 0.5f, 1));

    if (Input.KeyHasBeenPressed(SDL_SCANCODE_K))
    {
        StartEditor();
    }
}

