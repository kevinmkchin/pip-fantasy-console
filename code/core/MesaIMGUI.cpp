#include "MesaIMGUI.h"

#include <queue>
#include <stack>

#include "MesaMath.h"
#include "../singleheaders/vertext.h"
#include "../singleheaders/stb_sprintf.h"

#include "MesaUtility.h"
#include "GfxRenderer.h"
#include "GfxShader.h"
#include "PrintLog.h"
#include "FileSystem.h"

namespace MesaGUI
{
    static std::unordered_map<std::string, vtxt_font> __vtxtLoadedFonts;
    Font FontCreateFromFile(const std::string& fontFilePath, u8 fontSize, bool useNearestFiltering)
    {
        Font fontToReturn;
        vtxt_font fontHandle;

        BinaryFileHandle fontfile;
        ReadFileBinary(fontfile, fontFilePath.c_str());
        ASSERT(fontfile.memory);
        vtxt_init_font(&fontHandle, (u8*)fontfile.memory, fontSize);
        FreeFileBinary(fontfile);

        glGenTextures(1, &fontToReturn.textureId);
        glBindTexture(GL_TEXTURE_2D, fontToReturn.textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (useNearestFiltering ? GL_NEAREST : GL_LINEAR));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (useNearestFiltering ? GL_NEAREST : GL_LINEAR));
        glTexImage2D(
                GL_TEXTURE_2D,              // texture target type
                0,                          // level-of-detail number n = n-th mipmap reduction image
                GL_RED,               // format of data to store (target): num of color components
                fontHandle.font_atlas.width,                // texture width
                fontHandle.font_atlas.height,               // texture height
                0,                          // must be 0 (legacy)
                GL_RED,               // format of data being loaded (source)
                GL_UNSIGNED_BYTE,           // data type of the texture data
                fontHandle.font_atlas.pixels);                    // data
        glGenerateMipmap(GL_TEXTURE_2D);    // generate mip maps automatically
        glBindTexture(GL_TEXTURE_2D, 0);
        free(fontHandle.font_atlas.pixels);

        __vtxtLoadedFonts.emplace(fontFilePath, fontHandle);

