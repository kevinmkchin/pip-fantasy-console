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


//       row  byte in row
u8 bitmap[32][32*4];

void DoSpriteEditorGUI()
{
    // https://stackoverflow.com/questions/9863969/updating-a-texture-in-opengl-with-glteximage2d
    static bool test = true;
    static Gfx::TextureHandle bt;
    if (test)
    {
        test = false;

        for (int i = 0; i < 32; ++i)
        {
            for (int j = 0; j < 32; ++j)
            {
                bitmap[i][j * 4 + 0] = u8((float)i / 32.f * 255);
                bitmap[i][j * 4 + 1] = u8((float)j / 32.f * 255);
                bitmap[i][j * 4 + 2] = 0xff;
                bitmap[i][j * 4 + 3] = 0xff;
                //vec4((float)i / 32.f, (float)j / 32.f, 0.f, 1.f);
            }
        }

        bt = Gfx::CreateGPUTextureFromBitmap((unsigned char*)bitmap, 32, 32, GL_RGBA, GL_RGBA, GL_NEAREST);
    }

    MesaGUI::PrimitivePanel(alh_sprite_editor_right_panel_top, vec4(0.2,0.2,0.2,1));

    MesaGUI::PrimtiveImage(MesaGUI::UIRect(alh_sprite_editor_right_panel_top->x, alh_sprite_editor_right_panel_top->y, 256, 256), bt.textureId);

    static std::vector<ivec2> fuck;

    if (Input.mouseLeftHasBeenPressed)
    {
        ivec2 click_guispace = Gfx::GetCoreRenderer()->TransformWindowCoordinateToEditorGUICoordinateSpace(Input.mousePos);
        //ivec2 click = ivec2(click_guispace.x - alh_sprite_editor_right_panel_top->x, click_guispace.y - alh_sprite_editor_right_panel_top->y);
        fuck.push_back(click_guispace);
    }

    for (ivec2 a : fuck)
    {
        MesaGUI::PrimitivePanel(MesaGUI::UIRect(a.x, a.y, 10, 10), vec4(0,0,0,1));
    }

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

    MesaGUI::EndZone();

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