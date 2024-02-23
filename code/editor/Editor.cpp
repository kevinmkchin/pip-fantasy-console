#include "Editor.h"

#include "../FileSystem.h"
#include "../PrintLog.h"
#include "../GameData.h"
#include "../GfxDataTypesAndUtility.h"
#include "../GfxRenderer.h"
#include "../Input.h"
#include "CodeEditor.h"
#include "SpriteEditor.h"
#include "../Console.h"

const static int s_ToolBarHeight = 26;
const static vec4 s_EditorColor1 = vec4(RGBHEXTO1(0x414141), 1.f);
const static vec4 s_EditorColor2 = vec4(RGBHEXTO1(0x6495ed), 1.f);

static Gfx::TextureHandle thBu00_generic_n;
static Gfx::TextureHandle thBu00_generic_h;
static Gfx::TextureHandle thBu00_generic_a;
static Gfx::TextureHandle thBu01_normal;
static Gfx::TextureHandle thBu01_hovered;
static Gfx::TextureHandle thBu01_active;
static Gfx::TextureHandle borders_00;

MesaGUI::ALH *editorLayout = NULL;
MesaGUI::ALH *mainbarLayout = NULL;

MesaGUI::ALH *codeEditorTabLayout = NULL;
MesaGUI::ALH *alh_sprite_editor = NULL;
MesaGUI::ALH *alh_sprite_editor_left_panel = NULL;
MesaGUI::ALH *alh_sprite_editor_right_panel = NULL;
MesaGUI::ALH *alh_sprite_editor_right_panel_top = NULL;
MesaGUI::ALH *alh_sprite_editor_right_panel_bot = NULL;

static CodeEditorString tempCodeEditorStringA;


