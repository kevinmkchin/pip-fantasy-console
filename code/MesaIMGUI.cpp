#include "MesaIMGUI.h"
#include "GUI_DRAWING.H"

#include <stack>
#include <string>

#include "MesaMath.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "singleheaders/stb_truetype.h"
#define VERTEXT_IMPLEMENTATION
#include "singleheaders/vertext.h"
#include "singleheaders/stb_sprintf.h"

#include "MemoryAllocator.h"
#include "GfxRenderer.h"
#include "PrintLog.h"
#include "FileSystem.h"
#include "Input.h"

static ui_id freshIdCounter = 0;
static ui_id FreshID()
{
    return freshIdCounter++;
}

namespace Gui
{
    vec4 style_buttonNormalColor = vec4(0.18f, 0.18f, 0.18f, 1.f);
    vec4 style_buttonHoveredColor = vec4(0.08f, 0.08f, 0.08f, 1.f);
    vec4 style_buttonActiveColor = vec4(0.06f, 0.06f, 0.06f, 1.f);
    Font style_textFont;
    vec4 style_textColor = vec4(1.f, 1.f, 1.f, 1.0f);
    int style_paddingTop = 1;
    int style_paddingBottom = 1;
    int style_paddingLeft = 1;
    int style_paddingRight = 1;
    vec4 style_editorWindowBackgroundColor = vec4(0.1f, 0.1f, 0.1f, 0.85f);


    static NiceArray<vtxt_font, 10> __vtxtLoadedFonts;
    static Font __fonts[32];
    static Font __default_font;
    
    Font FontCreateFromFile(const std::string& fontFilePath, u8 fontSize, bool useNearestFiltering)
    {
        Font fontToReturn;
        vtxt_font fontHandle;

        BinaryFileHandle fontfile;
        ReadFileBinary(fontfile, fontFilePath.c_str());
        ASSERT(fontfile.memory);
        vtxt_init_font(&fontHandle, (u8*)fontfile.memory, fontSize);
        FreeFileBinary(fontfile);

        Gfx::TextureHandle fontTexture = 
            Gfx::CreateGPUTextureFromBitmap(fontHandle.font_atlas.pixels, fontHandle.font_atlas.width, 
                fontHandle.font_atlas.height, GL_RED, GL_RED, (useNearestFiltering ? GL_NEAREST : GL_LINEAR));
        fontToReturn.textureId = fontTexture.textureId;
        free(fontHandle.font_atlas.pixels);

        __vtxtLoadedFonts.PushBack(fontHandle);
        fontToReturn.ptr = &__vtxtLoadedFonts.Back();
        return fontToReturn;
    }

    Font FontCreateFromBitmap(Gfx::TextureHandle bitmapTexture)
    {
        const int bitmapW = bitmapTexture.width;
        const int bitmapH = bitmapTexture.height;
        const int glyphW = bitmapW / 16;
        const int glyphH = bitmapH / 16;

        Font bitmapFont;
        bitmapFont.textureId = bitmapTexture.textureId;

        vtxt_font fontHandle;
        fontHandle.font_height_px = glyphH;
        fontHandle.ascender = float(glyphH);
        fontHandle.descender = 0;
        fontHandle.linegap = 3; // NOTE(Kevin): 2023-12-22 THIS VALUE IS BEING COPIED HARDCODED INTO CODE EDITOR MAKE SURE TO CHANGE AS WELL
        fontHandle.font_atlas.width = bitmapW;
        fontHandle.font_atlas.height = bitmapH;

        for(int codepoint = 0; codepoint < 256; ++codepoint)
        {
            vtxt_glyph *glyph = &fontHandle.glyphs[codepoint];
            glyph->codepoint = codepoint;
            glyph->width = float(glyphW);
            glyph->height = float(glyphH);
            glyph->advance = float(glyphW);
            glyph->offset_x = 0;
            glyph->offset_y = float(-glyphH);
            glyph->min_u = float(codepoint % 16 * glyphW) / bitmapW;
            glyph->min_v = 1.f - (float((codepoint / 16 + 1) * glyphH) / bitmapH);
            glyph->max_u = float((codepoint % 16 + 1) * glyphW) / bitmapW;
            glyph->max_v = 1.f - (float(codepoint / 16 * glyphH) / bitmapH);
        }

        __vtxtLoadedFonts.PushBack(fontHandle);
        bitmapFont.ptr = &__vtxtLoadedFonts.Back();
        return bitmapFont;
    }

    UIRect::UIRect(struct ALH *layout) : x(layout->x), y(layout->y), w(layout->w), h(layout->h) {};

    static char __reservedTextMemory[16000000];
    static u32 __reservedTextMemoryIndexer = 0;
    NiceArray<SDL_Keycode, 32> keyboardInputASCIIKeycodeThisFrame;
    static NiceArray<char, 128> activeTextInputBuffer;

    static ui_id hoveredUI = null_ui_id;
    static ui_id activeUI = null_ui_id;
    static bool anyElementHoveredThisFrame = false;


    static MemoryLinearBuffer drawRequestsFrameStorageBuffer;
#define MESAIMGUI_NEW_DRAW_REQUEST(type) new (MEMORY_LINEAR_ALLOCATE(&drawRequestsFrameStorageBuffer, type)) type()

    struct WindowData
    {
        ui_id zoneId = null_ui_id;
        UIRect zoneRect;
        int topLeftXOffset = 0;
        int topLeftYOffset = 0;
    };
    static std::stack<WindowData> WINDOWSTACK;
    WindowData *CurrentWindow()
    {
        ASSERT(!WINDOWSTACK.empty());
        return &WINDOWSTACK.top();
    }


    bool IsActive(ui_id id)
    {
        return activeUI == id;
    }
    
    bool IsHovered(ui_id id)
    {
        return hoveredUI == id;
    }
    
    void SetActive(ui_id id)
    {
        activeUI = id;
    }
    
