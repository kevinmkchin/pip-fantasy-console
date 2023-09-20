#include "EditorGUI.h"

#include "PrintLog.h"
#include "MesaIMGUI.h"
#include "EditorCodeEditor.h"
#include "EditorState.h"
#include "GfxDataTypesAndUtility.h"

const static int s_ToolBarHeight = 26;
const static vec4 s_EditorColor1 = vec4(RGBHEXTO1(0x414141), 1.f);

static Gfx::TextureHandle thBu00_generic_n;
static Gfx::TextureHandle thBu00_generic_h;
static Gfx::TextureHandle thBu00_generic_a;
static Gfx::TextureHandle thBu01_normal;
static Gfx::TextureHandle thBu01_hovered;
static Gfx::TextureHandle thBu01_active;

static void LoadResourcesForEditorGUI()
{
    thBu00_generic_n = Gfx::CreateGPUTextureFromDisk(data_path("bu00_generic.png").c_str());
    thBu00_generic_h = Gfx::CreateGPUTextureFromDisk(data_path("bu00_generic_hovered.png").c_str());
    thBu00_generic_a = Gfx::CreateGPUTextureFromDisk(data_path("bu00_generic_active.png").c_str());
    thBu01_normal = Gfx::CreateGPUTextureFromDisk(data_path("bu01.png").c_str());
    thBu01_hovered = Gfx::CreateGPUTextureFromDisk(data_path("bu01_hovered.png").c_str());
    thBu01_active = Gfx::CreateGPUTextureFromDisk(data_path("bu01_active.png").c_str());
}


enum class EditorMode
{
    ArtAndAnimation,
    EntityDesigner,
    WorldDesigner,
    SoundAndMusic
};

static EditorMode s_ActiveMode = EditorMode::EntityDesigner;


int s_SelectedEntityAssetId = -1;

// I select an entity template: it's code shows up in the code editor -> a code editor state is created
// I can keep multiple code editor states open at once (multiple tabs, one tab for each entity code)
//     Each CodeEditor state has the buffer to write to, the cursor location, scroll location 
//     (how many lines down from the top?)
// Does the entity code get updated constantly or only when saved?

code_editor_state_t s_ActiveCodeEditorState;

void DoCodeEditor(code_editor_state_t *codeEditorState)
{
    MesaGUI::PrimitivePanel(MesaGUI::UIRect(EDITOR_FIXED_INTERNAL_RESOLUTION_W/4 + 1, s_ToolBarHeight, 
                                            EDITOR_FIXED_INTERNAL_RESOLUTION_W * 3 / 4, EDITOR_FIXED_INTERNAL_RESOLUTION_H - 1),
                            s_EditorColor1);
                     //vec4(RGB255TO1(126, 145, 159), 1.f));
                     //vec4(RGB255TO1(101, 124, 140), 1.f));

    MesaGUI::BeginZone(MesaGUI::UIRect(EDITOR_FIXED_INTERNAL_RESOLUTION_W/4, s_ToolBarHeight,
                                       EDITOR_FIXED_INTERNAL_RESOLUTION_W * 3 / 4, EDITOR_FIXED_INTERNAL_RESOLUTION_H));

    EditorCodeEditor(&s_ActiveCodeEditorState, EDITOR_FIXED_INTERNAL_RESOLUTION_W * 3 / 4 - 8, EDITOR_FIXED_INTERNAL_RESOLUTION_H - s_ToolBarHeight - 8, s_SelectedEntityAssetId > 0);

    MesaGUI::EndZone();
}

