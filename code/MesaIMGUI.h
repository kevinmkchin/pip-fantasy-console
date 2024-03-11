#pragma once

#include <SDL.h>

#include <string>
#include <vector>

#include "MesaMath.h"
#include "MesaCommon.h"
#include "MesaUtility.h"
struct vtxt_font;

#define null_ui_id (-1)
typedef i64 ui_id;

namespace Gui
{
    extern NiceArray<SDL_Keycode, 32> keyboardInputASCIIKeycodeThisFrame;
    extern vec4 style_textColor;

    enum class Align
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
        UIRect(struct ALH *layout);
        int x;
        int y;
        int w;
        int h;
    };

    /* Application API */
    void Init(); // on program start
    void NewFrame(); // before each frame
    void ProcessSDLEvent(const SDL_Event evt); // update input state
    void Draw(); // flush draw requests to GPU

    /* Immediate-mode state helpers */
    bool IsActive(ui_id id);
    bool IsHovered(ui_id id);
    void SetActive(ui_id id);
    void SetHovered(ui_id id);
    bool MouseWentUp();
    bool MouseWentDown();
    bool MouseInside(const UIRect& rect);
    // IsMouseInsideWindow?

    /* Behaviours
    * "building block" behaviours for making interactible GUI elements */
    bool Behaviour_Button(ui_id id, UIRect rect);

    bool ImageButton(UIRect rect, u32 normalTexId, u32 hoveredTexId, u32 activeTexId);
    extern vec3 CodeCharIndexToColor[];
    void PipCode(int x, int y, int size, const char* text);


    /* Primitive "building block" GUI elements with the most parameters
    * */
    void PrimitivePanel(UIRect rect, vec4 colorRGBA);
    void PrimitivePanel(UIRect rect, int cornerRadius, vec4 colorRGBA);
    void PrimitivePanel(UIRect rect, u32 glTextureId);
    void PrimitivePanel(UIRect rect, int cornerRadius, u32 glTextureId = 0, float normalizedCornerSizeInUV = 0.3f);
    bool PrimitiveButton(ui_id id, UIRect rect, vec4 normalColor, vec4 hoveredColor, vec4 activeColor, bool activeColorOnClickReleaseFrame = false);
    void PrimitiveText(int x, int y, int size, Align alignment, const char* text);
    void PrimitiveTextFmt(int x, int y, int size, Align alignment, const char* textFmt, ...);
    void PrimitiveTextMasked(int x, int y, int size, Align alignment, const char* text, UIRect mask, int maskCornerRadius);
    void PrimitiveIntegerInputField(ui_id id, UIRect rect, int* v);
    void PrimitiveFloatInputField(ui_id id, UIRect rect, float* v);
    //void PrimitiveCheckbox(const char* label, );
    bool PrimitiveLabelledButton(UIRect rect, const char* label, Align textAlignment);


    /* Windows
    * For aligning GUI elements like a DearImGui window
    * TODO Can be set to stop clicks from going through (if i click inside zone, then click doesn't go through to zones or program behind?)
    * TODO think about:
    * Can be set to capture focus?
    * Can be set to collapse?
    * */
    void BeginWindow(UIRect windowRect, vec4 bgcolor = vec4(0.05f, 0.05f, 0.05f, 1.f), int depth = -1);
    void EndWindow();
    void GetWindowWidthAndHeight(int *w, int *h);
    void GetXYInWindow(int *x, int *y);
    void MoveXYInWindow(int x, int y);

    /* GUI elements that go inside windows
    * */
    void EditorText(const char *text);
    bool EditorLabelledButton(const char *label);
    void EditorIncrementableIntegerField(const char *label, int *v, int increment = 1);
    void EditorIncrementableFloatField(const char *label, float *v, float increment = 0.1f);

    // EditorSelectable returns true once when it gets selected
    bool EditorSelectable(const char *label, bool *selected);

    void EditorBeginListBox();
    void EditorEndListBox();

    void EditorColorPicker(ui_id id, float *hue, float *saturation, float *value, float *opacity);





    struct ALH
    {
        // if layout has absolute x y then it is not auto layouted
        int x;
        int y;
        bool xauto = true;
        bool yauto = true;

        int w;
        int h;
        bool wauto = true;
        bool hauto = true;

        bool vertical = true;

        void Insert(ALH *layout, int index)
        {
            container.insert(container.begin() + index, layout);
        }

        void Insert(ALH *layout)
        {
            container.push_back(layout);
        }

        void Replace(int index, ALH *layout)
        {
            ASSERT(index < (int)container.size());
            container.at(index) = layout;
        }

        int Count()
        {
            return int(container.size());
        }

        std::vector<ALH*> container;
    };
    
    void UpdateMainCanvasALH(ALH *layout);
    ALH *NewALH(bool vertical);
    ALH *NewALH(int absX, int absY, int absW, int absH, bool vertical);
    void DeleteALH(ALH *layout);
}

