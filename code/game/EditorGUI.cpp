#include "EditorGUI.h"

#include "../core/PrintLog.h"
#include "../core/MesaIMGUI.h"
#include "AssetManager.h"

EntityAsset *s_SelectedEntityAsset = NULL;

// I select an entity template: it's code shows up in the code editor -> a code editor state is created
// I can keep multiple code editor states open at once (multiple tabs, one tab for each entity code)
//     Each CodeEditor state has the buffer to write to, the cursor location, scroll location 
//     (how many lines down from the top?)
// Does the entity code get updated constantly or only when saved?

MesaGUI::CodeEditorState s_ActiveCodeEditorState;

void DoCodeEditor(MesaGUI::CodeEditorState *codeEditorState)
{
    MesaGUI::PrimitivePanel(MesaGUI::UIRect(EDITOR_FIXED_INTERNAL_RESOLUTION_W/2, 20, 
                                            EDITOR_FIXED_INTERNAL_RESOLUTION_W/2-6, EDITOR_FIXED_INTERNAL_RESOLUTION_H - 26),
                     vec4(RGB255TO1(126, 145, 159), 1.f));
                     //vec4(RGB255TO1(101, 124, 140), 1.f));

    MesaGUI::BeginZone(MesaGUI::UIRect(EDITOR_FIXED_INTERNAL_RESOLUTION_W/2, 22, 
                                       EDITOR_FIXED_INTERNAL_RESOLUTION_W/2-8, EDITOR_FIXED_INTERNAL_RESOLUTION_H - 30));

    MesaGUI::EditorCodeEditor(&s_ActiveCodeEditorState, EDITOR_FIXED_INTERNAL_RESOLUTION_W/2-18, EDITOR_FIXED_INTERNAL_RESOLUTION_H-40, s_SelectedEntityAsset != NULL);

    MesaGUI::EndZone();
}

void DoAssetsWindow()
{
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

    std::string text_aefa = "Active Entity Asset: ";
    if (s_SelectedEntityAsset) 
    {
        text_aefa += s_SelectedEntityAsset->name;
    }
    else 
    {
        text_aefa += "NULL";
        s_ActiveCodeEditorState.codeBuf = "";
    }
    MesaGUI::EditorText(text_aefa.c_str());

    std::vector<EntityAsset>* entityAssetList = GetAll_Entity();
    for (size_t i = 0; i < entityAssetList->size(); ++i)
    {
        EntityAsset& e = entityAssetList->at(i);
        if (MesaGUI::EditorLabelledButton(e.name.c_str()))
        {
            s_SelectedEntityAsset = &e;

            s_ActiveCodeEditorState = MesaGUI::CodeEditorState();
            s_ActiveCodeEditorState.codeBuf = s_SelectedEntityAsset->code;
        }
    }
    MesaGUI::MoveXYInZone(0, 10);
    if (s_SelectedEntityAsset && MesaGUI::EditorLabelledButton("Save Code Changes"))
    {
        s_SelectedEntityAsset->code = s_ActiveCodeEditorState.codeBuf;
        PrintLog.Message("Saving code changes...");
    }

    MesaGUI::EndZone();
}

void DoEditorGUI()
{
    static bool doOnce = false;
    if (!doOnce)
    {
        doOnce = true;
        CreateBlankAsset_Entity("entity_0");
        CreateBlankAsset_Entity("entity_1");
        CreateBlankAsset_Entity("entity_2");
        std::vector<EntityAsset>* entityAssets = GetAll_Entity();

        entityAssets->at(0).code = "fn Update(self) { \n"
                   "    if (input['left']) {\n"
                   "        self['x'] = self['x'] - 3\n" 
                   "    }\n" 
                   "    if (input['right']) {\n"
                   "        self['x'] = self['x'] + 3\n" 
                   "    }\n" 
                   "    if (input['up']) {\n"
                   "        self['y'] = self['y'] + 3\n" 
                   "    }\n" 
                   "    if (input['down']) {\n"
                   "        self['y'] = self['y'] - 3\n" 
                   "    }\n" 
                   "}";
        entityAssets->at(1).code = "fn Update() { print('et1 update') }";
        entityAssets->at(2).code = "fn Update() { print('et2 update') }";
    }

    const int assetsViewW = EDITOR_FIXED_INTERNAL_RESOLUTION_W/4 + 28;
    const int entityViewW = EDITOR_FIXED_INTERNAL_RESOLUTION_W/4 - 40;

    DoAssetsWindow();

    MesaGUI::PrimitivePanel(
        MesaGUI::UIRect(6 + assetsViewW + 6, 20,
                        entityViewW, EDITOR_FIXED_INTERNAL_RESOLUTION_H - 26),
        vec4(RGB255TO1(126, 145, 159), 1.f));

    DoCodeEditor(&s_ActiveCodeEditorState);
}