void DoEntitySelectionPanel()
{
    const int selectionPanelW = EDITOR_FIXED_INTERNAL_RESOLUTION_W/4;
    const int selectionPanelH = EDITOR_FIXED_INTERNAL_RESOLUTION_H/2;

    MesaGUI::PrimitivePanel(MesaGUI::UIRect(0, s_ToolBarHeight, selectionPanelW, selectionPanelH), s_EditorColor1);
    MesaGUI::BeginZone(MesaGUI::UIRect(0, s_ToolBarHeight, selectionPanelW, selectionPanelH));

    // MesaGUI::DoTextUnformatted(8, 32, 9, MesaGUI::TextAlignment::Left, "Search");
    // MesaGUI::DoTextUnformatted(8, 42, 9, MesaGUI::TextAlignment::Left, "v entities");
    // MesaGUI::DoTextUnformatted(8, 52, 9, MesaGUI::TextAlignment::Left, "  - folders");
    // MesaGUI::DoTextUnformatted(8, 62, 9, MesaGUI::TextAlignment::Left, "v sprites");
    // MesaGUI::DoTextUnformatted(8, 72, 9, MesaGUI::TextAlignment::Left, "  - folders");
    // MesaGUI::DoTextUnformatted(8, 82, 9, MesaGUI::TextAlignment::Left, "v spaces");
    // MesaGUI::DoTextUnformatted(8, 92, 9, MesaGUI::TextAlignment::Left, "  - folders");

    EditorState *activeEditorState = EditorState::ActiveEditorState();


    MesaGUI::EditorBeginListBox();
    const std::vector<int> entityAssetIdsList = *activeEditorState->RetrieveAllEntityAssetIds();
    for (size_t i = 0; i < entityAssetIdsList.size(); ++i)
    {
        int entityAssetId = entityAssetIdsList.at(i);
        EntityAsset *e = activeEditorState->RetrieveEntityAssetById(entityAssetId);
        bool selected = entityAssetId == s_SelectedEntityAssetId;
        if (MesaGUI::EditorSelectable(e->name.c_str(), &selected))
        {
            s_SelectedEntityAssetId = entityAssetId;
            InitializeCodeEditorState(&s_ActiveCodeEditorState, false, e->code.c_str(), (u32)e->code.size());
        }
    }
    MesaGUI::EditorEndListBox();

    MesaGUI::MoveXYInZone(0, 10);
    if (s_SelectedEntityAssetId && MesaGUI::EditorLabelledButton("Save Code Changes"))
    {
        activeEditorState->RetrieveEntityAssetById(s_SelectedEntityAssetId)->code = std::string(s_ActiveCodeEditorState.code_buf, s_ActiveCodeEditorState.code_len);
        PrintLog.Message("Saved code changes...");
    }
    if (s_SelectedEntityAssetId && MesaGUI::EditorLabelledButton("New Entity Asset"))
    {
        EditorState *activeEditorState = EditorState::ActiveEditorState();
        int newId = activeEditorState->CreateNewEntityAsset("entity_x");
        activeEditorState->RetrieveEntityAssetById(newId)->code = "fn Update() { print('fucky fucky') }";
        PrintLog.Message("Created new entity asset...");
    }

    MesaGUI::EndZone();
}

void DoEntityConfigurationPanel()
{
    const int configurationPanelW = EDITOR_FIXED_INTERNAL_RESOLUTION_W / 4;
    const int configurationPanelH = EDITOR_FIXED_INTERNAL_RESOLUTION_H - s_ToolBarHeight - (EDITOR_FIXED_INTERNAL_RESOLUTION_H/2);

    MesaGUI::PrimitivePanel(MesaGUI::UIRect(0, s_ToolBarHeight + (EDITOR_FIXED_INTERNAL_RESOLUTION_H / 2) + 1, configurationPanelW, configurationPanelH), s_EditorColor1);
}

void EntityDesigner()
{
    DoEntitySelectionPanel();
    DoEntityConfigurationPanel();
    DoCodeEditor(&s_ActiveCodeEditorState);
}

void WorldDesigner()
{

}

