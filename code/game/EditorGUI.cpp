#include "EditorGUI.h"

#include "../core/MesaIMGUI.h"

std::string codeSampleBuf = 
               "fn Update(self) { \n"
               "    if (input['left']) {\n"
               "        self['x'] = self['x']-1\n" 
               "    }\n" 
               "    if (input['right']) {\n"
               "        self['x'] = self['x']+1\n" 
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
                        EDITOR_FIXED_INTERNAL_RESOLUTION_W/2-6, EDITOR_FIXED_INTERNAL_RESOLUTION_H - 26),
        vec4(RGB255TO1(101, 124, 140), 1.f));

    MesaGUI::DoText(EDITOR_FIXED_INTERNAL_RESOLUTION_W/2+2, 20+8+2, 9, MesaGUI::TextAlignment::Left, codeSampleBuf.c_str());
}

void DoEditorGUI()
{
    const int assetsViewW = EDITOR_FIXED_INTERNAL_RESOLUTION_W/4 + 28;
    const int entityViewW = EDITOR_FIXED_INTERNAL_RESOLUTION_W/4 - 40;

    MesaGUI::DoPanel(
        MesaGUI::UIRect(6, 20, 
                        assetsViewW, EDITOR_FIXED_INTERNAL_RESOLUTION_H - 26),
        vec4(RGB255TO1(126, 145, 159), 1.f));
    MesaGUI::DoTextUnformatted(8, 32, 9, MesaGUI::TextAlignment::Left, "Search");
    MesaGUI::DoTextUnformatted(8, 42, 9, MesaGUI::TextAlignment::Left, "v entities");
    MesaGUI::DoTextUnformatted(8, 52, 9, MesaGUI::TextAlignment::Left, "  - folders");
    MesaGUI::DoTextUnformatted(8, 62, 9, MesaGUI::TextAlignment::Left, "v sprites");
    MesaGUI::DoTextUnformatted(8, 72, 9, MesaGUI::TextAlignment::Left, "  - folders");
    MesaGUI::DoTextUnformatted(8, 82, 9, MesaGUI::TextAlignment::Left, "v spaces");
    MesaGUI::DoTextUnformatted(8, 92, 9, MesaGUI::TextAlignment::Left, "  - folders");

    MesaGUI::DoPanel(
        MesaGUI::UIRect(6 + assetsViewW + 6, 20,
                        entityViewW, EDITOR_FIXED_INTERNAL_RESOLUTION_H - 26),
        vec4(RGB255TO1(126, 145, 159), 1.f));

    DoCodeEditor();
}