    void SetHovered(ui_id id)
    {
        anyElementHoveredThisFrame = true;
        hoveredUI = id;
    }


    static int mousePosX = 0;
    static int mousePosY = 0;
    
    bool MouseWentUp()
    {
        return Input.mouseLeftHasBeenReleased;
    }
    
    bool MouseWentDown()
    {
        return Input.mouseLeftHasBeenPressed;
    }
    
    bool MouseInside(const UIRect& rect)
    {
        int left = rect.x;
        int top = rect.y;
        int right = left + rect.w;
        int bottom = top + rect.h;
        if (left <= mousePosX && mousePosX < right
            && top <= mousePosY && mousePosY < bottom)
        {
            return true;
        }
        else
        {
            return false;
        }
    }




    bool Behaviour_Button(ui_id id, UIRect rect)
    {
        bool result = false;

        if(IsActive(id))
        {
            if(MouseWentUp())
            {
                if(IsHovered(id))
                {
                    result = true;
                }
                SetActive(null_ui_id);
            }
        }
        else if(IsHovered(id))
        {
            if(MouseWentDown())
            {
                SetActive(id);
            }
        }

        if(MouseInside(rect))
        {
            SetHovered(id);
        }

        return result;
    }

    vec3 CodeCharIndexToColor[32000];
    void PipCode(int x, int y, int size, const char* text)
    {
        if (text == NULL) return;

#if INTERNAL_BUILD
        // Giving 500,000 bytes of free space in the frame text buffer to be safe
        if (__reservedTextMemoryIndexer >= ARRAY_COUNT(__reservedTextMemory) - 500000)
        {
            PrintLog.Warning("Attempting to draw text in GUI.cpp but not enough reserved memory to store text.");
            return;
        }
#endif

        char* textBuffer = __reservedTextMemory + __reservedTextMemoryIndexer;
        strcpy(textBuffer, text);
        int numCharactersWritten = (int)strlen(text);
        __reservedTextMemoryIndexer += numCharactersWritten + 1;

        PipCodeDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(PipCodeDrawRequest);
        drawRequest->text = textBuffer;
        drawRequest->size = size;
        drawRequest->x = x;
        drawRequest->y = y;
        drawRequest->font = style_textFont;

        AppendToCurrentDrawRequestsCollection(drawRequest);
    }


    bool ImageButton(UIRect rect, u32 normalTexId, u32 hoveredTexId, u32 activeTexId)
    {
        ui_id id = FreshID();
        bool result = Behaviour_Button(id, rect);

        RectDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(RectDrawRequest);
        drawRequest->rect = rect;
        drawRequest->textureId = IsHovered(id) ? hoveredTexId : normalTexId;
        if (IsActive(id) || result) drawRequest->textureId = activeTexId;

        AppendToCurrentDrawRequestsCollection(drawRequest);

        return result;
    }


    bool PrimitiveButton(ui_id id, UIRect rect, vec4 normalColor, vec4 hoveredColor, vec4 activeColor, bool activeColorOnClickReleaseFrame)
    {
        bool result = Behaviour_Button(id, rect);

        RectDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(RectDrawRequest);
        drawRequest->rect = rect;
        drawRequest->color = IsHovered(id) ? hoveredColor : normalColor;
        if (IsActive(id)) drawRequest->color = activeColor;
        if (result && activeColorOnClickReleaseFrame)
        {
            drawRequest->color = activeColor;
        }

        AppendToCurrentDrawRequestsCollection(drawRequest);

        return result;
    }

    void PrimitivePanel(UIRect rect, vec4 colorRGBA)
    {
        RectDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(RectDrawRequest);
        drawRequest->rect = rect;
        drawRequest->color = colorRGBA;

        AppendToCurrentDrawRequestsCollection(drawRequest);
    }

    void PrimitivePanel(UIRect rect, int cornerRadius, vec4 colorRGBA)
    {
        RoundedCornerRectDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(RoundedCornerRectDrawRequest);
        drawRequest->rect = rect;
        drawRequest->color = colorRGBA;
        drawRequest->radius = cornerRadius;

        AppendToCurrentDrawRequestsCollection(drawRequest);
    }

    void PrimitivePanel(UIRect rect, u32 glTextureId)
    {
        RectDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(RectDrawRequest);
        drawRequest->rect = rect;
        drawRequest->color = vec4(1.f, 0.f, 1.f, 1.f);
        drawRequest->textureId = glTextureId;

        AppendToCurrentDrawRequestsCollection(drawRequest);
    }

    void PrimitivePanel(UIRect rect, int cornerRadius, u32 glTextureId, float normalizedCornerSizeInUV)
    {
        CorneredRectDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(CorneredRectDrawRequest);
        drawRequest->rect = rect;
        drawRequest->color = vec4(1.f, 0.f, 1.f, 1.f);
        drawRequest->textureId = glTextureId;
        drawRequest->radius = cornerRadius;
        drawRequest->normalizedCornerSizeInUV = normalizedCornerSizeInUV;

        AppendToCurrentDrawRequestsCollection(drawRequest);
    }

    void PrimitiveTextFmt(int x, int y, int size, Align alignment, const char* textFmt, ...)
    {
        if (textFmt == NULL) return;

#if INTERNAL_BUILD
        // Giving 500,000 bytes of free space in the frame text buffer to be safe
        if (__reservedTextMemoryIndexer >= ARRAY_COUNT(__reservedTextMemory) - 500000)
        {
            PrintLog.Warning("Attempting to draw text in GUI.cpp but not enough reserved memory to store text.");
            return;
        }
#endif

        va_list argptr;
        char* formattedTextBuffer = __reservedTextMemory + __reservedTextMemoryIndexer;
        va_start(argptr, textFmt);
        int numCharactersWritten = stbsp_vsprintf(formattedTextBuffer, textFmt, argptr);
        va_end(argptr);
        __reservedTextMemoryIndexer += numCharactersWritten + 1;

        TextDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(TextDrawRequest);
        drawRequest->text = formattedTextBuffer;
        drawRequest->size = size;
        drawRequest->x = x;
        drawRequest->y = y;
        drawRequest->alignment = alignment;
        drawRequest->font = style_textFont;
        drawRequest->color = style_textColor;

        AppendToCurrentDrawRequestsCollection(drawRequest);
    }

