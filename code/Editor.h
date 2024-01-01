#pragma once

#include <SDL.h>
#include <vector>

#include "MesaCommon.h"
#include "MesaMath.h"
#include "MesaIMGUI.h"


bool Temp_StartGameOrEditorButton();

void EditorSDLProcessEvent(const SDL_Event event);
void EditorDoGUI();

extern MesaGUI::ALH *alh_sprite_editor;


