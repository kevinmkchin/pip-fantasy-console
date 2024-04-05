#include "SpriteEditor.h"

#include "Editor.h"
#include "SpriteEditorActions.h"
#include "../GameData.h"
#include "../GfxRenderer.h"
#include "../Input.h"

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


SpriteEditorState spreditState;

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

struct Fuck
{
    i32 x;
    i32 y;
    spredit_Color pixel;
};


static int brushSz = 1;
static vec3 activeRGB = vec3();
static float activeOpacity = 1.f;
std::vector<Fuck> restore;

void DoBrushPoint(i32 x, i32 y)
{
    float brushRadius = (float)brushSz/2.f;
    i32 mx = x - brushSz/2;
    i32 my = y - brushSz/2;

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
                spredit_Color *p = PixelAt(&spreditState.frame, mx+i, my+j);
                if (p)
                {
                    // Blend mode: RGB is linearly interpolated, A is added onto where opacity is % from
                    // existing alpha to 255.
                    vec3 existingRGB = {RGB255TO1(p->r, p->g, p->b)};
                    vec3 finalRGB = Lerp(existingRGB, activeRGB, activeOpacity);
                    restore.push_back({ mx+i, my+j, *p });
                    p->r = u8(finalRGB.x * 255.f);
                    p->g = u8(finalRGB.y * 255.f);
                    p->b = u8(finalRGB.z * 255.f);
                    p->a = GM_clamp(p->a + u8(float(255 - p->a) * activeOpacity), 0, 255);
                }
            }
        }
    }
}