    void PrimitiveText(int x, int y, int size, Align alignment, const char* text)
    {
        if (text == NULL) return;

#if INTERNAL_BUILD
        // Giving 500,000 bytes of free space in the frame text buffer to be safe
        if (__reservedTextMemoryIndexer >= ARRAY_COUNT(__reservedTextMemory) - 500000)
        {
            PrintLog.Warning("Attempting to draw text in GUI.cpp but not enough reserved memory to store text.");
            return;
        }
#endif

        char* textBuffer = __reservedTextMemory + __reservedTextMemoryIndexer;
        strcpy(textBuffer, text);
        int numCharactersWritten = (int)strlen(text);
        __reservedTextMemoryIndexer += numCharactersWritten + 1;

        TextDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(TextDrawRequest);
        drawRequest->text = textBuffer;
        drawRequest->size = size;
        drawRequest->x = x;
        drawRequest->y = y;
        drawRequest->alignment = alignment;
        drawRequest->font = style_textFont;
        drawRequest->color = style_textColor;

        AppendToCurrentDrawRequestsCollection(drawRequest);
    }

    void PrimitiveTextMasked(int x, int y, int size, Align alignment, const char* text, UIRect mask, int maskCornerRadius)
    {
        if (text == NULL) return;

#if INTERNAL_BUILD
        // Giving 500,000 bytes of free space in the frame text buffer to be safe
        if (__reservedTextMemoryIndexer >= ARRAY_COUNT(__reservedTextMemory) - 500000)
        {
            PrintLog.Warning("Attempting to draw text in GUI.cpp but not enough reserved memory to store text.");
            return;
        }
#endif

        char* textBuffer = __reservedTextMemory + __reservedTextMemoryIndexer;
        strcpy(textBuffer, text);
        int numCharactersWritten = (int)strlen(text);
        __reservedTextMemoryIndexer += numCharactersWritten + 1;

        TextDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(TextDrawRequest);
        drawRequest->text = textBuffer;
        drawRequest->size = size;
        drawRequest->x = x;
        drawRequest->y = y;
        drawRequest->alignment = alignment;
        drawRequest->font = style_textFont;
        drawRequest->color = style_textColor;
        drawRequest->rectMask = mask;
        drawRequest->rectMaskCornerRadius = maskCornerRadius;

        AppendToCurrentDrawRequestsCollection(drawRequest);
    }

    void PrimitiveIntegerInputField(ui_id id, UIRect rect, int* v)
    {
        bool bSetInactiveAndReturnValue = false;

        if (IsActive(id))
        {
            for (int i = 0; i < keyboardInputASCIIKeycodeThisFrame.count; ++i)
            {
                i32 keycodeASCII = keyboardInputASCIIKeycodeThisFrame.At(i);
                if (48 <= keycodeASCII && keycodeASCII <= 57)
                {
                    if (activeTextInputBuffer.count == 1 && activeTextInputBuffer.At(0) == '0')
                    {
                        activeTextInputBuffer.At(0) = char(keycodeASCII);
                    }
                    else if (activeTextInputBuffer.count < 9) // just to prevent integers that are too big
                    {
                        activeTextInputBuffer.PushBack(char(keycodeASCII));
                    }
                }
                else if (keycodeASCII == 45 /* minus sign */ && activeTextInputBuffer.count == 0)
                {
                    activeTextInputBuffer.PushBack(char(keycodeASCII));
                }
                else if (keycodeASCII == SDLK_RETURN)
                {
                    bSetInactiveAndReturnValue = true;
                }
                else if (keycodeASCII == SDLK_BACKSPACE)
                {
                    if (activeTextInputBuffer.count > 0)
                    {
                        activeTextInputBuffer.Back() = '\0';
                        --activeTextInputBuffer.count;
                    }
                }
            }

            if (MouseWentDown() && !MouseInside(rect))
            {
                bSetInactiveAndReturnValue = true;
            }
        }
        else if (IsHovered(id))
        {
            if (MouseWentDown())
            {
                std::string intValueAsString = std::to_string(*v);
                activeTextInputBuffer.ResetToZero();
                memcpy(activeTextInputBuffer.data, intValueAsString.c_str(), intValueAsString.size());
                activeTextInputBuffer.count = (int)intValueAsString.size();
                SetActive(id);
            }
        }

        if (bSetInactiveAndReturnValue)
        {
            int inputtedInteger = 0;
            bool inputIsNotEmpty = activeTextInputBuffer.count > 0;
            bool onlyInputIsMinusSign = activeTextInputBuffer.count == 1 && activeTextInputBuffer.At(0) == '-';
            if (inputIsNotEmpty && !onlyInputIsMinusSign)
            {
                inputtedInteger = std::stoi(activeTextInputBuffer.data);
            }
            *v = inputtedInteger;
            activeTextInputBuffer.ResetCount();
            activeTextInputBuffer.ResetToZero();
            SetActive(null_ui_id);
        }

        if (MouseInside(rect))
        {
            SetHovered(id);
        }

        RectDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(RectDrawRequest);
        drawRequest->rect = rect;
        drawRequest->color = IsActive(id) ? vec4(0.f, 0.f, 0.f, 1.f) : vec4(0.2f, 0.2f, 0.2f, 1.f);//vec4(1.f, 1.f, 1.f, 1.f) : vec4(0.8f, 0.8f, 0.8f, 1.f);

        AppendToCurrentDrawRequestsCollection(drawRequest);

        if (IsActive(id))
        {
            if (activeTextInputBuffer.count > 0)
            {
                PrimitiveText(rect.x + rect.w, rect.y + rect.h, 9, Align::Right, activeTextInputBuffer.data);
            }
        }
        else
        {
            PrimitiveText(rect.x + rect.w, rect.y + rect.h, 9, Align::Right, std::to_string(*v).c_str());
        }
    }

