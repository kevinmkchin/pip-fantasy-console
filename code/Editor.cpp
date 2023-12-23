#include "Editor.h"

#include "PrintLog.h"
#include "MesaIMGUI.h"
#include "EditorState.h"
#include "GfxDataTypesAndUtility.h"
#include "GfxRenderer.h"
#include "InputSystem.h"
#include "CodeEditor.h"

const static int s_ToolBarHeight = 26;
const static vec4 s_EditorColor1 = vec4(RGBHEXTO1(0x414141), 1.f);
const static vec4 s_EditorColor2 = vec4(RGBHEXTO1(0x6495ed), 1.f);

static Gfx::TextureHandle thBu00_generic_n;
static Gfx::TextureHandle thBu00_generic_h;
static Gfx::TextureHandle thBu00_generic_a;
static Gfx::TextureHandle thBu01_normal;
static Gfx::TextureHandle thBu01_hovered;
static Gfx::TextureHandle thBu01_active;

static MesaGUI::ALH *editorLayout = NULL;
static MesaGUI::ALH *mainbarLayout = NULL;

static MesaGUI::ALH *codeEditorTabLayout = NULL;

static CodeEditorString tempCodeEditorStringA;


static void LoadResourcesForEditorGUI()
{
    thBu00_generic_n = Gfx::CreateGPUTextureFromDisk(data_path("bu00_generic.png").c_str());
    thBu00_generic_h = Gfx::CreateGPUTextureFromDisk(data_path("bu00_generic_hovered.png").c_str());
    thBu00_generic_a = Gfx::CreateGPUTextureFromDisk(data_path("bu00_generic_active.png").c_str());
    thBu01_normal = Gfx::CreateGPUTextureFromDisk(data_path("bu01.png").c_str());
    thBu01_hovered = Gfx::CreateGPUTextureFromDisk(data_path("bu01_hovered.png").c_str());
    thBu01_active = Gfx::CreateGPUTextureFromDisk(data_path("bu01_active.png").c_str());

    EditorState::ActiveEditorState()->sprites.push_back(thBu01_normal);
    EditorState::ActiveEditorState()->sprites.push_back(thBu01_active);
}


enum class EditorMode
{
    ArtAndAnimation,
    EntityDesigner,
    WorldDesigner,
    SoundAndMusic
};

static EditorMode s_ActiveMode = EditorMode::EntityDesigner;

// I select an entity template: it's code shows up in the code editor -> a code editor state is created
// I can keep multiple code editor states open at once (multiple tabs, one tab for each entity code)
//     Each CodeEditor state has the buffer to write to, the cursor location, scroll location 
//     (how many lines down from the top?)
// Does the entity code get updated constantly or only when saved?

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


#include "Editor_WorldEditor.hpp"

void EditorMainBar()
{
    MesaGUI::PrimitivePanel(MesaGUI::UIRect(mainbarLayout->x, mainbarLayout->y, mainbarLayout->w, mainbarLayout->h), vec4(RGBHEXTO1(0xd2cabd),1));

    if(s_ActiveMode == EditorMode::ArtAndAnimation)
        MesaGUI::PrimitivePanel(MesaGUI::UIRect(mainbarLayout->w - 136, 4, thBu01_active.width, thBu01_active.height), thBu01_active.textureId);
    else
        if(Input.KeyHasBeenPressed(SDL_SCANCODE_F1) || MesaGUI::ImageButton(MesaGUI::UIRect(mainbarLayout->w - 136, 4, thBu01_normal.width, thBu01_normal.height), thBu01_normal.textureId, thBu01_hovered.textureId, thBu01_active.textureId))
            s_ActiveMode = EditorMode::ArtAndAnimation;

    if(s_ActiveMode == EditorMode::EntityDesigner)
        MesaGUI::PrimitivePanel(MesaGUI::UIRect(mainbarLayout->w - 102, 4, thBu01_active.width, thBu01_active.height), thBu01_active.textureId);
    else
        if(Input.KeyHasBeenPressed(SDL_SCANCODE_F2) || MesaGUI::ImageButton(MesaGUI::UIRect(mainbarLayout->w - 102, 4, thBu01_normal.width, thBu01_normal.height), thBu01_normal.textureId, thBu01_hovered.textureId, thBu01_active.textureId))
            s_ActiveMode = EditorMode::EntityDesigner;

    if(s_ActiveMode == EditorMode::WorldDesigner)
        MesaGUI::PrimitivePanel(MesaGUI::UIRect(mainbarLayout->w - 68, 4, thBu01_active.width, thBu01_active.height), thBu01_active.textureId);
    else
        if(Input.KeyHasBeenPressed(SDL_SCANCODE_F3) || MesaGUI::ImageButton(MesaGUI::UIRect(mainbarLayout->w - 68, 4, thBu01_normal.width, thBu01_normal.height), thBu01_normal.textureId, thBu01_hovered.textureId, thBu01_active.textureId))
            s_ActiveMode = EditorMode::WorldDesigner;

    if(s_ActiveMode == EditorMode::SoundAndMusic)
        MesaGUI::PrimitivePanel(MesaGUI::UIRect(mainbarLayout->w - 34, 4, thBu01_active.width, thBu01_active.height), thBu01_active.textureId);
    else
        if(Input.KeyHasBeenPressed(SDL_SCANCODE_F4) || MesaGUI::ImageButton(MesaGUI::UIRect(mainbarLayout->w - 34, 4, thBu01_normal.width, thBu01_normal.height), thBu01_normal.textureId, thBu01_hovered.textureId, thBu01_active.textureId))
            s_ActiveMode = EditorMode::SoundAndMusic;
}

