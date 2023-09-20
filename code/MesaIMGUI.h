#pragma once

#pragma once

#include <SDL.h>

#include <string>

#include "MesaMath.h"
#include "MesaCommon.h"
#include "MesaUtility.h"
struct vtxt_font;

#define null_ui_id (-1)
typedef i64 ui_id;

namespace MesaGUI
{
    extern NiceArray<SDL_Keycode, 32> keyboardInputASCIIKeycodeThisFrame;

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

    void Init();
    void NewFrame();
    void SDLProcessEvent(const SDL_Event* evt);
    void Draw();

    bool Temp_Escape();

    bool IsActive(ui_id id);
    bool IsHovered(ui_id id);
    void SetActive(ui_id id);
    void SetHovered(ui_id id);
    bool MouseWentUp();
    bool MouseWentDown();
    bool MouseInside(const UIRect& rect);

    void PushUIStyle(UIStyle style);
    void PopUIStyle();
    UIStyle GetActiveUIStyleCopy();
    UIStyle& GetActiveUIStyleReference();


    bool Behaviour_Button(ui_id id, UIRect rect);

    bool ImageButton(UIRect rect, u32 normalTexId, u32 hoveredTexId, u32 activeTexId);


    // Primitive "building block" GUI elements with the most parameters
    void PrimitivePanel(UIRect rect, vec4 colorRGBA);
    void PrimitivePanel(UIRect rect, int cornerRadius, vec4 colorRGBA);
    void PrimitivePanel(UIRect rect, u32 glTextureId);
    void PrimitivePanel(UIRect rect, int cornerRadius, u32 glTextureId = 0, float normalizedCornerSizeInUV = 0.3f);
    bool PrimitiveButton(ui_id id, UIRect rect, vec4 normalColor, vec4 hoveredColor, vec4 activeColor, bool activeColorOnClickReleaseFrame = false);
    void PrimitiveText(int x, int y, int size, TextAlignment alignment, const char* text);
    void PrimitiveTextFmt(int x, int y, int size, TextAlignment alignment, const char* textFmt, ...);
    void PrimtiveImage(UIRect rect, u32 glTextureId = 0);
    void PrimitiveIntegerInputField(ui_id id, UIRect rect, int* v);
    void PrimitiveFloatInputField(ui_id id, UIRect rect, float* v);
    //void PrimtiveCheckbox(const char* label, );

    bool LabelledButton(UIRect rect, const char* label, TextAlignment textAlignment);

    /** Zones
     * For aligning UI elements in order like a DearImGui window.
     * TODO Can be set to stop clicks from going through (if i click inside zone, then click doesn't go through to zones or program behind?)
     * TODO think about: 
     * Can be set to capture focus?
     * Can be set to collapse? */
    void BeginZone(UIRect windowRect);
    void GetXYInZone(int *x, int *y);
    void MoveXYInZone(int x, int y);
    void EndZone();

    void EditorText(const char *text);
    bool EditorLabelledButton(const char *label);
    void EditorIncrementableIntegerField(const char *label, int *v, int increment = 1);
    void EditorIncrementableFloatField(const char *label, float *v, float increment = 0.1f);

    // EditorSelectable returns true once when it gets selected
    bool EditorSelectable(const char *label, bool *selected);

    void EditorBeginListBox();
    void EditorEndListBox();
    
}