    void PrimitiveFloatInputField(ui_id id, UIRect rect, float* v)
    {
        bool bSetInactiveAndReturnValue = false;

        if (IsActive(id))
        {
            for (int i = 0; i < keyboardInputASCIIKeycodeThisFrame.count; ++i)
            {
                i32 keycodeASCII = keyboardInputASCIIKeycodeThisFrame.At(i);
                if (48 <= keycodeASCII && keycodeASCII <= 57)
                {
                    if (activeTextInputBuffer.count == 1 && activeTextInputBuffer.At(0) == '0')
                    {
                        activeTextInputBuffer.At(0) = char(keycodeASCII);
                    }
                    else if (activeTextInputBuffer.count < 9) // just to prevent floats that are too big
                    {
                        activeTextInputBuffer.PushBack(char(keycodeASCII));
                    }
                }
                else if (keycodeASCII == 45 /* minus sign */
                         && activeTextInputBuffer.count == 0)
                {
                    activeTextInputBuffer.PushBack(char(keycodeASCII));
                }
                else if (keycodeASCII == 46 /* decimal point */
                         && !IsOneOfArray('.', activeTextInputBuffer.data, activeTextInputBuffer.count))
                {
                    activeTextInputBuffer.PushBack(char(keycodeASCII));
                }
                else if (keycodeASCII == SDLK_RETURN)
                {
                    bSetInactiveAndReturnValue = true;
                }
                else if (keycodeASCII == SDLK_BACKSPACE)
                {
                    if (activeTextInputBuffer.count > 0)
                    {
                        activeTextInputBuffer.Back() = '\0';
                        --activeTextInputBuffer.count;
                    }
                }
            }

            if (MouseWentDown() && !MouseInside(rect))
            {
                bSetInactiveAndReturnValue = true;
            }
        }
        else if (IsHovered(id))
        {
            if (MouseWentDown())
            {
                std::string floatValueAsString = std::to_string(*v);
                RemoveCharactersFromEndOfString(floatValueAsString, '0');
                if (floatValueAsString.back() == '.') floatValueAsString.push_back('0');
                activeTextInputBuffer.ResetToZero();
                memcpy(activeTextInputBuffer.data, floatValueAsString.c_str(), floatValueAsString.size());
                activeTextInputBuffer.count = (int)floatValueAsString.size();
                SetActive(id);
            }
        }

        if (bSetInactiveAndReturnValue)
        {
            float inputtedFloat = 0;
            bool inputIsNotEmpty = activeTextInputBuffer.count > 0;
            bool onlyInputIsMinusSignOrDot = activeTextInputBuffer.count == 1
                                             && ISANYOF2(activeTextInputBuffer.At(0), '-', '.');
            if (inputIsNotEmpty && !onlyInputIsMinusSignOrDot)
            {
                inputtedFloat = std::stof(activeTextInputBuffer.data);
            }
            *v = inputtedFloat;
            activeTextInputBuffer.ResetCount();
            activeTextInputBuffer.ResetToZero();
            SetActive(null_ui_id);
        }

        if (MouseInside(rect))
        {
            SetHovered(id);
        }

        RectDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(RectDrawRequest);
        drawRequest->rect = rect;
        drawRequest->color = IsActive(id) ? vec4(0.f, 0.f, 0.f, 1.f) : vec4(0.2f, 0.2f, 0.2f, 1.f);

        AppendToCurrentDrawRequestsCollection(drawRequest);

        if (IsActive(id))
        {
            if (activeTextInputBuffer.count > 0)
            {
                PrimitiveText(rect.x + rect.w, rect.y + rect.h, 14, Align::Right, activeTextInputBuffer.data);
            }
        }
        else
        {
            char cbuf[32];
            stbsp_sprintf(cbuf, "%.2f", *v);
            PrimitiveText(rect.x + rect.w, rect.y + rect.h, 14, Align::Right, cbuf);
        }
    }



    bool PrimitiveLabelledButton(UIRect rect, const char* label, Align textAlignment)
    {
        int ascenderTextSize = style_textFont.ptr->font_height_px;
        float yTextPaddingRatio = (1.f - (float(ascenderTextSize) / float(rect.h))) / 2.f;
        ivec2 textPadding = ivec2(10, (int) roundf(rect.h * yTextPaddingRatio));
        int textX = rect.x + textPadding.x;
        if (textAlignment == Align::Center)
        {
            textX = (int) ((rect.w / 2) + rect.x);
        }
        else if (textAlignment == Align::Right)
        {
            textX = (int) rect.x + rect.w - textPadding.x;
        }

        bool buttonValue = PrimitiveButton(FreshID(), rect, style_buttonNormalColor, style_buttonHoveredColor, style_buttonActiveColor);
        PrimitiveText(textX, rect.y + rect.h - textPadding.y, ascenderTextSize, textAlignment, label);
        return buttonValue;
    }



    void BeginWindow(UIRect windowRect, vec4 bgcolor, int depth)
    {
        GUIDraw_PushDrawCollection(windowRect, depth);

        WindowData windata;
        windata.zoneId = FreshID();
        windata.zoneRect = windowRect;
        windata.topLeftXOffset = 5;
        windata.topLeftYOffset = 5;
        WINDOWSTACK.push(windata);

        PrimitivePanel(windowRect, bgcolor);
    }

    void EndWindow()
    {
        if (!WINDOWSTACK.empty())
        {
            GUIDraw_PopDrawCollection();
            WINDOWSTACK.pop();
        }
        else
        {
            ASSERT(0);
        }
    }

    void GetWindowWidthAndHeight(int *w, int *h)
    {
        *w = CurrentWindow()->zoneRect.w;
        *h = CurrentWindow()->zoneRect.h;
    }

