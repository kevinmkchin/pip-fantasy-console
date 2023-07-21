#include "EditorGUI.h"

#include "../core/MesaIMGUI.h"

std::string codeSampleBuf = 
               "fn Update(self) { \n"
               "    if (input['left']) {\n"
               "        self['x'] = self['x'] - 1\n" 
               "    }\n" 
               "    if (input['right']) {\n"
               "        self['x'] = self['x'] + 1\n" 
               "    }\n" 
               "    if (input['up']) {\n"
               "        self['y'] = self['y'] + 1\n" 
               "    }\n" 
               "    if (input['down']) {\n"
               "        self['y'] = self['y'] - 1\n" 
               "    }\n" 
               "}";

void DoCodeEditor()
{
    MesaGUI::DoPanel(
        MesaGUI::UIRect(EDITOR_FIXED_INTERNAL_RESOLUTION_W/2, 20, 
            EDITOR_FIXED_INTERNAL_RESOLUTION_W/2-6, 340),
        vec4(RGB255TO1(126, 145, 159), 1.f));

    MesaGUI::DoText(EDITOR_FIXED_INTERNAL_RESOLUTION_W/2+2, 20+8+2, 8, MesaGUI::TextAlignment::Left, codeSampleBuf.c_str());
}