bool EditorButton(ui_id id, int x, int y, int w, int h, const char *text)
{
    bool result = MesaGUI::Behaviour_Button(id, MesaGUI::UIRect(x,y,w,h));

    u32 texId = MesaGUI::IsHovered(id) ? thBu00_generic_h.textureId : thBu00_generic_n.textureId;
    if (MesaGUI::IsActive(id) || result) texId = thBu00_generic_a.textureId;
    MesaGUI::PrimitivePanel(MesaGUI::UIRect(x,y,w,h), 
        5,
        texId,
        5.f/thBu00_generic_n.width);

    MesaGUI::UIStyle style = MesaGUI::GetActiveUIStyleCopy();
    style.textColor = vec4(0.f,0.f,0.f,1.f);
    MesaGUI::PushUIStyle(style);
    int sz = 9;
    MesaGUI::PrimitiveText(x+4, y+5+sz+((MesaGUI::IsActive(id) || result) ? 1 : 0), sz, MesaGUI::TextAlignment::Left, text);
    MesaGUI::PopUIStyle();

    return result;
}

void EditorMainBar()
{
    MesaGUI::PrimitivePanel(MesaGUI::UIRect(0, 0, EDITOR_FIXED_INTERNAL_RESOLUTION_W, s_ToolBarHeight), vec4(RGBHEXTO1(0xd2cabd),1));

    if(s_ActiveMode == EditorMode::ArtAndAnimation)
    {
        MesaGUI::PrimitivePanel(MesaGUI::UIRect(718, 3, thBu01_active.width, thBu01_active.height), thBu01_active.textureId);
    }
    else
    {
        if(MesaGUI::ImageButton(MesaGUI::UIRect(718, 4, thBu01_normal.width, thBu01_normal.height), thBu01_normal.textureId, thBu01_hovered.textureId, thBu01_active.textureId))
        {
            s_ActiveMode = EditorMode::ArtAndAnimation;
        } 
    }

    if(s_ActiveMode == EditorMode::EntityDesigner)
    {
        MesaGUI::PrimitivePanel(MesaGUI::UIRect(752, 3, thBu01_active.width, thBu01_active.height), thBu01_active.textureId);
    }
    else
    {
        if(MesaGUI::ImageButton(MesaGUI::UIRect(752, 4, thBu01_normal.width, thBu01_normal.height), thBu01_normal.textureId, thBu01_hovered.textureId, thBu01_active.textureId))
        {
            s_ActiveMode = EditorMode::EntityDesigner;
        } 
    }

    if(s_ActiveMode == EditorMode::WorldDesigner)
    {
        MesaGUI::PrimitivePanel(MesaGUI::UIRect(786, 3, thBu01_active.width, thBu01_active.height), thBu01_active.textureId);
    }
    else
    {
        if(MesaGUI::ImageButton(MesaGUI::UIRect(786, 4, thBu01_normal.width, thBu01_normal.height), thBu01_normal.textureId, thBu01_hovered.textureId, thBu01_active.textureId))
        {
            s_ActiveMode = EditorMode::WorldDesigner;
        } 
    }

    if(s_ActiveMode == EditorMode::SoundAndMusic)
    {
        MesaGUI::PrimitivePanel(MesaGUI::UIRect(820, 3, thBu01_active.width, thBu01_active.height), thBu01_active.textureId);
    }
    else
    {
        if(MesaGUI::ImageButton(MesaGUI::UIRect(820, 4, thBu01_normal.width, thBu01_normal.height), thBu01_normal.textureId, thBu01_hovered.textureId, thBu01_active.textureId))
        {
            s_ActiveMode = EditorMode::SoundAndMusic;
        } 
    }
}

bool Temp_StartGameOrEditorButton()
{
    return EditorButton(38105130914, 2, 4, 50, 19, "> Start");
}