    void GetXYInWindow(int *x, int *y)
    {
        *x = CurrentWindow()->zoneRect.x + CurrentWindow()->topLeftXOffset;
        *y = CurrentWindow()->zoneRect.y + CurrentWindow()->topLeftYOffset;
    }

    void MoveXYInWindow(int x, int y)
    {
        CurrentWindow()->topLeftXOffset += x;
        CurrentWindow()->topLeftYOffset += y;
    }

    void EditorText(const char* text)
    {
        int x;
        int y;
        GetXYInWindow(&x, &y);

        int sz = style_textFont.ptr->font_height_px;

        PrimitiveText(x + style_paddingLeft, y + sz + style_paddingTop, sz, Align::Left, text);

        MoveXYInWindow(0, sz + style_paddingTop + style_paddingBottom);
    }

    bool EditorLabelledButton(const char* label)
    {
        int labelTextSize = style_textFont.ptr->font_height_px;
        float textW;
        float textH;
        vtxt_get_text_bounding_box_info(&textW, &textH, label, style_textFont.ptr, labelTextSize);

        int buttonX;
        int buttonY;
        GetXYInWindow(&buttonX, &buttonY);
        buttonX += style_paddingLeft;
        buttonY += style_paddingTop;
        int buttonW = GM_max((int) textW + 4, 50);
        int buttonH = labelTextSize + 4;

        UIRect buttonRect = UIRect(buttonX, buttonY, buttonW, buttonH);
        bool result = PrimitiveLabelledButton(buttonRect, label, Align::Center);

        MoveXYInWindow(0, style_paddingTop + buttonH + style_paddingBottom);

        return result;
    }

    void EditorIncrementableIntegerField(const char* label, int* v, int increment)
    {
        int x;
        int y;
        GetXYInWindow(&x, &y);

        int w = 50;
        int h = 9 + 4;
        int paddingAbove = style_paddingTop;
        int paddingBelow = style_paddingBottom;

        PrimitivePanel(UIRect(x, y, w-2, h), vec4(0.4f, 0.4f, 0.4f, 1.f));
        PrimitiveIntegerInputField(FreshID(), UIRect(x + 1, y + 1, w-4, h - 2), v);
//        if (PrimitiveButton(FreshID(), UIRect(x + w, y + 1, 20, (h / 2) - 1), ui_ss.top().buttonNormalColor, ui_ss.top().buttonHoveredColor, ui_ss.top().buttonActiveColor))
//        {
//            (*v) += increment;
//        }
//        if (PrimitiveButton(FreshID(), UIRect(x + w, y + (h / 2) + 1, 20, (h / 2) - 1), ui_ss.top().buttonNormalColor, ui_ss.top().buttonHoveredColor, ui_ss.top().buttonActiveColor))
//        {
//            (*v) -= increment;
//        }
//        PrimitiveText(x + w + 22, y + h, 9, Align::Left, label);

        MoveXYInWindow(0, paddingAbove + h + paddingBelow);
    }

    void EditorIncrementableFloatField(const char* label, float* v, float increment)
    {
        int x, y;
        GetXYInWindow(&x, &y);

        int w = 80;
        int h = 20;
        int paddingAbove = style_paddingTop;
        int paddingBelow = style_paddingBottom;

        PrimitivePanel(UIRect(x, y, w-2, h), vec4(0.4f, 0.4f, 0.4f, 1.f));
        PrimitiveFloatInputField(FreshID(), UIRect(x + 1, y + 1, w-4, h - 2), v);
        if (PrimitiveButton(FreshID(), UIRect(x + w, y + 1, 20, (h / 2) - 1), style_buttonNormalColor, style_buttonHoveredColor, style_buttonActiveColor))
        {
            (*v) += increment;
        }
        if (PrimitiveButton(FreshID(), UIRect(x + w, y + (h / 2) + 1, 20, (h / 2) - 1), style_buttonNormalColor, style_buttonHoveredColor, style_buttonActiveColor))
        {
            (*v) -= increment;
        }
        PrimitiveText(x + w + 22, y + h, 20, Align::Left, label);

        MoveXYInWindow(0, paddingAbove + h + paddingBelow);
    }

    bool EditorSelectable(const char *label, bool *selected)
    {
        int x, y;
        GetXYInWindow(&x, &y);

        UIRect selectableRegion = UIRect(x, y, 100, 11);
        if (*selected)
        {
            PrimitivePanel(selectableRegion, vec4(0,0,0,0.4f));
            PrimitiveText(x + 1, y + 10, 9, Align::Left, label);
            MoveXYInWindow(0, 11);
        }
        else
        {
            *selected = PrimitiveButton(FreshID(), selectableRegion, vec4(), vec4(0,0,0,0.2f), vec4(0,0,0,0.4f), true);
            PrimitiveText(x + 1, y + 10, 9, Align::Left, label);
            MoveXYInWindow(0, 11);
            if (*selected)
            {
                return true;
            }
        }
        return false;
    }

    void EditorBeginListBox()
    {

    }
    void EditorEndListBox()
    {

    }

#include "editor/SpriteEditor.h"

    void SetFramePixelColor(spredit_Frame *frame, i32 x, i32 y, spredit_Color color)
    {
        if (x >= frame->w || y >= frame->h || x < 0 || y < 0)
            return;
        *(frame->pixels + (frame->w*y + x)) = color;
    }