static void LoadResourcesForEditorGUI()
{
    thBu00_generic_n = Gfx::CreateGPUTextureFromDisk(data_path("bu00_generic.png").c_str());
    thBu00_generic_h = Gfx::CreateGPUTextureFromDisk(data_path("bu00_generic_hovered.png").c_str());
    thBu00_generic_a = Gfx::CreateGPUTextureFromDisk(data_path("bu00_generic_active.png").c_str());
    thBu01_normal = Gfx::CreateGPUTextureFromDisk(data_path("bu01.png").c_str());
    thBu01_hovered = Gfx::CreateGPUTextureFromDisk(data_path("bu01_hovered.png").c_str());
    thBu01_active = Gfx::CreateGPUTextureFromDisk(data_path("bu01_active.png").c_str());
    borders_00 = Gfx::CreateGPUTextureFromDisk(data_path("borders_01.png").c_str());

    gamedata.sprites.push_back(thBu01_normal);
    gamedata.sprites.push_back(thBu01_active);
    gamedata.sprites.push_back(Gfx::CreateGPUTextureFromDisk(data_path("spr_ground_01.png").c_str()));
    gamedata.sprites.push_back(Gfx::CreateGPUTextureFromDisk(data_path("spr_crosshair_00.png").c_str()));
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
    MesaGUI::PrimitivePanel(MesaGUI::UIRect(mainbarLayout->x, mainbarLayout->y, mainbarLayout->w, mainbarLayout->h), vec4(RGBHEXTO1(0xD0CABD),1)); // 0xc9c9ae RGBHEXTO1(0xc6c6c6) RGBHEXTO1(0xd2cabd)

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

void Temp_SaveScript(std::string pathFromWd)
{
    BinaryFileHandle binfile;
    binfile.memory = tempCodeEditorStringA.string;
    binfile.size = tempCodeEditorStringA.stringlen;
    if (WriteFileBinary(binfile, wd_path(pathFromWd).c_str()))
    {
        printf("saved %s\n", wd_path(pathFromWd).c_str());
    }
}

void Temp_LoadScript(std::string pathFromWd)
{
    BinaryFileHandle binfile;
    ReadFileBinary(binfile, wd_path(pathFromWd).c_str());
    if (binfile.memory)
    {
        SetupCodeEditorString(&tempCodeEditorStringA, (char *)binfile.memory, binfile.size);
        FreeFileBinary(binfile);
        printf("loaded %s\n", wd_path(pathFromWd).c_str());
    }
}

#include "../Timer.h"
#include <sstream>
#include <chrono>

#include "../piplang/VM.h"

void Temp_ExecCurrentScript()
{
    //std::ostringstream profilerOutput;
    auto script = std::string(tempCodeEditorStringA.string, tempCodeEditorStringA.stringlen);
    PipLangVM_InitVM();
    PipLangVM_RunScript(script.c_str());
    PipLangVM_FreeVM();
    //RunProfilerOnScript(script, profilerOutput);
    //PrintLog.Message(profilerOutput.str());
}

void InitEditorGUI()
{
    LoadResourcesForEditorGUI();

    GiveMeTheConsole()->bind_cmd("save", Temp_SaveScript);
    GiveMeTheConsole()->bind_cmd("open", Temp_LoadScript);
    GiveMeTheConsole()->bind_cmd("run", Temp_ExecCurrentScript);


    //EditorState *activeEditorState = EditorState::ActiveEditorState();

    gamedata.codePage1 = "";
    //activeEditorState->codePage1 = "str = 'test' fn tick() { print(str) }\nfn draw() { gfx_sprite(0, 0, 0) gfx_sprite(1, 50, 50) }";

    SetupWorldDesigner();

    editorLayout = MesaGUI::NewALH(true);
    mainbarLayout = MesaGUI::NewALH(-1, -1, -1, s_ToolBarHeight, false);
    codeEditorTabLayout = MesaGUI::NewALH(false);
    alh_sprite_editor = MesaGUI::NewALH(false);
    alh_sprite_editor_left_panel = MesaGUI::NewALH(-1, -1, 140, -1, false);
    alh_sprite_editor_right_panel = MesaGUI::NewALH(true);
    alh_sprite_editor_right_panel_top = MesaGUI::NewALH(false);
    alh_sprite_editor_right_panel_bot = MesaGUI::NewALH(-1, -1, -1, 100, false);

    editorLayout->Insert(mainbarLayout);
    editorLayout->Insert(codeEditorTabLayout);

    alh_sprite_editor->Insert(alh_sprite_editor_left_panel);
    alh_sprite_editor->Insert(alh_sprite_editor_right_panel);
    alh_sprite_editor_right_panel->Insert(alh_sprite_editor_right_panel_top);
    alh_sprite_editor_right_panel->Insert(alh_sprite_editor_right_panel_bot);

    tempCodeEditorStringA = GiveMeNewCodeEditorString();
    SetupCodeEditorString(&tempCodeEditorStringA, gamedata.codePage1.c_str(), (u32)gamedata.codePage1.size());
}

static bool IsPointInLayoutRect(ivec2 point, MesaGUI::ALH *layout)
{
    int xmin = layout->x;
    int ymin = layout->y;
    int xmax = layout->x + layout->w - 1;
    int ymax = layout->y + layout->h - 1;
    return xmin <= point.x && point.x <= xmax && ymin <= point.y && point.y <= ymax;
}

static ivec2 WindowCoordinateToCodeEditorCoordinate(ivec2 xy_win)
{
    ivec2 xy_internal = Gfx::GetCoreRenderer()->TransformWindowCoordinateToEditorGUICoordinateSpace(xy_win);
    MesaGUI::BeginZone(MesaGUI::UIRect(codeEditorTabLayout));
    ivec2 xy_in_zone;
    MesaGUI::GetXYInZone(&xy_in_zone.x, &xy_in_zone.y);
    MesaGUI::EndZone();
    ivec2 xy_codeeditorgui = ivec2(xy_internal.x - xy_in_zone.x, xy_internal.y - xy_in_zone.y);
    return xy_codeeditorgui;
}


void EditorSDLProcessEvent(const SDL_Event event)
{
    static bool leftMouseDown = false;

    switch (event.type)
    {
        case SDL_MOUSEBUTTONDOWN:
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                leftMouseDown = true;

                if (MesaGUI::IsActive(g_CodeEditorUIID))
                {
                    ivec2 point_windowcoords = ivec2(event.button.x, event.button.y);
                    ivec2 xy_internal = Gfx::GetCoreRenderer()->TransformWindowCoordinateToEditorGUICoordinateSpace(
                            point_windowcoords);
                    if (IsPointInLayoutRect(xy_internal, codeEditorTabLayout))
                    {
                        ivec2 xy_codeeditorgui = WindowCoordinateToCodeEditorCoordinate(point_windowcoords);
                        SendMouseDownToCodeEditor(&tempCodeEditorStringA, xy_codeeditorgui.x, xy_codeeditorgui.y);
                    }
                }
            }

            break;
        }

        case SDL_MOUSEBUTTONUP:
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                leftMouseDown = false;
            }

            break;
        }

        case SDL_MOUSEMOTION:
        {
            if (MesaGUI::IsActive(g_CodeEditorUIID) && leftMouseDown)
            {
                ivec2 point_windowcoords = ivec2(event.button.x, event.button.y);
                ivec2 xy_internal = Gfx::GetCoreRenderer()->TransformWindowCoordinateToEditorGUICoordinateSpace(
                        point_windowcoords);
                if (IsPointInLayoutRect(xy_internal, codeEditorTabLayout))
                {
                    ivec2 xy_codeeditorgui = WindowCoordinateToCodeEditorCoordinate(point_windowcoords);
                    SendMouseMoveToCodeEditor(&tempCodeEditorStringA, xy_codeeditorgui.x, xy_codeeditorgui.y);
                }
            }

            break;
        }

        case SDL_MOUSEWHEEL:
        {
            const SDL_MouseWheelEvent wheel = event.wheel;
            const u8* keystate = SDL_GetKeyboardState(NULL);
            int xscroll = wheel.x;
            int yscroll = wheel.y;
            if (keystate[SDL_SCANCODE_LSHIFT] | keystate[SDL_SCANCODE_RSHIFT])
            {
                xscroll = wheel.y;
                yscroll = 0;
            }
            SendMouseScrollToCodeEditor(xscroll, yscroll);
            break;
        }

        case SDL_KEYDOWN:
        {
            SDL_KeyboardEvent keyevent = event.key;

            if (MesaGUI::IsActive(g_CodeEditorUIID))
            {
                int key = (int)keyevent.keysym.sym;
                if (keyevent.keysym.mod & KMOD_SHIFT) 
                    key |= STB_TEXTEDIT_K_SHIFT;
                if (keyevent.keysym.mod & KMOD_CTRL)
                    key |= STB_TEXTEDIT_K_CONTROL;
                SendKeyInputToCodeEditor(&tempCodeEditorStringA, key);
            }

            break;
        }
    }
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

    switch (s_ActiveMode)
    {
        case EditorMode::ArtAndAnimation:
        {
            editorLayout->Replace(1, alh_sprite_editor);
            MesaGUI::UpdateMainCanvasALH(editorLayout);
            DoSpriteEditorGUI();
//            MesaGUI::PrimitiveText(20, 100, 9, MesaGUI::TextAlignment::Left, "Art and animation creator coming soon");
            break;
        }
        case EditorMode::EntityDesigner: 
        {
            editorLayout->Replace(1, codeEditorTabLayout);
            MesaGUI::UpdateMainCanvasALH(editorLayout);

            MesaGUI::PrimitivePanel(MesaGUI::UIRect(codeEditorTabLayout), vec4(RGBHEXTO1(0x414141), 1.f));// 0x193342 //s_EditorColor1);
            MesaGUI::BeginZone(MesaGUI::UIRect(codeEditorTabLayout));
            DoCodeEditorGUI(tempCodeEditorStringA);
            gamedata.codePage1 = std::string(tempCodeEditorStringA.string, tempCodeEditorStringA.stringlen);
            MesaGUI::EndZone();

            MesaGUI::UIRect codeEditorBorder = MesaGUI::UIRect(codeEditorTabLayout);
            codeEditorBorder.y -= 6;
            codeEditorBorder.h += 6;
            MesaGUI::PrimitivePanel(codeEditorBorder, 8, borders_00.textureId, 0.4f);

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

    EditorMainBar();

    // MesaGUI::PrimitivePanel(MesaGUI::UIRect(mainbarLayout), vec4(1,0,0,0.5));
    // //MesaGUI::PrimitivePanel(MesaGUI::UIRect(entityDesignerTabLayout), vec4(0,1,0,0.5));
    // MesaGUI::PrimitivePanel(MesaGUI::UIRect(entitySelectorLayout), vec4(0,0,1,0.5));
    // MesaGUI::PrimitivePanel(MesaGUI::UIRect(entityCodeEditorLayout), vec4(1,0,1,0.5));
}