bool Temp_StartGameOrEditorButton()
{
    return EditorButton(38105130914, 2, 4, 50, 19, "> play");
}


void InitEditorGUI()
{
    LoadResourcesForEditorGUI();

    EditorState *activeEditorState = EditorState::ActiveEditorState();
    
    //activeEditorState->codePage1 = "abcdef\nghij\nlm\n";
    //activeEditorState->codePage1 = "ab\ncdef\nhijklm\n";
    activeEditorState->codePage1 = "";
    //activeEditorState->codePage1 = "str = 'test' fn tick() { print(str) }\nfn draw() { gfx_sprite(0, 0, 0) gfx_sprite(1, 50, 50) }";

    //activeEditorState->codePage1 = 
    //           "fn Update(self) { \n"
    //           "    print(time['dt'])\n"
    //           "    if (input['left']) {\n"
    //           "        self['x'] = self['x'] - 180 * time['dt']\n" 
    //           "    }\n" 
    //           "    if (input['right']) {\n"
    //           "        self['x'] = self['x'] + 180 * time['dt']\n" 
    //           "    }\n" 
    //           "    if (input['up']) {\n"
    //           "        self['y'] = self['y'] + 180 * time['dt']\n" 
    //           "    }\n" 
    //           "    if (input['down']) {\n"
    //           "        self['y'] = self['y'] - 180 * time['dt']\n" 
    //           "    }\n" 
    //           "}";

    SetupWorldDesigner();

    editorLayout = MesaGUI::NewALH(true);
    mainbarLayout = MesaGUI::NewALH(-1, -1, -1, s_ToolBarHeight, false);
    codeEditorTabLayout = MesaGUI::NewALH(false);

    editorLayout->Insert(mainbarLayout);
    editorLayout->Insert(codeEditorTabLayout);

    tempCodeEditorStringA = GiveMeNewCodeEditorString();
    SetupCodeEditorString(&tempCodeEditorStringA, activeEditorState->codePage1.c_str(), (u32)activeEditorState->codePage1.size());
}

void EditorSDLProcessEvent(const SDL_Event event)
{
    switch (event.type)
    {
        case SDL_MOUSEBUTTONDOWN:
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {

            }
            break;
        }
        case SDL_KEYDOWN:
        {
            SDL_KeyboardEvent keyevent = event.key;
            int key = (int)keyevent.keysym.sym;
            if (keyevent.keysym.mod & KMOD_SHIFT) 
                key |= STB_TEXTEDIT_K_SHIFT;
            if (keyevent.keysym.mod & KMOD_CTRL)
                key |= STB_TEXTEDIT_K_CONTROL;
            SendKeyInputToCodeEditor(&tempCodeEditorStringA, key);
            break;
        }
    }
}

void SetCode(std::string codePage1)
{
}

void EditorDoGUI()
{
    static bool doOnce = false;
    if (!doOnce)
    {
        doOnce = true;
        InitEditorGUI();
    }

    MesaGUI::UpdateMainCanvasALH(editorLayout);

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
            editorLayout->Replace(1, codeEditorTabLayout);
            MesaGUI::UpdateMainCanvasALH(editorLayout);

            MesaGUI::PrimitivePanel(MesaGUI::UIRect(codeEditorTabLayout), s_EditorColor1);
            MesaGUI::BeginZone(MesaGUI::UIRect(codeEditorTabLayout));
            DoCodeEditorGUI(tempCodeEditorStringA);
            MesaGUI::EndZone();

            // if (EditorButton(38105130915, 54, 4, 50, 19, "save code"))
            // {
            //     EditorState::ActiveEditorState()->codePage1 = std::string(tempCodeEditorStringA.string, tempCodeEditorStringA.stringlen);
            //     PrintLog.Message("Saved code changes...");
            // }

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

    // MesaGUI::PrimitivePanel(MesaGUI::UIRect(mainbarLayout), vec4(1,0,0,0.5));
    // //MesaGUI::PrimitivePanel(MesaGUI::UIRect(entityDesignerTabLayout), vec4(0,1,0,0.5));
    // MesaGUI::PrimitivePanel(MesaGUI::UIRect(entitySelectorLayout), vec4(0,0,1,0.5));
    // MesaGUI::PrimitivePanel(MesaGUI::UIRect(entityCodeEditorLayout), vec4(1,0,1,0.5));
}