    void EditorColorPicker(ui_id id, float *hue, float *saturation, float *value, float *opacity)
    {
        int x, y;
        GetXYInWindow(&x, &y);

        static spredit_Frame chromaselector;
        static spredit_Frame hueselector;
        static spredit_Frame alphaselector;
        if (chromaselector.gputex.textureId == 0)
        {
            chromaselector.w = 61;
            chromaselector.h = 96;
            chromaselector.pixels = (spredit_Color*)calloc(chromaselector.w * chromaselector.h, sizeof(spredit_Color));
            chromaselector.gputex = Gfx::CreateGPUTextureFromBitmap((u8*)chromaselector.pixels, chromaselector.w, chromaselector.h, GL_RGBA, GL_RGBA);

            hueselector.w = chromaselector.w;
            hueselector.h = 12;
            hueselector.pixels = (spredit_Color*)calloc(hueselector.w * hueselector.h, sizeof(spredit_Color));
            hueselector.gputex = Gfx::CreateGPUTextureFromBitmap((u8*)hueselector.pixels, hueselector.w, hueselector.h, GL_RGBA, GL_RGBA);

            alphaselector.w = chromaselector.w;
            alphaselector.h = 12;
            alphaselector.pixels = (spredit_Color*)calloc(alphaselector.w * alphaselector.h, sizeof(spredit_Color));
            alphaselector.gputex = Gfx::CreateGPUTextureFromBitmap((u8*)alphaselector.pixels, alphaselector.w, alphaselector.h, GL_RGBA, GL_RGBA);
        }

        const UIRect chromaselectorrect = UIRect(x, y, chromaselector.w, chromaselector.h);
        const UIRect hueselectorrect = UIRect(x, y + chromaselector.h, hueselector.w, hueselector.h);
        const UIRect alphaselectorrect = UIRect(x, y + chromaselector.h + hueselector.h, alphaselector.w, alphaselector.h);

        if (IsActive(id) && MouseWentUp())
        {
            SetActive(null_ui_id);
        }
        if (MouseWentDown() && IsHovered(id))
        {
            SetActive(id);
        }
        if (MouseInside(chromaselectorrect))
        {
            SetHovered(id);
        }

        // todo(kevin): this is all temporary code to quickly hack
        if (IsActive(id + 1) && MouseWentUp())
        {
            SetActive(null_ui_id);
        }
        if (MouseWentDown() && IsHovered(id + 1))
        {
            SetActive(id + 1);
        }
        if (MouseInside(hueselectorrect))
        {
            SetHovered(id + 1);
        }

        if (IsActive(id + 2) && MouseWentUp())
        {
            SetActive(null_ui_id);
        }
        if (MouseWentDown() && IsHovered(id + 2))
        {
            SetActive(id + 2);
        }
        if (MouseInside(alphaselectorrect))
        {
            SetHovered(id + 2);
        }

        if (IsActive(id))
        {
            i32 chromaSMouseX = GM_clamp(mousePosX - chromaselectorrect.x, 0, chromaselectorrect.w - 1);
            i32 chromaVMouseY = GM_clamp(chromaselectorrect.h - (mousePosY - chromaselectorrect.y) - 1, 0, chromaselectorrect.h - 1);
            *saturation = float(chromaSMouseX) / float (chromaselectorrect.w - 1);
            *value = float(chromaVMouseY) / float (chromaselectorrect.h - 1);
        }
        else if (IsActive(id + 1))
        {
            i32 hueselectormousex = GM_clamp(mousePosX - hueselectorrect.x, 0, hueselectorrect.w - 1);
            *hue = float(hueselectormousex) / float(hueselectorrect.w - 1);
        }
        else if (IsActive(id + 2))
        {
            i32 alphaselectormousex = GM_clamp(mousePosX - alphaselectorrect.x, 0, alphaselectorrect.w - 1);
            *opacity = float(alphaselectormousex) / float(alphaselectorrect.w - 1);
        }

        for (i32 i = 0; i < chromaselector.w; ++i)
        {
            for (i32 j = 0; j < chromaselector.h; ++j)
            {
                float isaturation = float(i) / float(chromaselector.w - 1);
                float ivalue = float(j) / float(chromaselector.h - 1);
                vec3 interprgb = HSVToRGB(*hue, isaturation, ivalue);
                spredit_Color c = {
                        (u8)(255.f * interprgb.x),
                        (u8)(255.f * interprgb.y),
                        (u8)(255.f * interprgb.z),
                        255
                };
                *(chromaselector.pixels + chromaselector.w * j + i) = c;
            }
        }
        spredit_Color selectedchromacirclecolor = { 0,0,0,200 };
        if (*value < 0.5f)
            selectedchromacirclecolor = { 255,255,255,200 };
        i32 left = i32(*saturation * float(chromaselector.w)) - 2;
        i32 bottom = i32(*value * float(chromaselector.h)) - 2;
        SetFramePixelColor(&chromaselector, (left + 0), (bottom + 1), selectedchromacirclecolor);
        SetFramePixelColor(&chromaselector, (left + 0), (bottom + 2), selectedchromacirclecolor);
        SetFramePixelColor(&chromaselector, (left + 3), (bottom + 1), selectedchromacirclecolor);
        SetFramePixelColor(&chromaselector, (left + 3), (bottom + 2), selectedchromacirclecolor);
        SetFramePixelColor(&chromaselector, (left + 1), (bottom + 0), selectedchromacirclecolor);
        SetFramePixelColor(&chromaselector, (left + 2), (bottom + 0), selectedchromacirclecolor);
        SetFramePixelColor(&chromaselector, (left + 1), (bottom + 3), selectedchromacirclecolor);
        SetFramePixelColor(&chromaselector, (left + 2), (bottom + 3), selectedchromacirclecolor);
        Gfx::UpdateGPUTextureFromBitmap(&chromaselector.gputex, (u8*)chromaselector.pixels, chromaselector.w, chromaselector.h);

        for (i32 i = 0; i < hueselector.w; ++i)
        {
            float normalizedhuef = float(i)/float(hueselector.w - 1);
            vec3 irgb = HSVToRGB(normalizedhuef, 1.f, 1.f);
            for (i32 j = 0; j < hueselector.h; ++j)
            {
                SetFramePixelColor(&hueselector, i, j, {
                        u8(irgb.x * 255.f),
                        u8(irgb.y * 255.f),
                        u8(irgb.z * 255.f),
                        255
                });
            }
        }
        i32 selectedhuecirclex = i32(*hue * float(hueselector.w));
        i32 selectedhuecircley = hueselector.h / 2;
        spredit_Color selectedhuecirclecolor = { 10,10,10,180 };
        SetFramePixelColor(&hueselector, selectedhuecirclex-2, selectedhuecircley-1, selectedhuecirclecolor);
        SetFramePixelColor(&hueselector, selectedhuecirclex-2, selectedhuecircley, selectedhuecirclecolor);
        SetFramePixelColor(&hueselector, selectedhuecirclex+1, selectedhuecircley-1, selectedhuecirclecolor);
        SetFramePixelColor(&hueselector, selectedhuecirclex+1, selectedhuecircley, selectedhuecirclecolor);
        SetFramePixelColor(&hueselector, selectedhuecirclex-1, selectedhuecircley-2, selectedhuecirclecolor);
        SetFramePixelColor(&hueselector, selectedhuecirclex, selectedhuecircley-2, selectedhuecirclecolor);
        SetFramePixelColor(&hueselector, selectedhuecirclex-1, selectedhuecircley+1, selectedhuecirclecolor);
        SetFramePixelColor(&hueselector, selectedhuecirclex, selectedhuecircley+1, selectedhuecirclecolor);
        Gfx::UpdateGPUTextureFromBitmap(&hueselector.gputex, (u8*)hueselector.pixels, hueselector.w, hueselector.h);

        for (i32 i = 0; i < alphaselector.w; ++i)
        {
            float normalizedalpha = float(i) / float(alphaselector.w - 1);
            for (i32 j = 0; j < alphaselector.h; ++j)
            {
                vec3 alphaselectorbg;
                if ((i % 16) < 8 != j < (alphaselector.h / 2))
                    alphaselectorbg = { 0.75f, 0.75f, 0.75f };
                else
                    alphaselectorbg = { 0.50f, 0.50f, 0.50f };

                vec3 alphaselectorfg = HSVToRGB(*hue, *saturation, *value);

                vec3 alphaselectorfinalcolor = Lerp(alphaselectorbg, alphaselectorfg, normalizedalpha);

                SetFramePixelColor(&alphaselector, i, j, {
                        u8(alphaselectorfinalcolor.x * 255.f),
                        u8(alphaselectorfinalcolor.y * 255.f),
                        u8(alphaselectorfinalcolor.z * 255.f),
                        255
                });
            }
        }
        i32 selectedalphacirclex = i32(*opacity * float(alphaselector.w));
        i32 selectedalphacircley = alphaselector.h / 2;
        spredit_Color selectedalphacirclecolor = { 10,10,10,180 };
        SetFramePixelColor(&alphaselector, selectedalphacirclex-2, selectedalphacircley-1, selectedalphacirclecolor);
        SetFramePixelColor(&alphaselector, selectedalphacirclex-2, selectedalphacircley, selectedalphacirclecolor);
        SetFramePixelColor(&alphaselector, selectedalphacirclex+1, selectedalphacircley-1, selectedalphacirclecolor);
        SetFramePixelColor(&alphaselector, selectedalphacirclex+1, selectedalphacircley, selectedalphacirclecolor);
        SetFramePixelColor(&alphaselector, selectedalphacirclex-1, selectedalphacircley-2, selectedalphacirclecolor);
        SetFramePixelColor(&alphaselector, selectedalphacirclex, selectedalphacircley-2, selectedalphacirclecolor);
        SetFramePixelColor(&alphaselector, selectedalphacirclex-1, selectedalphacircley+1, selectedalphacirclecolor);
        SetFramePixelColor(&alphaselector, selectedalphacirclex, selectedalphacircley+1, selectedalphacirclecolor);
        Gfx::UpdateGPUTextureFromBitmap(&alphaselector.gputex, (u8*)alphaselector.pixels, alphaselector.w, alphaselector.h);

        PrimitivePanel(chromaselectorrect, chromaselector.gputex.textureId);
        PrimitivePanel(hueselectorrect, hueselector.gputex.textureId);
        PrimitivePanel(alphaselectorrect, alphaselector.gputex.textureId);
    }


