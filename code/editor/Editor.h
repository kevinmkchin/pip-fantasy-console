#pragma once

#include "SDL.h"
#include <vector>

#include "../MesaCommon.h"
#include "../MesaMath.h"
#include "../MesaIMGUI.h"


bool Temp_StartGameOrEditorButton();

void EditorSDLProcessEvent(const SDL_Event event);
void EditorDoGUI();

extern Gui::ALH *alh_sprite_editor;
extern Gui::ALH *alh_sprite_editor_left_panel;
extern Gui::ALH *alh_sprite_editor_right_panel;
extern Gui::ALH *alh_sprite_editor_right_panel_top;
extern Gui::ALH *alh_sprite_editor_right_panel_bot;


