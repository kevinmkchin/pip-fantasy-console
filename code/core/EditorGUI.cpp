#include "EditorGUI.h"

#include "PrintLog.h"
#include "MesaIMGUI.h"
#include "EditorCodeEditor.h"
#include "../game/AssetManager.h"

EntityAsset *s_SelectedEntityAsset = NULL;

// I select an entity template: it's code shows up in the code editor -> a code editor state is created
// I can keep multiple code editor states open at once (multiple tabs, one tab for each entity code)
//     Each CodeEditor state has the buffer to write to, the cursor location, scroll location 
//     (how many lines down from the top?)
// Does the entity code get updated constantly or only when saved?

code_editor_state_t s_ActiveCodeEditorState;

void DoCodeEditor(code_editor_state_t *codeEditorState)
{
    MesaGUI::PrimitivePanel(MesaGUI::UIRect(EDITOR_FIXED_INTERNAL_RESOLUTION_W/2, 0, 
                                            EDITOR_FIXED_INTERNAL_RESOLUTION_W/2, EDITOR_FIXED_INTERNAL_RESOLUTION_H),
                            vec4(RGB255TO1(103, 122, 137), 1.f));
                     //vec4(RGB255TO1(126, 145, 159), 1.f));
                     //vec4(RGB255TO1(101, 124, 140), 1.f));

    MesaGUI::BeginZone(MesaGUI::UIRect(EDITOR_FIXED_INTERNAL_RESOLUTION_W/2, 12, 
                                       EDITOR_FIXED_INTERNAL_RESOLUTION_W/2, EDITOR_FIXED_INTERNAL_RESOLUTION_H-14));

    EditorCodeEditor(&s_ActiveCodeEditorState, EDITOR_FIXED_INTERNAL_RESOLUTION_W/2-8, EDITOR_FIXED_INTERNAL_RESOLUTION_H-20, s_SelectedEntityAsset != NULL);

    MesaGUI::EndZone();
}

void DoAssetsWindow()
{
    const int assetsViewW = EDITOR_FIXED_INTERNAL_RESOLUTION_W/4;

    MesaGUI::BeginZone(MesaGUI::UIRect(6, 20, assetsViewW, EDITOR_FIXED_INTERNAL_RESOLUTION_H - 26));
    MesaGUI::PrimitivePanel(MesaGUI::UIRect(0, 0, assetsViewW, EDITOR_FIXED_INTERNAL_RESOLUTION_H), vec4(RGB255TO1(126, 145, 159), 1.f));

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
    }
    MesaGUI::EditorText(text_aefa.c_str());

    std::vector<EntityAsset>* entityAssetList = GetAll_Entity();
    for (size_t i = 0; i < entityAssetList->size(); ++i)
    {
        EntityAsset& e = entityAssetList->at(i);
        if (MesaGUI::EditorLabelledButton(e.name.c_str()))
        {
            s_SelectedEntityAsset = &e;

            InitializeCodeEditorState(&s_ActiveCodeEditorState, false, s_SelectedEntityAsset->code.c_str(), (u32)s_SelectedEntityAsset->code.size());
        }
    }
    MesaGUI::MoveXYInZone(0, 10);
    if (s_SelectedEntityAsset && MesaGUI::EditorLabelledButton("Save Code Changes"))
    {
        s_SelectedEntityAsset->code = std::string(s_ActiveCodeEditorState.code_buf);
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
        std::vector<EntityAsset> *entityAssets = GetAll_Entity();

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
        entityAssets->at(1).code = "                       .,,uod8B8bou,,.\n"
                                   "              ..,uod8BBBBBBBBBBBBBBBBRPFT?l!i:.\n"
                                   "         ,=m8BBBBBBBBBBBBBBBRPFT?!||||||||||||||\n"
                                   "         !...:!TVBBBRPFT||||||||||!!^^\"\"'   ||||\n"
                                   "         !.......:!?|||||!!^^\"\"'            ||||\n"
                                   "         !.........||||                     ||||\n"
                                   "         !.........||||  ##                 ||||\n"
                                   "         !.........||||  mesa               ||||\n"
                                   "         !.........||||                     ||||\n"
                                   "         !.........||||                     ||||\n"
                                   "         !.........||||                     ||||\n"
                                   "         `.........||||                    ,||||\n"
                                   "          .;.......||||               _.-!!|||||\n"
                                   "   .,uodWBBBBb.....||||       _.-!!|||||||||!:'\n"
                                   "!YBBBBBBBBBBBBBBb..!|||:..-!!|||||||!iof68BBBBBb.... \n"
                                   "!..YBBBBBBBBBBBBBBb!!||||||||!iof68BBBBBBRPFT?!::   `.\n"
                                   "!....YBBBBBBBBBBBBBBbaaitf68BBBBBBRPFT?!:::::::::     `.\n"
                                   "!......YBBBBBBBBBBBBBBBBBBBRPFT?!::::::;:!^\"`;:::       `.  \n"
                                   "!........YBBBBBBBBBBRPFT?!::::::::::^''...::::::;         iBBbo.\n"
                                   "`..........YBRPFT?!::::::::::::::::::::::::;iof68bo.      WBBBBbo.\n"
                                   "  `..........:::::::::::::::::::::::;iof688888888888b.     `YBBBP^'\n"
                                   "    `........::::::::::::::::;iof688888888888888888888b.     `\n"
                                   "      `......:::::::::;iof688888888888888888888888888888b.\n"
                                   "        `....:::;iof688888888888888888888888888888888899fT!  \n"
                                   "          `..::!8888888888888888888888888888888899fT|!^\"'   \n"
                                   "            `' !!988888888888888888888888899fT|!^\"' \n"
                                   "                `!!8888888888888888899fT|!^\"'\n"
                                   "                  `!988888888899fT|!^\"'\n"
                                   "                    `!9899fT|!^\"'\n"
                                   "                      `!^\"'\n"
                                   "";
        // entityAssets->at(1).code = "fn Update() { print('et1 update') }";
        entityAssets->at(2).code = "fn Update() { print('et2 update') }";

        AllocateMemoryCodeEditorState(&s_ActiveCodeEditorState);
    }

    const int assetsViewW = EDITOR_FIXED_INTERNAL_RESOLUTION_W/4;
    const int entityViewW = EDITOR_FIXED_INTERNAL_RESOLUTION_W/4;

    DoAssetsWindow();

    MesaGUI::PrimitivePanel(
        MesaGUI::UIRect(assetsViewW, 0, entityViewW, EDITOR_FIXED_INTERNAL_RESOLUTION_H),
        vec4(RGB255TO1(103, 122, 137), 1.f));

    DoCodeEditor(&s_ActiveCodeEditorState);
}