    static void UpdateALHContainer(ALH *layout)
    {
        const int lx = layout->x;
        const int ly = layout->y;
        const int lw = layout->w;
        const int lh = layout->h;
        const int lc = layout->Count();

        if (lc == 0) return;

        if (layout->vertical)
        {
            int absHeightSum = 0;
            int elemIgnoredCount = 0;

            for (ALH *child : layout->container)
            {
                if (child->xauto == false || child->yauto == false)
                {
                    ++elemIgnoredCount;
                }
                else if (child->hauto == false)
                {
                    absHeightSum += child->h;
                    ++elemIgnoredCount;
                }
            }

            int elemAutoHeight = (lh - absHeightSum) / (lc - elemIgnoredCount);

            int yPosAccum = ly;
            for (int i = 0; i < lc; ++i)
            {
                ALH *child = layout->container.at(i);
                if (child->xauto == false || child->yauto == false) continue;

                child->x = lx;
                child->y = yPosAccum;
                child->w = child->wauto ? lw : child->w;
                child->h = child->hauto ? elemAutoHeight : child->h;

                yPosAccum += child->h;

                if (i == lc - 1 && yPosAccum < lh)
                    child->h += lh - yPosAccum;
            }
        }
        else
        {
            int absWidthSum = 0;
            int elemIgnoredCount = 0;

            for (ALH *child : layout->container)
            {
                if (child->xauto == false || child->yauto == false)
                {
                    ++elemIgnoredCount;
                }
                else if (child->wauto == false)
                {
                    absWidthSum += child->w;
                    ++elemIgnoredCount;
                }
            }

            int elemAutoWidth = (lw - absWidthSum) / (lc - elemIgnoredCount);

            int xPosAccum = lx;
            for (int i = 0; i < lc; ++i)
            {
                ALH *child = layout->container.at(i);
                if (child->xauto == false || child->yauto == false) continue;

                child->x = xPosAccum;
                child->y = ly;
                child->w = child->wauto ? elemAutoWidth : child->w;
                child->h = child->hauto ? lh : child->h;

                xPosAccum += child->w;

                if (i == lc - 1 && xPosAccum < lw)
                    child->w += lw - xPosAccum;
            }
        }

        for (ALH *child : layout->container)
        {
            UpdateALHContainer(child);
        }
    }

