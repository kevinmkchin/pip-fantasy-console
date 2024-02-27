#include "SpriteEditor.h"

#include "Editor.h"
#include "../GameData.h"

//#include <direct.h>
#include <ShObjIdl.h>
#include <codecvt>

std::string OpenFilePrompt(u32 fileTypesCount, COMDLG_FILTERSPEC fileTypes[])
{
    std::string filePathString;
    IFileOpenDialog *pFileOpen;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                          IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
    if (SUCCEEDED(hr))
    {
        hr = pFileOpen->SetFileTypes(fileTypesCount, fileTypes);

        // Show the Open dialog box.
        hr = pFileOpen->Show(NULL);
        // Get the file name from the dialog box.
        if (SUCCEEDED(hr))
        {
            IShellItem *pItem;
            hr = pFileOpen->GetResult(&pItem);
            if (SUCCEEDED(hr))
            {
                PWSTR pszFilePath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                if (SUCCEEDED(hr))
                {
                    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converterX;
                    filePathString = converterX.to_bytes(pszFilePath);
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }
    CoUninitialize();
    return filePathString;
}

std::string OpenLoadImageDialog()
{
    COMDLG_FILTERSPEC fileTypes[] = { { L"PNG files", L"*.png" } };
    std::string imagepath = OpenFilePrompt(1, fileTypes);
    printf("%s\n", imagepath.c_str());
    return imagepath;
}

#include "../GfxRenderer.h"
#include "../Input.h"


static spredit_Color *PixelAt(spredit_Frame *frame, i32 x, i32 y)
{
    if (x >= frame->w || y >= frame->h || x < 0 || y < 0)
        return NULL;
    // Note(Kevin): flip y in access so in data first pixel is bottom left,
    // but access is from top left which is nicer
    y = frame->h - y - 1;
    return frame->pixels + (frame->w*y + x);
}

static void UpdateGPUTex(spredit_Frame *frame)
{
    if (frame->gputex.textureId == 0)
    {
        frame->gputex = Gfx::CreateGPUTextureFromBitmap(
                (unsigned char*)frame->pixels, frame->w, frame->h,
                GL_RGBA, GL_RGBA, GL_NEAREST);
    }
    else
    {
        Gfx::UpdateGPUTextureFromBitmap(&frame->gputex, (unsigned char*)frame->pixels, frame->w, frame->h);
    }
}

static void DeleteGPUTex(spredit_Frame *frame)
{
    // TODO(Kevin): delete gpu texture function in GfxDataTypesAndUtility
}

void DoSpriteEditorGUI()
{
    MesaGUI::BeginZone(alh_sprite_editor);
    if (MesaGUI::EditorLabelledButton("Load a new sprite"))
    {
        std::string imagepath = OpenLoadImageDialog();
        if (!imagepath.empty())
        {
            Gfx::TextureHandle sprite = Gfx::CreateGPUTextureFromDisk(imagepath.c_str());
            gamedata.sprites.push_back(sprite);
        }
    }

    MesaGUI::EditorBeginListBox();
    for (size_t i = 0; i < gamedata.sprites.size(); ++i)
    {
        Gfx::TextureHandle sprite = gamedata.sprites.at(i);
        bool selected = false;
        MesaGUI::EditorSelectable(std::to_string(i).c_str(), &selected);
    }
    MesaGUI::EditorEndListBox();

    static int brushSz = 1;
    MesaGUI::EditorIncrementableIntegerField("brush size", &brushSz);

    static float userR = 0.f;
    static float userG = 0.f;
    static float userB = 0.f;
    static float userA = 1.f;
    MesaGUI::EditorIncrementableFloatField("r", &userR);
    MesaGUI::EditorIncrementableFloatField("g", &userG);
    MesaGUI::EditorIncrementableFloatField("b", &userB);
    MesaGUI::EditorIncrementableFloatField("a", &userA);

    int showUserColorX;
    int showUserColorY;
    MesaGUI::GetXYInZone(&showUserColorX, &showUserColorY);
    MesaGUI::PrimitivePanel(MesaGUI::UIRect(showUserColorX, showUserColorY, 32, 32), vec4(userR, userG, userB, userA));
    MesaGUI::MoveXYInZone(0, 32);

    static float panxf = 20;
    static float panyf = 20;
    static i32 zoom = 1;
    //MesaGUI::EditorIncrementableIntegerField()
    MesaGUI::EditorText((std::to_string(zoom*100) + std::string(".0%")).c_str());

    MesaGUI::EndZone();



    static spredit_Frame testsprite{};
    testsprite.w = 16;
    testsprite.h = 16;
    if (testsprite.pixels == 0)
    {
        testsprite.pixels = (spredit_Color*)calloc(testsprite.w * testsprite.h, sizeof(spredit_Color));

        for (int i = 0; i < testsprite.w; ++i)
        {
            for (int j = 0; j < testsprite.h; ++j)
            {
                spredit_Color *p = PixelAt(&testsprite, i, j);
                //p->r = u8((float)i / 32.f * 255);
                //p->g = u8((float)j / 32.f * 255);
                p->r = 0xff;
                p->g = 0xff;
                p->b = 0xff;
                p->a = 0xff;
            }
        }
    }

    if (Input.mouseYScroll)
    {
        if (Input.mouseYScroll > 0)
        {
            switch (zoom)
            {
                case 8: zoom = 12; break;
                case 12: zoom = 16; break;
                case 16: zoom = 24; break;
                case 24: zoom = 32; break;
                case 32: zoom = 48; break;
                case 48: zoom = 64; break;
                default: zoom = GM_min(zoom + 1, 64);
            }
        }
        else
        {
            switch (zoom)
            {
                case 12: zoom = 8; break;
                case 16: zoom = 12; break;
                case 24: zoom = 16; break;
                case 32: zoom = 24; break;
                case 48: zoom = 32; break;
                case 64: zoom = 48; break;
                default: zoom = GM_max(zoom - 1, 1);
            }
        }
    }

    if (Input.mouseMiddlePressed)
    {
        vec2 scaledMDelta = Input.mouseDelta / (float)Gfx::GetCoreRenderer()->GetEditorIntegerScale();
        panxf += scaledMDelta.x;
        panyf += scaledMDelta.y;
    }

    ivec2 click_guispace = Gfx::GetCoreRenderer()->TransformWindowCoordinateToEditorGUICoordinateSpace(Input.mousePos);
    bool mouseOverViewport = click_guispace.x > alh_sprite_editor_right_panel_top->x && click_guispace.y < alh_sprite_editor_right_panel_top->h;
    if (Input.mouseLeftPressed && mouseOverViewport)
    {
        ivec2 click = ivec2(
                click_guispace.x - alh_sprite_editor_right_panel_top->x - (int)panxf,
                click_guispace.y - alh_sprite_editor_right_panel_top->y - (int)panyf);
        //printf("%d, %d\n", click.x, click.y);

        i32 x = click.x/zoom;
        i32 y = click.y/zoom;
        i32 mx = x - brushSz/2;
        i32 my = y - brushSz/2;

        float brushRadius = (float)brushSz/2.f;

        for (int i = 0; i < brushSz; ++i)
        {
            for (int j = 0; j < brushSz; ++j)
            {
                float fx = (float)x;
                float fy = (float)y;
                if (brushSz % 2 == 1)
                {
                    fx += 0.5f;
                    fy += 0.5f;
                }
                float cx = -brushRadius + (float)i + 0.5f;
                float cy = -brushRadius + (float)j + 0.5f;

                if (sqrtf(float(cx*cx + cy*cy)) < float(brushSz/2) + (brushSz % 2 == 1 ? 0.24f : -0.1f))
                {
                    spredit_Color *p = PixelAt(&testsprite, mx+i, my+j);
                    if (p)
                    {
                        p->r = GM_clamp(userR, 0, 1) * 255;
                        p->g = GM_clamp(userG, 0, 1) * 255;
                        p->b = GM_clamp(userB, 0, 1) * 255;
                        p->a = GM_clamp(userA, 0, 1) * 255;
                    }
                }
            }
        }
    }

    UpdateGPUTex(&testsprite);

    MesaGUI::PrimitivePanel(alh_sprite_editor_right_panel_top, vec4(0.2,0.2,0.2,1));
    MesaGUI::PrimtiveImage(MesaGUI::UIRect(
            alh_sprite_editor_right_panel_top->x + (int)panxf,
            alh_sprite_editor_right_panel_top->y + (int)panyf,
            testsprite.w * zoom, testsprite.h * zoom), testsprite.gputex.textureId);

}





/* Trash down here



// static void ChooseEntityForCodeEditor(EntityAsset *entityAsset)
// {
//     InitializeCodeEditorState(&s_ActiveCodeEditorState, false, entityAsset->code.c_str(), (u32)entityAsset->code.size());
// }


    // MesaGUI::PrimitivePanel(MesaGUI::UIRect(entitySelectorLayout), s_EditorColor1);
    // MesaGUI::BeginZone(MesaGUI::UIRect(entitySelectorLayout));

    // EditorState *activeEditorState = EditorState::ActiveEditorState();

    // MesaGUI::EditorBeginListBox();
    // const std::vector<int> entityAssetIdsList = *activeEditorState->RetrieveAllEntityAssetIds();
    // for (size_t i = 0; i < entityAssetIdsList.size(); ++i)
    // {
    //     int entityAssetId = entityAssetIdsList.at(i);
    //     EntityAsset *e = activeEditorState->RetrieveEntityAssetById(entityAssetId);
    //     bool selected = entityAssetId == s_SelectedEntityAssetId;
    //     if (MesaGUI::EditorSelectable(e->name.c_str(), &selected))
    //     {
    //         s_SelectedEntityAssetId = entityAssetId;
    //         ChooseEntityForCodeEditor(e);
    //     }
    // }
    // MesaGUI::EditorEndListBox();

    // if (MesaGUI::EditorLabelledButton("+ new entity asset"))
    // {
    //     EditorState *activeEditorState = EditorState::ActiveEditorState();
    //     int newId = activeEditorState->CreateNewEntityAsset("entity_x");
    //     EntityAsset *newEnt = activeEditorState->RetrieveEntityAssetById(newId);
    //     newEnt->code = "fn Update() { print('new asset bro') }";
    //     s_SelectedEntityAssetId = newId;
    //     ChooseEntityForCodeEditor(newEnt);
    // }

    // MesaGUI::MoveXYInZone(0, 10);

    // if (s_SelectedEntityAssetId > 0 && MesaGUI::EditorLabelledButton("save code"))
    // {
    //     activeEditorState->RetrieveEntityAssetById(s_SelectedEntityAssetId)->code = std::string(s_ActiveCodeEditorState.code_buf, s_ActiveCodeEditorState.code_len);
    //     PrintLog.Message("Saved code changes...");
    // }

    // int abut_x, abut_y;
    // MesaGUI::GetXYInZone(&abut_x, &abut_y);
    // if (s_SelectedEntityAssetId > 0 && EditorButton(4313913109, abut_x, abut_y, 130, 25, "Temp: Choose Sprite"))
    // {
    //     COMDLG_FILTERSPEC fileTypes[] =
    //     {
    //         { L"PNG files", L"*.png" },
    //     };
    //     std::string someshit = OpenFilePrompt(1, fileTypes);
    //     printf(someshit.c_str());
    //     if (!someshit.empty())
    //     {
    //         activeEditorState->RetrieveEntityAssetById(s_SelectedEntityAssetId)->sprite = Gfx::CreateGPUTextureFromDisk(someshit.c_str());
    //     }
    // }

    // MesaGUI::MoveXYInZone(0, 35);
    // MesaGUI::GetXYInZone(&abut_x, &abut_y);
    // if (s_SelectedEntityAssetId > 0)
    // {
    //     Gfx::TextureHandle spr = activeEditorState->RetrieveEntityAssetById(s_SelectedEntityAssetId)->sprite;
    //     if (spr.textureId > 0)
    //     {
    //         MesaGUI::PrimitivePanel(MesaGUI::UIRect(abut_x, abut_y, spr.width, spr.height), spr.textureId);
    //     }
    // }

    // MesaGUI::EndZone();

    //DoEntityConfigurationPanel();

*/