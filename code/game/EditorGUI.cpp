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

// I select an entity template: it's code shows up in the code editor -> a code editor state is created
// I can keep multiple code editor states open at once (multiple tabs, one tab for each entity code)
//     Each CodeEditor state has the buffer to write to, the cursor location, scroll location 
//     (how many lines down from the top?)
// Does the entity code get updated constantly or only when saved?

struct CodeEditorState
{
    std::string codeBuf;
    u32 codeCursor = 0;
    u32 firstVisibleLineNumber = 0;
};

void DoCodeEditor(CodeEditorState *codeEditorState)
{
    MesaGUI::PrimitivePanel(MesaGUI::UIRect(EDITOR_FIXED_INTERNAL_RESOLUTION_W/2, 20, 
                                     EDITOR_FIXED_INTERNAL_RESOLUTION_W/2-6, EDITOR_FIXED_INTERNAL_RESOLUTION_H - 26),
                     vec4(RGB255TO1(126, 145, 159), 1.f));
                     //vec4(RGB255TO1(101, 124, 140), 1.f));

    MesaGUI::BeginZone(MesaGUI::UIRect(EDITOR_FIXED_INTERNAL_RESOLUTION_W/2, 22, 
                                               EDITOR_FIXED_INTERNAL_RESOLUTION_W/2-8, EDITOR_FIXED_INTERNAL_RESOLUTION_H - 30));
    MesaGUI::EndZone();

    MesaGUI::PrimitiveText(EDITOR_FIXED_INTERNAL_RESOLUTION_W/2+2, 20+8+2, 9, MesaGUI::TextAlignment::Left, codeEditorState->codeBuf.c_str());
}

void DoAssetsWindow()
{
    static bool doOnce = false;
    if (!doOnce)
    {
        doOnce = true;
        // Load code editor for some entity type.

    }

    const int assetsViewW = EDITOR_FIXED_INTERNAL_RESOLUTION_W/4 + 28;
    const int entityViewW = EDITOR_FIXED_INTERNAL_RESOLUTION_W/4 - 40;

    MesaGUI::BeginZone(MesaGUI::UIRect(6, 20, assetsViewW, EDITOR_FIXED_INTERNAL_RESOLUTION_H - 26));
    MesaGUI::PrimitivePanel(MesaGUI::UIRect(6, 20, assetsViewW, EDITOR_FIXED_INTERNAL_RESOLUTION_H - 26), vec4(RGB255TO1(126, 145, 159), 1.f));

    // MesaGUI::DoTextUnformatted(8, 32, 9, MesaGUI::TextAlignment::Left, "Search");
    // MesaGUI::DoTextUnformatted(8, 42, 9, MesaGUI::TextAlignment::Left, "v entities");
    // MesaGUI::DoTextUnformatted(8, 52, 9, MesaGUI::TextAlignment::Left, "  - folders");
    // MesaGUI::DoTextUnformatted(8, 62, 9, MesaGUI::TextAlignment::Left, "v sprites");
    // MesaGUI::DoTextUnformatted(8, 72, 9, MesaGUI::TextAlignment::Left, "  - folders");
    // MesaGUI::DoTextUnformatted(8, 82, 9, MesaGUI::TextAlignment::Left, "v spaces");
    // MesaGUI::DoTextUnformatted(8, 92, 9, MesaGUI::TextAlignment::Left, "  - folders");

    for (int i = 0; i < 5; ++i)
    {
        MesaGUI::EditorLabelledButton("Entity");
    }

    MesaGUI::EndZone();
}

void DoEditorGUI()
{

    const int assetsViewW = EDITOR_FIXED_INTERNAL_RESOLUTION_W/4 + 28;
    const int entityViewW = EDITOR_FIXED_INTERNAL_RESOLUTION_W/4 - 40;

    DoAssetsWindow();

    MesaGUI::PrimitivePanel(
        MesaGUI::UIRect(6 + assetsViewW + 6, 20,
                        entityViewW, EDITOR_FIXED_INTERNAL_RESOLUTION_H - 26),
        vec4(RGB255TO1(126, 145, 159), 1.f));

    CodeEditorState codeEdit_0;
    codeEdit_0.codeBuf = codeSampleBuf;
    DoCodeEditor(&codeEdit_0);
}