    void UpdateMainCanvasALH(ALH *layout)
    {
        layout->x = 0;
        layout->y = 0;
        layout->w = Gfx::GetCoreRenderer()->renderTargetGUI.width;
        layout->h = Gfx::GetCoreRenderer()->renderTargetGUI.height;
        UpdateALHContainer(layout);
    }

    ALH *NewALH(bool vertical)
    {
        return NewALH(-1, -1, -1, -1, vertical);
    }

    ALH *NewALH(int absX, int absY, int absW, int absH, bool vertical)
    {
        ALH *alh = new ALH();
        
        alh->x = absX;
        alh->xauto = alh->x < 0;
        alh->y = absY;
        alh->yauto = alh->y < 0;
        alh->w = absW;
        alh->wauto = alh->w < 0;
        alh->h = absH;
        alh->hauto = alh->h < 0;
        
        alh->vertical = vertical;
        
        return alh;
    }

    void DeleteALH(ALH *layout)
    {

    }






    void Init()
    {
        MemoryLinearInitialize(&drawRequestsFrameStorageBuffer, 1000000);

        hoveredUI = null_ui_id;
        activeUI = null_ui_id;

        // __default_font = FontCreateFromFile(data_path("Baskic8.otf"), 32, true);
        // __fonts[1] = FontCreateFromFile(data_path("Baskic8.otf"), 16, true);
        // __fonts[2] = FontCreateFromFile(data_path("EndlessBossBattle.ttf"), 16, true);
        __fonts[5] = FontCreateFromFile(data_path("PressStart2P.ttf"), 16, true);
        __fonts[0] = FontCreateFromFile(data_path("BitFontMaker2Tes.ttf"), 12, true);
        __fonts[1] = FontCreateFromFile(data_path("BitFontMaker2Tes.ttf"), 13, true);
        __fonts[2] = FontCreateFromFile(data_path("BitFontMaker2Tes.ttf"), 14, true);
        __fonts[3] = FontCreateFromFile(data_path("BitFontMaker2Tes.ttf"), 15, true);
        __fonts[4] = FontCreateFromFile(data_path("BitFontMaker2Tes.ttf"), 16, true);

        BitmapHandle bm_anikki;
        ReadImage(bm_anikki, data_path("Kevin6x9.png").c_str());//data_path("Curses6x9.png").c_str());
        //ReadImage(bm_anikki, data_path("Anikki_square_8x8.png").c_str());
        for (u32 y = 0; y < bm_anikki.height; ++y)
        {
            for (u32 x = 0; x < bm_anikki.width; ++x)
            {
                unsigned char *bitmapData = (unsigned char *)bm_anikki.memory;
                unsigned char *pixelData = bitmapData + (y * 3 * bm_anikki.width + x * 3);
                if (pixelData[0] == 255 && pixelData[1] != 255 && pixelData[2] == 255)
                {
                    pixelData[0] = 0;
                    pixelData[1] = 0;
                    pixelData[2] = 0;
                }
                else
                {
                    pixelData[0] = 255;
                    pixelData[1] = 0;
                    pixelData[2] = 0;
                }
            }
        }
        Gfx::TextureHandle tex_0 = Gfx::CreateGPUTextureFromBitmap((unsigned char *) bm_anikki.memory, bm_anikki.width, bm_anikki.height, GL_RED, GL_RGB);
        __fonts[6] = FontCreateFromBitmap(tex_0);
        __default_font = __fonts[6];

        style_textFont = __default_font;

        GUIDraw_InitResources();
    }

    void NewFrame()
    {
        if (!anyElementHoveredThisFrame)
        {
            hoveredUI = null_ui_id;
        }
        anyElementHoveredThisFrame = false;
        keyboardInputASCIIKeycodeThisFrame.ResetCount();
        keyboardInputASCIIKeycodeThisFrame.ResetToZero();
        freshIdCounter = 0;
        __reservedTextMemoryIndexer = 0;

        drawRequestsFrameStorageBuffer.arenaOffset = 0;
        GUIDraw_NewFrame();
    }

    void Draw()
    {
//         int fontAtlasDebugY = 100;
//         for (int i = 6; i < 7; ++i)
//         {
//             PrimitivePanel(UIRect(10, fontAtlasDebugY, __fonts[i].ptr->font_atlas.width, __fonts[i].ptr->font_atlas.height), __fonts[i].textureId);
//             fontAtlasDebugY += __fonts[i].ptr->font_atlas.height + 5;
//         }

        GUIDraw_DrawEverything();
    }

    void ProcessSDLEvent(const SDL_Event event)
    {
        switch (event.type)
        {
            case SDL_MOUSEMOTION: 
            {
                Gfx::CoreRenderer *renderer = Gfx::GetCoreRenderer();
                i32 winW, winH = 0;
                renderer->GetBackBufferSize(&winW, &winH);
                // TODO(Kevin): For game GUI where window aspect ratio does not match game aspect ratio, must
                //              map from window mouse pos to gui canvas mouse pos
                mousePosX = int(float(event.motion.x) * (float(renderer->renderTargetGUI.width) / float(winW)));
                mousePosY = int(float(event.motion.y) * (float(renderer->renderTargetGUI.height) / float(winH)));
            }break;
            case SDL_KEYDOWN:
            {
                SDL_KeyboardEvent keyevent = event.key;
                SDL_Keycode keycodeASCII = keyevent.keysym.sym;
                keycodeASCII = ModifyASCIIBasedOnModifiers(keycodeASCII, keyevent.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT));
                keyboardInputASCIIKeycodeThisFrame.PushBack(keycodeASCII);
            }break;
        }
    }

}