// Line code based on Alois Zingl work released under the
// MIT license http://members.chello.at/easyfilter/bresenham.html
void algo_line_continuous(int x0, int y0, int x1, int y1)
{
  int dx =  GM_abs(x1-x0), sx = (x0 < x1 ? 1: -1);
  int dy = -GM_abs(y1-y0), sy = (y0 < y1 ? 1: -1);
  int err = dx+dy, e2;                                  // error value e_xy

  for (;;) {
    DoBrushPoint(x0, y0);
    e2 = 2*err;
    if (e2 >= dy) {                                       // e_xy+e_x > 0
      if (x0 == x1)
        break;
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {                                       // e_xy+e_y < 0
      if (y0 == y1)
        break;
      err += dx;
      y0 += sy;
    }
  }
}

void algo_line_perfect(int x1, int y1, int x2, int y2) // I don't like
{
  bool yaxis;

  // If the height if the line is bigger than the width, we'll iterate
  // over the y-axis.
  if (GM_abs(y2-y1) > GM_abs(x2-x1)) {
    std::swap(x1, y1);
    std::swap(x2, y2);
    yaxis = true;
  }
  else
    yaxis = false;

  const int w = GM_abs(x2-x1)+1;
  const int h = GM_abs(y2-y1)+1;
  const int dx = GM_sign(x2-x1);
  const int dy = GM_sign(y2-y1);

  int e = 0;
  int y = y1;

  // Move x2 one extra pixel to the dx direction so we can use
  // operator!=() instead of operator<(). Here I prefer operator!=()
  // instead of swapping x1 with x2 so the error always start from 0
  // in the origin (x1,y1).
  x2 += dx;

  for (int x=x1; x!=x2; x+=dx) {
    if (yaxis)
      DoBrushPoint(y, x);
    else
      DoBrushPoint(x, y);

    // The error advances "h/w" per each "x" step. As we're using a
    // integer value for "e", we use "w" as the unit.
    e += h;
    if (e >= w) {
      y += dy;
      e -= w;
    }
  }
}

void ResetSpriteEditorState()
{
    spreditState.frame.w = 64;
    spreditState.frame.h = 64;
    spreditState.frame.pixels = (spredit_Color*)
            calloc(spreditState.frame.w * spreditState.frame.h, sizeof(spredit_Color));

    for (int i = 0; i < spreditState.frame.w; ++i)
    {
        for (int j = 0; j < spreditState.frame.h; ++j)
        {
            spredit_Color *p = PixelAt(&spreditState.frame, i, j);
            //p->r = u8((float)i / 32.f * 255);
            //p->g = u8((float)j / 32.f * 255);
            p->r = 0xff;
            p->g = 0xff;
            p->b = 0xff;
            p->a = 0xff;
        }
    }
}

void DoSpriteEditorGUI()
{
    /*
    Gui::BeginWindow(Gui::UIRect(200,100,200,200), vec4(0.05f,0.05f,0.05f,0.5f), 3);

    Gui::EditorText("Hello world!");
    static int in = 3;
    Gui::EditorIncrementableIntegerField("int", &in);
    static float f = 32.05f;
    Gui::EditorIncrementableFloatField("float", &f);

    Gui::EditorBeginListBox();
    static bool a,b,c = false;
    Gui::EditorSelectable("item a", &a);
    Gui::EditorSelectable("item b", &b);
    Gui::EditorSelectable("item c", &c);
    Gui::EditorEndListBox();

    if (Gui::EditorLabelledButton("Load a new sprite"))
    {
        std::string imagepath = OpenLoadImageDialog();
        if (!imagepath.empty())
        {
            Gfx::TextureHandle sprite = Gfx::CreateGPUTextureFromDisk(imagepath.c_str());
            gamedata.sprites.push_back(sprite);
        }
    }

    Gui::EditorBeginListBox();
    for (size_t i = 0; i < gamedata.sprites.size(); ++i)
    {
        Gfx::TextureHandle sprite = gamedata.sprites.at(i);
        bool selected = false;
        Gui::EditorSelectable(std::to_string(i).c_str(), &selected);
    }
    Gui::EditorEndListBox();

    Gui::EndWindow();
    */



    Gui::BeginWindow(alh_sprite_editor_left_panel, vec4(0.05f, 0.05f, 0.05f, 0.5f), 2);

    Gui::EditorText("brush size");
    Gui::EditorIncrementableIntegerField("brush size", &brushSz);

    static float hue = 0.45f;
    static float saturation = 1.f;
    static float value = 1.f;
    Gui::EditorBeginHorizontal();
    for (int i = 0; i < spreditState.palette.size(); ++i)
    {
        spredit_Color c = spreditState.palette.at(i);
        if (Gui::EditorSelectableRect(vec4(RGB255TO1(c.r, c.g, c.b), float(c.a)/255.f), nullptr, 8482371 + i))
        {
            //RGBToHSV(RGB255TO1(c.r, c.g, c.b));
            // activeRGB = vec3(RGB255TO1(c.r, c.g, c.b));
            vec3 hsv = RGBToHSV(RGB255TO1(c.r, c.g, c.b));
            hue = hsv.x;
            saturation = hsv.y;
            value = hsv.z;
            activeOpacity = float(c.a)/255.f;
        }
        if (i > 0 && i % 3 == 1)
        {
            Gui::EditorEndHorizontal();
            Gui::EditorBeginHorizontal();
        }
    }
    Gui::EditorEndHorizontal();

    int showUserColorX;
    int showUserColorY;
    Gui::Window_GetCurrentOffsets(&showUserColorX, &showUserColorY);
    Gui::PrimitivePanel(Gui::UIRect(showUserColorX, showUserColorY + 2, 61, 12), vec4(activeRGB, activeOpacity));
    Gui::Window_StageLastElementDimension(0, 16);

    static float panxf = 20;
    static float panyf = 20;
    static i32 zoom = 1;
    //Gui::EditorIncrementableIntegerField()
    Gui::EditorText((std::to_string(zoom * 100) + std::string(".0%")).c_str());

    if (Gui::EditorLabelledButton("add to palette"))
    {
        vec3 rgbf = HSVToRGB(hue,saturation,value);
        spreditState.palette.push_back({ u8(rgbf.x * 255.f), u8(rgbf.y * 255.f), u8(rgbf.z * 255.f), u8(activeOpacity * 255.f) });
    }

    Gui::EditorColorPicker(0x32f98, &hue, &saturation, &value, &activeOpacity);
    activeRGB = HSVToRGB(hue, saturation, value);

    Gui::EditorSpacer(0, 16);
    Gui::EditorText("Sprites");
    for (auto& sprd : gamedata.spriteData)
    {
        if (Gui::EditorLabelledButton(sprd.name.c_str()))
        {
            spreditState.frame = sprd.frame;
        }
    }
    if (Gui::EditorLabelledButton("new sprite"))
    {
        SpriteData spr;
        spr.frame.w = 32;
        spr.frame.h = 32;
        spr.frame.pixels = (spredit_Color*)
                calloc(spr.frame.w * spr.frame.h, sizeof(spredit_Color));

        for (int i = 0; i < spr.frame.w; ++i)
        {
            for (int j = 0; j < spr.frame.h; ++j)
            {
                spredit_Color *p = PixelAt(&spr.frame, i, j);
                p->r = 0xff;
                p->g = 0xff;
                p->b = 0xff;
                p->a = 0xff;
            }
        }

        spr.name = "newspr";

        gamedata.spriteData.push_back(spr);
    }

    Gui::EndWindow();


    if (spreditState.frame.pixels == 0)
    {
        InitSpriteEditorActionBuffers();

        ResetSpriteEditorState();

        // temp
        spreditState.palette.push_back({ 0xff, 0x00, 0x00, 0xff });
        spreditState.palette.push_back({ 0xff, 0xff, 0x00, 0xff });
        spreditState.palette.push_back({ 0xff, 0x00, 0xff, 0xff });
        spreditState.palette.push_back({ 0x00, 0xff, 0xff, 0xff });
        spreditState.palette.push_back({ 0x00, 0xff, 0x00, 0xff });
        spreditState.palette.push_back({ 0x00, 0x00, 0xff, 0xff });
        spreditState.palette.push_back({ 0x00, 0x41, 0x41, 0xff });
        spreditState.palette.push_back({ 0x41, 0x00, 0xff, 0xff });
        spreditState.palette.push_back({ 0x00, 0x41, 0xff, 0xff });
        spreditState.palette.push_back({ 0xff, 0x41, 0x41, 0xff });
        spreditState.palette.push_back({ 0x41, 0x00, 0x41, 0xff });
    }

    if (Input.mouseYScroll)
    {
        if (Input.KeyPressed(SDL_SCANCODE_LCTRL))
        {
            if (Input.mouseYScroll > 0)
                ++brushSz;
            else
                --brushSz;
            brushSz = GM_clamp(brushSz, 1, 256);
        }
        else
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
    }

    if (Input.mouseMiddlePressed)
    {
        vec2 scaledMDelta = Input.mouseDelta / (float)Gfx::GetCoreRenderer()->GetEditorIntegerScale();
        panxf += scaledMDelta.x;
        panyf += scaledMDelta.y;
    }

    ivec2 click_guispace = Gfx::GetCoreRenderer()->TransformWindowCoordinateToEditorGUICoordinateSpace(Input.mousePos);
    bool mouseOverViewport = click_guispace.x > alh_sprite_editor_right_panel_top->x && click_guispace.y < alh_sprite_editor_right_panel_top->h;


    if (Input.KeyHasBeenPressed(SDL_SCANCODE_Z))
    {
        Undo(&spreditState);
    }

    if (Input.KeyHasBeenPressed(SDL_SCANCODE_Y))
    {
        Redo(&spreditState);
    }

    static float t;
    static i32 x0 = -1;
    static i32 y0 = -1;

    static spredit_Color *pixelsBeforeAction = nullptr;
    if (Input.mouseLeftHasBeenPressed && mouseOverViewport)
    {
        if (pixelsBeforeAction) delete pixelsBeforeAction;
        size_t sz = sizeof(spredit_Color) * spreditState.frame.w * spreditState.frame.h;
        pixelsBeforeAction = (spredit_Color*)malloc(sz);
        memcpy(pixelsBeforeAction, spreditState.frame.pixels, sz);
    }

    if (Input.mouseLeftHasBeenReleased && mouseOverViewport)
    {
        RecordPixelsWrite(pixelsBeforeAction, spreditState.frame.pixels, spreditState.frame.w, spreditState.frame.h);
        ClearRedoBuffer();
    }

    if (mouseOverViewport)
    {
        // TODO(Kevin): a "pixel has been touched during this action" look up array basically bit array

        ivec2 click = ivec2(
                click_guispace.x - alh_sprite_editor_right_panel_top->x - (int)panxf,
                click_guispace.y - alh_sprite_editor_right_panel_top->y - (int)panyf);
        //printf("%d, %d\n", click.x, click.y);

        i32 x = click.x/zoom;
        i32 y = click.y/zoom;

        if (Input.mouseLeftHasBeenPressed)
        {
            x0 = x;
            y0 = y;
        }

        // Eye dropper
        if (Input.KeyHasBeenPressed(SDL_SCANCODE_J))
        {
            // Note(Kevin): once I add layers, need to add up the color under mouse from each layer
            spredit_Color *p = PixelAt(&spreditState.frame, x, y);
            if (p)
            {
                vec3 hsv = RGBToHSV(RGB255TO1(p->r, p->g, p->b));
                hue = hsv.x;
                saturation = hsv.y;
                value = hsv.z;
                activeOpacity = float(p->a)/255.f;
            }
        }

        if (Input.mouseLeftPressed)
        {
            algo_line_continuous(x0, y0, x, y);
        }
        else
        {
            DoBrushPoint(x, y);
        }

        x0 = x;
        y0 = y;
    }

    UpdateGPUTex(&spreditState.frame);

//    Gui::BeginWindow(alh_sprite_editor_right_panel_top, vec4(0.157f, 0.172f, 0.204f, 1.f));
    Gui::BeginWindow(alh_sprite_editor, vec4(0.157f, 0.172f, 0.204f, 1.f));
    Gui::PrimitivePanel(Gui::UIRect(
            alh_sprite_editor_right_panel_top->x + (int)panxf - 1,
            alh_sprite_editor_right_panel_top->y + (int)panyf - 1,
            spreditState.frame.w * zoom + 2,
            spreditState.frame.h * zoom + 2), vec4(0.f,0.f,0.f,1.f));
    Gui::PrimitivePanel(Gui::UIRect(
            alh_sprite_editor_right_panel_top->x + (int)panxf,
            alh_sprite_editor_right_panel_top->y + (int)panyf,
            spreditState.frame.w * zoom, spreditState.frame.h * zoom), spreditState.frame.gputex.textureId);
    Gui::EndWindow();

    if (!Input.mouseLeftPressed && mouseOverViewport)
    {
        for (auto p : restore)
        {
            *PixelAt(&spreditState.frame, p.x, p.y) = p.pixel;
        }
    }
    restore.clear();
}