void DoEditorGUI()
{
    static bool doOnce = false;
    if (!doOnce)
    {
        doOnce = true;

        LoadResourcesForEditorGUI();

        EditorState *activeEditorState = EditorState::ActiveEditorState();
        int aid = activeEditorState->CreateNewEntityAsset("entity_0");
        int bid = activeEditorState->CreateNewEntityAsset("entity_1");
        int cid = activeEditorState->CreateNewEntityAsset("entity_2");

        activeEditorState->RetrieveEntityAssetById(aid)->code = 
                   "fn Update(self) { \n"
                   "    print(time['dt'])\n"
                   "    if (input['left']) {\n"
                   "        self['x'] = self['x'] - 180 * time['dt']\n" 
                   "    }\n" 
                   "    if (input['right']) {\n"
                   "        self['x'] = self['x'] + 180 * time['dt']\n" 
                   "    }\n" 
                   "    if (input['up']) {\n"
                   "        self['y'] = self['y'] + 180 * time['dt']\n" 
                   "    }\n" 
                   "    if (input['down']) {\n"
                   "        self['y'] = self['y'] - 180 * time['dt']\n" 
                   "    }\n" 
                   "}";
        // entityAssets->at(1).code = "                       .,,uod8B8bou,,.\n"
        //                            "              ..,uod8BBBBBBBBBBBBBBBBRPFT?l!i:.\n"
        //                            "         ,=m8BBBBBBBBBBBBBBBRPFT?!||||||||||||||\n"
        //                            "         !...:!TVBBBRPFT||||||||||!!^^\"\"'   ||||\n"
        //                            "         !.......:!?|||||!!^^\"\"'            ||||\n"
        //                            "         !.........||||                     ||||\n"
        //                            "         !.........||||  ##                 ||||\n"
        //                            "         !.........||||  mesa               ||||\n"
        //                            "         !.........||||                     ||||\n"
        //                            "         !.........||||                     ||||\n"
        //                            "         !.........||||                     ||||\n"
        //                            "         `.........||||                    ,||||\n"
        //                            "          .;.......||||               _.-!!|||||\n"
        //                            "   .,uodWBBBBb.....||||       _.-!!|||||||||!:'\n"
        //                            "!YBBBBBBBBBBBBBBb..!|||:..-!!|||||||!iof68BBBBBb.... \n"
        //                            "!..YBBBBBBBBBBBBBBb!!||||||||!iof68BBBBBBRPFT?!::   `.\n"
        //                            "!....YBBBBBBBBBBBBBBbaaitf68BBBBBBRPFT?!:::::::::     `.\n"
        //                            "!......YBBBBBBBBBBBBBBBBBBBRPFT?!::::::;:!^\"`;:::       `.  \n"
        //                            "!........YBBBBBBBBBBRPFT?!::::::::::^''...::::::;         iBBbo.\n"
        //                            "`..........YBRPFT?!::::::::::::::::::::::::;iof68bo.      WBBBBbo.\n"
        //                            "  `..........:::::::::::::::::::::::;iof688888888888b.     `YBBBP^'\n"
        //                            "    `........::::::::::::::::;iof688888888888888888888b.     `\n"
        //                            "      `......:::::::::;iof688888888888888888888888888888b.\n"
        //                            "        `....:::;iof688888888888888888888888888888888899fT!  \n"
        //                            "          `..::!8888888888888888888888888888888899fT|!^\"'   \n"
        //                            "            `' !!988888888888888888888888899fT|!^\"' \n"
        //                            "                `!!8888888888888888899fT|!^\"'\n"
        //                            "                  `!988888888899fT|!^\"'\n"
        //                            "                    `!9899fT|!^\"'\n"
        //                            "                      `!^\"'\n"
        //                            "";
        activeEditorState->RetrieveEntityAssetById(bid)->code = "fn Update() { print('et1 update') }";
        activeEditorState->RetrieveEntityAssetById(cid)->code = "fn Update() { print('et2 update') }";

        AllocateMemoryCodeEditorState(&s_ActiveCodeEditorState);
    }

    EditorMainBar();

    switch (s_ActiveMode)
    {
        case EditorMode::ArtAndAnimation:
        {
            MesaGUI::PrimitiveText(20, 100, 9, MesaGUI::TextAlignment::Left, "Art and animation creator coming soon");
            break;
        }
        case EditorMode::EntityDesigner: 
        {
            EntityDesigner();
            break;
        }
        case EditorMode::WorldDesigner:
        {
            WorldDesigner();
            break;
        }
        case EditorMode::SoundAndMusic:
        {
            MesaGUI::PrimitiveText(20, 100, 9, MesaGUI::TextAlignment::Left, "Sound and music creator coming soon");
            break;
        }
    }
}
