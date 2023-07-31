#pragma once

#pragma once

#define null_ui_id (-1)
typedef int ui_id;

#include <SDL.h>

#include "MesaMath.h"
#include "MesaCommon.h"
struct vtxt_font;

namespace MesaGUI
{
    enum class TextAlignment
    {
        Left,
        Center,
        Right
    };

    struct Font
    {
        vtxt_font* ptr = nullptr;
        u32 textureId = 0;
    };

    struct UIRect
    {
        UIRect() : x(0), y(0), w(0), h(0) {};
        UIRect(int _x, int _y, int _w, int _h) : x(_x), y(_y), w(_w), h(_h) {};
        int x;
        int y;
        int w;
        int h;
    };

    struct UIStyle
    {
        vec4 buttonNormalColor = vec4(0.18f, 0.18f, 0.18f, 1.f);
        vec4 buttonHoveredColor = vec4(0.08f, 0.08f, 0.08f, 1.f);
        vec4 buttonActiveColor = vec4(0.06f, 0.06f, 0.06f, 1.f);
        Font textFont;
        vec4 textColor = vec4(1.f, 1.f, 1.f, 1.f);

        int paddingTop = 1;
        int paddingBottom = 1;
        int paddingLeft = 1;
        int paddingRight = 1;
 
        vec4 editorWindowBackgroundColor = vec4(0.1f, 0.1f, 0.1f, 0.85f);
    };

    struct UIZone
    {
        ui_id zoneId = null_ui_id;
        UIRect zoneRect;
        int topLeftXOffset;
        int topLeftYOffset;
    };

    ui_id FreshID();
    void Init();
    void NewFrame();
    void SDLProcessEvent(const SDL_Event* evt);
    void Draw();

    void PushUIStyle(UIStyle style);
    void PopUIStyle();
    UIStyle GetActiveUIStyleCopy();
    UIStyle& GetActiveUIStyleReference();


    // Primitive "building block" GUI elements with the most parameters
    void PrimitivePanel(UIRect rect, vec4 colorRGBA);
    void PrimitivePanel(UIRect rect, int cornerWidth, u32 glTextureId = 0, float normalizedCornerSizeInUV = 0.3f);
    bool PrimitiveButton(ui_id id, UIRect rect, vec4 normalColor, vec4 hoveredColor, vec4 activeColor);
    void PrimitiveText(int x, int y, int size, TextAlignment alignment, const char* text);
    void PrimitiveTextFmt(int x, int y, int size, TextAlignment alignment, const char* textFmt, ...);
    void PrimtiveImage(UIRect rect, u32 glTextureId = 0);
    void PrimitiveIntegerInputField(ui_id id, UIRect rect, int* v);
    void PrimitiveFloatInputField(ui_id id, UIRect rect, float* v);
    //void PrimtiveCheckbox(const char* label, );


    bool LabelledButton(UIRect rect, const char* label, TextAlignment textAlignment);


    void BeginZone(UIRect windowRect);
    void GetXYInZone(int *x, int *y);
    void MoveXYInZone(int x, int y);
    void EndZone();

    void EditorText(const char* text);
    bool EditorLabelledButton(const char* label);
    void EditorIncrementableIntegerField(const char* label, int* v, int increment = 1);
    void EditorIncrementableFloatField(const char* label, float* v, float increment = 0.1f);
}