        fontToReturn.ptr = &__vtxtLoadedFonts[fontFilePath];
        return fontToReturn;
    }

    static Shader __main_ui_shader;
    static const char* __main_ui_shader_vs =
            "#version 330 core\n"
            "layout (location = 0) in vec2 pos;\n"
            "layout (location = 1) in vec2 uv;\n"
            "out vec2 texUV;\n"
            "uniform mat4 matrixOrtho;\n"
            "void main() {\n"
            "   gl_Position = matrixOrtho * vec4(pos, 0.0, 1.0);\n"
            "   texUV = uv;\n"
            "}\n";
    static const char* __main_ui_shader_fs =
            "#version 330 core\n"
            "in vec2 texUV;\n"
            "out vec4 colour;\n"
            "uniform sampler2D textureSampler0;\n"
            "uniform bool useColour = false;\n"
            "uniform vec4 uiColour;\n"
            "void main() {\n"
            "   if (useColour) {\n"
            "       colour = uiColour;\n"
            "   } else {\n"
            "       colour = texture(textureSampler0, texUV);\n"
            "   }\n"
            "}\n";

    static Shader __text_shader;
    static const char* __text_shader_vs =
            "#version 330 core\n"
            "layout (location = 0) in vec2 pos;\n"
            "layout (location = 1) in vec2 uv;\n"
            "out vec2 texUV;\n"
            "uniform mat4 matrixModel;\n"
            "uniform mat4 matrixOrtho;\n"
            "void main() {\n"
            "   gl_Position = matrixOrtho * matrixModel * vec4(pos, 0.0, 1.0);\n"
            "   texUV = uv;\n"
            "}\n";
    static const char* __text_shader_fs =
            "#version 330 core\n"
            "in vec2 texUV;\n"
            "out vec4 colour;\n"
            "uniform sampler2D textureSampler0;\n"
            "uniform vec4 uiColour;\n"
            "void main() {\n"
            "   float textAlpha = texture(textureSampler0, texUV).x;\n"
            "   colour = vec4(uiColour.xyz, uiColour.w * textAlpha);\n"
            "}\n";

    static Mesh __ui_mesh;
    static Mesh __text_mesh;

    static Font __default_font;

    static char __reservedTextMemory[65536];
    static const int __reservedTextLineSizeBytes = 128;
    static int __reservedTextMemoryIndexer = 0;

    static NiceArray<SDL_Keycode, 16> keyboardInputASCIIKeycodeThisFrame;

    static NiceArray<char, 128> activeTextInputBuffer;

    static std::stack<UIStyle> ui_ss; // UI Style Stack
    static UIWindowGuide activeWindowGuide;

    static ui_id freshIdCounter = 0;
    static ui_id hoveredUI = null_ui_id;
    static ui_id activeUI = null_ui_id;
    static bool anyElementHoveredThisFrame = false;

    struct UIDrawRequest
    {
        vec4 color;

        virtual void Draw() = 0;
    };

    struct RectDrawRequest : UIDrawRequest
    {
        UIRect rect;

        void Draw()
        {
            float left = (float)rect.x;
            float top = (float)rect.y;
            float bottom = (float)rect.y + rect.h;
            float right = (float)rect.x + rect.w;
            float vb[] = { left, top, 0.f, 1.f,
                           left, bottom, 0.f, 0.f,
                           right, bottom, 1.f, 0.f,
                           right, top, 1.f, 1.f, };
            u32 ib[] = { 0, 1, 3, 1, 2, 3 };

            __main_ui_shader.UseShader();
            __main_ui_shader.GLBind1i("useColour", true);
            __main_ui_shader.GLBind4f("uiColour", color.x, color.y, color.z, color.w);

            RebindBufferObjects(__ui_mesh, vb, ib, ARRAY_COUNT(vb), ARRAY_COUNT(ib), GL_DYNAMIC_DRAW);
            RenderMesh(__ui_mesh);
        }
    };

    struct CorneredRectDrawRequest : UIDrawRequest
    {
        UIRect rect;
        int cornerWidth = 10;

        GLuint textureId = 0;
        float normalizedCornerSizeInUV = 0.3f; // [0,1] with 0.5 being half way across texture

        void Draw()
        {
            float left = (float)rect.x;
            float top = (float)rect.y;
            float bottom = (float)rect.y + rect.h;
            float right = (float)rect.x + rect.w;
            float corner = (float)cornerWidth;
            float uv0 = normalizedCornerSizeInUV;
            float uv1 = 1.f - uv0;

            float vb[] = { left, top,              0.f, 1.f,
                           left, top + corner,     0.f, uv1,
                           left + corner, top,     uv0, 1.f,
                           left + corner, top + corner, uv0, uv1,

                           right - corner, top,    uv1, 1.f,
                           right - corner, top + corner, uv1, uv1,
                           right, top,             1.f, 1.f,
                           right, top + corner,    1.f, uv1,

                           left, bottom - corner,  0.f, uv0,
                           left, bottom,           0.f, 0.f,
                           left + corner, bottom - corner, uv0, uv0,
                           left + corner, bottom,  uv0, 0.f,

                           right - corner, bottom - corner, uv1, uv0,
                           right - corner, bottom, uv1, 0.f,
                           right, bottom - corner, 1.f, uv0,
                           right, bottom,          1.f, 0.f,
            };
            u32 ib[] = { 0, 1, 2, 2, 1, 3, 2, 3, 4, 4, 3, 5, 4, 5, 6, 6, 5, 7, 1, 8, 3, 3, 8, 10, 3, 10, 5,
                         5, 10, 12, 5, 12, 7, 7, 12, 14, 8, 9, 10, 10, 9, 11, 10, 11, 12, 12, 11, 13, 12, 13, 14, 14, 13, 15 };

            __main_ui_shader.UseShader();

            if (textureId != 0)
            {
                __main_ui_shader.GLBind1i("useColour", false);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureId);
                __main_ui_shader.GLBind1i("textureSampler0", 0);
            }
            else
            {
                __main_ui_shader.GLBind1i("useColour", true);
                __main_ui_shader.GLBind4f("uiColour", color.x, color.y, color.z, color.w);
            }

            RebindBufferObjects(__ui_mesh, vb, ib, ARRAY_COUNT(vb), ARRAY_COUNT(ib), GL_DYNAMIC_DRAW);
            RenderMesh(__ui_mesh);
        }
    };

    struct TextDrawRequest : UIDrawRequest
    {
        const char* text = "";
        int size = 8;
        int x = 0;
        int y = 0;
        TextAlignment alignment = TextAlignment::Left;
        Font font;

        void Draw()
        {
            vtxt_setflags(VTXT_CREATE_INDEX_BUFFER);
            vtxt_clear_buffer();
            vtxt_move_cursor(x, y);
            switch (alignment)
            {
                case TextAlignment::Left:{
                    vtxt_append_line(text, font.ptr, size);
                }break;
                case TextAlignment::Center:{
                    vtxt_append_line_centered(text, font.ptr, size);
                }break;
                case TextAlignment::Right:{
                    vtxt_append_line_align_right(text, font.ptr, size);
                }break;
            }
            vtxt_vertex_buffer _txt = vtxt_grab_buffer();
            RebindBufferObjects(__text_mesh, _txt.vertex_buffer, _txt.index_buffer, _txt.vertices_array_count, _txt.indices_array_count, GL_DYNAMIC_DRAW);
            vtxt_clear_buffer();

            mat4 matrixModel = mat4();
            __text_shader.UseShader();
            __text_shader.GLBindMatrix4fv("matrixModel", 1, matrixModel.ptr());
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, font.textureId);
            __text_shader.GLBind1i("textureSampler0", 0);
            __text_shader.GLBind4f("uiColour", color.x, color.y, color.z, color.w);

            RenderMesh(__text_mesh);
        }
    };

    static NiceArray<RectDrawRequest, 256> rectDrawRequestsBuffer;
    static NiceArray<CorneredRectDrawRequest, 256> corneredRectDrawRequestsBuffer;
    static NiceArray<TextDrawRequest, 256> textDrawRequestsBuffer;
    static std::queue<UIDrawRequest*> drawQueue;

    static bool IsActive(ui_id id)
    {
        return activeUI == id;
    }
    static bool IsHovered(ui_id id)
    {
        return hoveredUI == id;
    }
    static void SetActive(ui_id id)
    {
        activeUI = id;
    }
    static void SetHovered(ui_id id)
    {
        anyElementHoveredThisFrame = true;
        hoveredUI = id;
    }


    static int mousePosX = 0;
    static int mousePosY = 0;
    static bool mouseDown = false;
    static bool mouseUp = false;
    static bool MouseWentUp()
    {
        return mouseUp;
    }
    static bool MouseWentDown()
    {
        return mouseDown;
    }
    static bool MouseInside(const UIRect& rect)
    {
        int left = rect.x;
        int top = rect.y;
        int right = left + rect.w;
        int bottom = top + rect.h;
        if (left <= mousePosX && mousePosX <= right
            && top <= mousePosY && mousePosY <= bottom)
        {
            return true;
        }
        else
        {
            return false;
        }
    }


    bool DoButton(ui_id id, UIRect rect, vec4 normalColor, vec4 hoveredColor, vec4 activeColor)
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

        RectDrawRequest drawRequest;
        drawRequest.rect = rect;
        drawRequest.color = IsHovered(id) ? hoveredColor : normalColor;
        if (IsActive(id)) drawRequest.color = activeColor;

        rectDrawRequestsBuffer.PushBack(drawRequest);
        drawQueue.push(&rectDrawRequestsBuffer.Back());

        return result;
    }

    void DoPanel(UIRect rect, vec4 colorRGBA)
    {
        RectDrawRequest drawRequest;
        drawRequest.rect = rect;
        drawRequest.color = colorRGBA;

        rectDrawRequestsBuffer.PushBack(drawRequest);
        drawQueue.push(&rectDrawRequestsBuffer.Back());
    }

    void DoPanel(UIRect rect, int cornerWidth, u32 glTextureId, float normalizedCornerSizeInUV)
    {
        CorneredRectDrawRequest drawRequest;
        drawRequest.rect = rect;
        drawRequest.color = vec4(0.157f, 0.172f, 0.204f, 1.f);
        drawRequest.textureId = glTextureId;
        drawRequest.cornerWidth = cornerWidth;
        drawRequest.normalizedCornerSizeInUV = normalizedCornerSizeInUV;

        corneredRectDrawRequestsBuffer.PushBack(drawRequest);
        drawQueue.push(&corneredRectDrawRequestsBuffer.Back());
    }

    void DoText(int x, int y, int size, TextAlignment alignment, const char* textFmt, ...)
    {
#if INTERNAL_BUILD & SLOW_BUILD
        if (__reservedTextMemoryIndexer >= ARRAY_COUNT(__reservedTextMemory) / __reservedTextLineSizeBytes)
        {
            PrintLog.WarningFmt("Attempting to draw text in GUI.cpp but not enough reserved memory to store text.");
            return;
        }
#endif

        va_list argptr;
        char* formattedTextBuffer = __reservedTextMemory + (__reservedTextMemoryIndexer * __reservedTextLineSizeBytes);
        va_start(argptr, textFmt);
        stbsp_vsprintf(formattedTextBuffer, textFmt, argptr);
        va_end(argptr);
        ++__reservedTextMemoryIndexer;

        TextDrawRequest drawRequest;
        drawRequest.text = formattedTextBuffer;
        drawRequest.size = size;
        drawRequest.x = x;
        drawRequest.y = y;
        drawRequest.alignment = alignment;
        drawRequest.font = ui_ss.top().textFont;
        drawRequest.color = ui_ss.top().textColor;

        textDrawRequestsBuffer.PushBack(drawRequest);
        drawQueue.push(&textDrawRequestsBuffer.Back());
    }

    void DoTextUnformatted(int x, int y, int size, TextAlignment alignment, const char* text)
    {
#if INTERNAL_BUILD & SLOW_BUILD
        if (__reservedTextMemoryIndexer >= ARRAY_COUNT(__reservedTextMemory) / __reservedTextLineSizeBytes)
        {
            PrintLog.WarningFmt("Attempting to draw text in GUI.cpp but not enough reserved memory to store text.");
            return;
        }
#endif

        char* textBuffer = __reservedTextMemory + (__reservedTextMemoryIndexer * __reservedTextLineSizeBytes);
        memcpy(textBuffer, text, __reservedTextLineSizeBytes);
        ++__reservedTextMemoryIndexer;

        TextDrawRequest drawRequest;
        drawRequest.text = textBuffer;
        drawRequest.size = size;
        drawRequest.x = x;
        drawRequest.y = y;
        drawRequest.alignment = alignment;
        drawRequest.font = ui_ss.top().textFont;
        drawRequest.color = ui_ss.top().textColor;

        textDrawRequestsBuffer.PushBack(drawRequest);
        drawQueue.push(&textDrawRequestsBuffer.Back());
    }

    void DoImage(UIRect rect, u32 glTextureId)
    {
        CorneredRectDrawRequest drawRequest;
        drawRequest.rect = rect;
        drawRequest.color = vec4(0.157f, 0.172f, 0.204f, 1.f);
        drawRequest.textureId = glTextureId;
        drawRequest.cornerWidth = 0;
        drawRequest.normalizedCornerSizeInUV = 0.f;

        corneredRectDrawRequestsBuffer.PushBack(drawRequest);
        drawQueue.push(&corneredRectDrawRequestsBuffer.Back());
    }

    void DoIntegerInputField(ui_id id, UIRect rect, int* v)
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

        RectDrawRequest drawRequest;
        drawRequest.rect = rect;
        drawRequest.color = IsActive(id) ? vec4(0.f, 0.f, 0.f, 1.f) : vec4(0.2f, 0.2f, 0.2f, 1.f);//vec4(1.f, 1.f, 1.f, 1.f) : vec4(0.8f, 0.8f, 0.8f, 1.f);

        rectDrawRequestsBuffer.PushBack(drawRequest);
        drawQueue.push(&rectDrawRequestsBuffer.Back());

        if (IsActive(id))
        {
            if (activeTextInputBuffer.count > 0)
            {
                DoText(rect.x + rect.w, rect.y + rect.h, 14, TextAlignment::Right, activeTextInputBuffer.data);
            }
        }
        else
        {
            DoText(rect.x + rect.w, rect.y + rect.h, 14, TextAlignment::Right, std::to_string(*v).c_str());
        }
    }

    void DoFloatInputField(ui_id id, UIRect rect, float* v)
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

        RectDrawRequest drawRequest;
        drawRequest.rect = rect;
        drawRequest.color = IsActive(id) ? vec4(0.f, 0.f, 0.f, 1.f) : vec4(0.2f, 0.2f, 0.2f, 1.f);

        rectDrawRequestsBuffer.PushBack(drawRequest);
        drawQueue.push(&rectDrawRequestsBuffer.Back());

        if (IsActive(id))
        {
            if (activeTextInputBuffer.count > 0)
            {
                DoText(rect.x + rect.w, rect.y + rect.h, 14, TextAlignment::Right, activeTextInputBuffer.data);
            }
        }
        else
        {
            char cbuf[32];
            stbsp_sprintf(cbuf, "%.2f", *v);
            DoText(rect.x + rect.w, rect.y + rect.h, 14, TextAlignment::Right, cbuf);
        }
    }


    void PushUIStyle(UIStyle style)
    {
        ui_ss.push(style);
    }

    void PopUIStyle()
    {
        if (!ui_ss.empty()) ui_ss.pop();
    }

    UIStyle GetActiveUIStyleCopy()
    {
        return ui_ss.top();
    }

    UIStyle& GetActiveUIStyleReference()
    {
        return ui_ss.top();
    }

    bool LabelledButton(UIRect rect, const char* label, int textSize, TextAlignment textAlignment)
    {
        float ascenderTextSize = (float)textSize;
        float yTextPaddingRatio = (1.f - (ascenderTextSize / (float) rect.h)) / 2.f;
        ivec2 textPadding = ivec2(10, (int) roundf(rect.h * yTextPaddingRatio));
        int textX = rect.x + textPadding.x;
        if (textAlignment == TextAlignment::Center)
        {
            textX = (int) ((rect.w / 2) + rect.x);
        }
        else if (textAlignment == TextAlignment::Right)
        {
            textX = (int) rect.x + rect.w - textPadding.x;
        }

        bool buttonValue = DoButton(FreshID(), rect, ui_ss.top().buttonNormalColor, ui_ss.top().buttonHoveredColor, ui_ss.top().buttonActiveColor);
        DoTextUnformatted(textX, rect.y + rect.h - textPadding.y, textSize, textAlignment, label);
        return buttonValue;
    }



    void EditorBeginWindow(UIRect windowRect)
    {
        if (activeWindowGuide.windowId != null_ui_id)
        {
            PrintLog.Error("EditorBeginWindow and EditorEndWindows don't match.");
            return;
        }

        ui_id windowId = FreshID();
        activeWindowGuide.windowId = windowId;
        activeWindowGuide.initialWindowRect = windowRect;
        activeWindowGuide.xOffsetFromInitialWindow = 5;
        activeWindowGuide.yOffsetFromInitialWindow = 5;
        // TODO(Kevin): Set active ui style to editor default style?
        DoPanel(windowRect, ui_ss.top().editorWindowBackgroundColor);
    }

    void EditorEndWindow()
    {
        activeWindowGuide.windowId = null_ui_id;
    }

    bool EditorLabelledButton(const char* label)
    {
        int labelTextSize = ui_ss.top().editorTextSize;
        float textW;
        float textH;
        vtxt_get_text_bounding_box_info(&textW, &textH, label, ui_ss.top().textFont.ptr, labelTextSize);

        int buttonX = activeWindowGuide.initialWindowRect.x + activeWindowGuide.xOffsetFromInitialWindow;
        int buttonY = activeWindowGuide.initialWindowRect.y + activeWindowGuide.yOffsetFromInitialWindow;
        int buttonW = GM_max((int) textW + 4, 50);
        int buttonH = labelTextSize + 4;
        int buttonPaddingAbove = 0;
        int buttonPaddingBelow = 5;
        UIRect buttonRect = UIRect(buttonX, buttonY + buttonPaddingAbove, buttonW, buttonH);
        bool result = LabelledButton(buttonRect, label, labelTextSize, TextAlignment::Center);
        activeWindowGuide.yOffsetFromInitialWindow += buttonPaddingAbove + buttonH + buttonPaddingBelow;
        return result;
    }

    void EditorIncrementableIntegerField(const char* label, int* v, int increment)
    {
        int x = activeWindowGuide.initialWindowRect.x + activeWindowGuide.xOffsetFromInitialWindow;
        int y = activeWindowGuide.initialWindowRect.y + activeWindowGuide.yOffsetFromInitialWindow;
        int w = 120;
        int h = 20;
        int paddingAbove = 0;
        int paddingBelow = 5;

        DoPanel(UIRect(x, y, w-2, h), vec4(0.4f, 0.4f, 0.4f, 1.f));
        DoIntegerInputField(FreshID(), UIRect(x + 1, y + 1, w-4, h - 2), v);
        if (DoButton(FreshID(), UIRect(x + w, y + 1, 20, (h / 2) - 1), ui_ss.top().buttonNormalColor, ui_ss.top().buttonHoveredColor, ui_ss.top().buttonActiveColor))
        {
            (*v) += increment;
        }
        if (DoButton(FreshID(), UIRect(x + w, y + (h / 2) + 1, 20, (h / 2) - 1), ui_ss.top().buttonNormalColor, ui_ss.top().buttonHoveredColor, ui_ss.top().buttonActiveColor))
        {
            (*v) -= increment;
        }
        DoTextUnformatted(x + w + 22, y + h, 20, TextAlignment::Left, label);

        activeWindowGuide.yOffsetFromInitialWindow += paddingAbove + h + paddingBelow;
    }

    void EditorIncrementableFloatField(const char* label, float* v, float increment)
    {
        int x = activeWindowGuide.initialWindowRect.x + activeWindowGuide.xOffsetFromInitialWindow;
        int y = activeWindowGuide.initialWindowRect.y + activeWindowGuide.yOffsetFromInitialWindow;
        int w = 120;
        int h = 20;
        int paddingAbove = 0;
        int paddingBelow = 5;

        DoPanel(UIRect(x, y, w-2, h), vec4(0.4f, 0.4f, 0.4f, 1.f));
        DoFloatInputField(FreshID(), UIRect(x + 1, y + 1, w-4, h - 2), v);
        if (DoButton(FreshID(), UIRect(x + w, y + 1, 20, (h / 2) - 1), ui_ss.top().buttonNormalColor, ui_ss.top().buttonHoveredColor, ui_ss.top().buttonActiveColor))
        {
            (*v) += increment;
        }
        if (DoButton(FreshID(), UIRect(x + w, y + (h / 2) + 1, 20, (h / 2) - 1), ui_ss.top().buttonNormalColor, ui_ss.top().buttonHoveredColor, ui_ss.top().buttonActiveColor))
        {
            (*v) -= increment;
        }
        DoTextUnformatted(x + w + 22, y + h, 20, TextAlignment::Left, label);

        activeWindowGuide.yOffsetFromInitialWindow += paddingAbove + h + paddingBelow;
    }



    ui_id FreshID()
    {
        return freshIdCounter++;
    }

    void Init()
    {
        hoveredUI = null_ui_id;
        activeUI = null_ui_id;
        Shader::GLCreateShaderProgram(__main_ui_shader, __main_ui_shader_vs, __main_ui_shader_fs);
        Shader::GLCreateShaderProgram(__text_shader, __text_shader_vs, __text_shader_fs);
        MeshCreate(__ui_mesh, nullptr, nullptr, 0, 0, 2, 2, 0, GL_DYNAMIC_DRAW);
        MeshCreate(__text_mesh, nullptr, nullptr, 0, 0, 2, 2, 0, GL_DYNAMIC_DRAW);

        __default_font = FontCreateFromFile(data_path("PressStart2P.ttf"), 16, true);

        if(ui_ss.empty())
        {
            UIStyle defaultStyle;
            defaultStyle.textFont = __default_font;
            ui_ss.push(defaultStyle);
        }
    }

    void NewFrame()
    {
        if (!anyElementHoveredThisFrame)
        {
            hoveredUI = null_ui_id;
        }
        anyElementHoveredThisFrame = false;

        rectDrawRequestsBuffer.ResetCount();
        corneredRectDrawRequestsBuffer.ResetCount();
        textDrawRequestsBuffer.ResetCount();

        keyboardInputASCIIKeycodeThisFrame.ResetCount();
        keyboardInputASCIIKeycodeThisFrame.ResetToZero();

        freshIdCounter = 0;

        __reservedTextMemoryIndexer = 0;
    }

    void SDLProcessEvent(const SDL_Event* evt)
    {
        SDL_Event event = *evt;
        switch (event.type)
        {
            case SDL_MOUSEMOTION: {
                mousePosX = event.motion.x;
                mousePosY = event.motion.y;
            }break;
            case SDL_MOUSEBUTTONDOWN:
            {
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    mouseDown = true;
                    mouseUp = false;
                }
            }break;
            case SDL_MOUSEBUTTONUP:
            {
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    mouseUp = true;
                    mouseDown = false;
                }
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

    void Draw()
    {

        i32 kevGuiScreenWidth = GetGfxRenderer()->guiLayer.width;
        i32 kevGuiScreenHeight = GetGfxRenderer()->guiLayer.height;
        mat4 matrixOrtho = ProjectionMatrixOrthographic2D(0.f, (float)kevGuiScreenWidth, (float)kevGuiScreenHeight, 0.f);

        __main_ui_shader.UseShader();
        __main_ui_shader.GLBindMatrix4fv("matrixOrtho", 1, matrixOrtho.ptr());

        __text_shader.UseShader();
        __text_shader.GLBindMatrix4fv("matrixOrtho", 1, matrixOrtho.ptr());

        while(!drawQueue.empty())
        {
            UIDrawRequest* drawCall = drawQueue.front();
            drawCall->Draw();
            drawQueue.pop();
        }
    }

}
