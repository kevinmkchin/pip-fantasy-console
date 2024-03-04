#include "MesaIMGUI.h"

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
#include "GfxDataTypesAndUtility.h"
#include "GfxShader.h"
#include "PrintLog.h"
#include "FileSystem.h"
#include "Input.h"

static ui_id freshIdCounter = 0;
static ui_id FreshID()
{
    return freshIdCounter++;
}

namespace MesaGUI
{
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

    static Gfx::Shader __main_ui_shader;
    static const char* __main_ui_shader_vs =
            "#version 330 core\n"
            "uniform mat4 matrixOrtho;\n"
            "layout (location = 0) in vec2 pos;\n"
            "layout (location = 1) in vec2 uv;\n"
            "out vec2 texUV;\n"
            "void main() {\n"
            "    gl_Position = matrixOrtho * vec4(pos, 0.0, 1.0);\n"
            "    texUV = uv;\n"
            "}\n";
    static const char* __main_ui_shader_fs =
            "#version 330 core\n"
            "uniform sampler2D textureSampler0;\n"
            "uniform bool useColour = false;\n"
            "uniform vec4 uiColour;\n"
            "in vec2 texUV;\n"
            "out vec4 colour;\n"
            "void main() {\n"
            "    if (useColour) {\n"
            "        colour = uiColour;\n"
            "    } else {\n"
            "        colour = texture(textureSampler0, texUV);\n"
            "    }\n"
            "}\n";

    static Gfx::Shader __rounded_corner_rect_shader;
    static const char* __rounded_corner_rect_shader_vs =
            "#version 330 core\n"
            "uniform mat4 matrixOrtho;\n"
            "layout (location = 0) in vec2 pos;\n"
            "layout (location = 1) in vec2 uv;\n"
            "out vec2 fragPos;\n"
            "void main() {\n"
            "    fragPos = pos;\n"
            "    gl_Position = matrixOrtho * vec4(pos, 0.0, 1.0);\n"
            "}\n";
    static const char* __rounded_corner_rect_shader_fs =
            "#version 330 core\n"
            "uniform ivec4 rect;\n"
            "uniform int cornerRadius;\n"
            "uniform vec4 uiColour;\n"
            "in vec2 fragPos;\n"
            "out vec4 colour;\n"
            "void main() {\n"
            "    vec4 frect = vec4(rect);\n"
            "    float fradius = float(cornerRadius);\n"
            "\n"
            "    bool xokay = (frect.x + fradius) < fragPos.x && fragPos.x < (frect.x + frect.z - fradius);\n"
            "    bool yokay = (frect.y + fradius) < fragPos.y && fragPos.y < (frect.y + frect.w - fradius);\n"
            "\n"
            "    if (xokay || yokay) { \n"
            "        colour = uiColour;\n"
            "    } else {\n"
            "        vec2 cornerPoint;\n"
            "        if (fragPos.x < frect.x + fradius && fragPos.y < frect.y + fradius) { // top left\n"
            "            cornerPoint = vec2(frect.x + fradius, frect.y + fradius);\n"
            "        } else if (fragPos.x < frect.x + fradius && fragPos.y > frect.y + frect.w - fradius) { // bottom left\n"
            "            cornerPoint = vec2(frect.x + fradius, frect.y + frect.w - fradius);\n"
            "        } else if (fragPos.x > frect.x + frect.z - fradius && fragPos.y < frect.y + fradius) { // top right\n"
            "            cornerPoint = vec2(frect.x + frect.z - fradius, frect.y + fradius);\n"
            "        } else if (fragPos.x > frect.x + frect.z - fradius && fragPos.y > frect.y + frect.w - fradius) { // bottom right\n"
            "            cornerPoint = vec2(frect.x + frect.z - fradius, frect.y + frect.w - fradius);\n"
            "        }\n"
            "        if (distance(cornerPoint, fragPos) < fradius) {\n"
            "            colour = uiColour;\n"
            "        } else {\n"
            "            colour = vec4(0.0, 0.0, 0.0, 0.0);\n"
            "        }\n"
            "    }\n"
            "}\n";

    static Gfx::Shader __text_shader;
    static const char* __text_shader_vs =
            "#version 330 core\n"
            "uniform mat4 matrixModel;\n"
            "uniform mat4 matrixOrtho;\n"
            "layout (location = 0) in vec2 pos;\n"
            "layout (location = 1) in vec2 uv;\n"
            "out vec2 fragPos;\n"
            "out vec2 texUV;\n"
            "void main() {\n"
            "    fragPos = pos;\n"
            "    gl_Position = matrixOrtho * matrixModel * vec4(pos, 0.0, 1.0);\n"
            "    texUV = uv;\n"
            "}\n";
    static const char* __text_shader_fs =
            "#version 330 core\n"
            "uniform sampler2D textureSampler0;\n"
            "uniform vec4 uiColour;\n"
            "uniform ivec4 rectMask;\n"
            "uniform int rectMaskCornerRadius;\n"
            "in vec2 fragPos;\n"
            "in vec2 texUV;\n"
            "out vec4 colour;\n"
            "void main() {\n"
            "    float textAlpha = texture(textureSampler0, texUV).x;\n"
            "    \n"
            "    if (rectMaskCornerRadius < 0)\n"
            "    {\n"
            "        colour = vec4(uiColour.xyz, uiColour.w * textAlpha);\n"
            "    }\n"
            "    else\n"
            "    {\n"
            "        vec4 frect = vec4(rectMask);\n"
            "        float fradius = float(rectMaskCornerRadius);\n"
            "\n"
            "        bool xbad = fragPos.x < frect.x || (frect.x + frect.z) < fragPos.x;\n"
            "        bool ybad = fragPos.y < frect.y || (frect.y + frect.w) < fragPos.y;\n"
            "\n"
            "        if (xbad || ybad) {\n"
            "            colour = vec4(0.0, 0.0, 0.0, 0.0);\n"
            "        } else {\n"
            "            bool xokay = (frect.x + fradius) < fragPos.x && fragPos.x < (frect.x + frect.z - fradius);\n"
            "            bool yokay = (frect.y + fradius) < fragPos.y && fragPos.y < (frect.y + frect.w - fradius);\n"
            "\n"
            "            if (xokay || yokay) { \n"
            "                colour = vec4(uiColour.xyz, uiColour.w * textAlpha);\n"
            "            } else {\n"
            "                vec2 cornerPoint;\n"
            "                if (fragPos.x < frect.x + fradius && fragPos.y < frect.y + fradius) { // top left\n"
            "                    cornerPoint = vec2(frect.x + fradius, frect.y + fradius);\n"
            "                } else if (fragPos.x < frect.x + fradius && fragPos.y > frect.y + frect.w - fradius) { // bottom left\n"
            "                    cornerPoint = vec2(frect.x + fradius, frect.y + frect.w - fradius);\n"
            "                } else if (fragPos.x > frect.x + frect.z - fradius && fragPos.y < frect.y + fradius) { // top right\n"
            "                    cornerPoint = vec2(frect.x + frect.z - fradius, frect.y + fradius);\n"
            "                } else if (fragPos.x > frect.x + frect.z - fradius && fragPos.y > frect.y + frect.w - fradius) { // bottom right\n"
            "                    cornerPoint = vec2(frect.x + frect.z - fradius, frect.y + frect.w - fradius);\n"
            "                }\n"
            "                if (distance(cornerPoint, fragPos) < fradius) {\n"
            "                    colour = vec4(uiColour.xyz, uiColour.w * textAlpha);\n"
            "                } else {\n"
            "                    colour = vec4(0.0, 0.0, 0.0, 0.0);\n"
            "                }\n"
            "            }\n"
            "        }\n"
            "    }\n"
            "}\n"
            "";

    static Gfx::Shader __colored_text_shader;
    static const char* __colored_text_shader_vs =
            "#version 330 core\n"
            "uniform mat4 matrixModel;\n"
            "uniform mat4 matrixOrtho;\n"
            "layout (location = 0) in vec2 pos;\n"
            "layout (location = 1) in vec2 uv;\n"
            "layout (location = 2) in vec3 vcol;\n"
            "out vec2 fragPos;\n"
            "out vec2 texUV;\n"
            "out vec3 vertColor;\n"
            "void main() {\n"
            "    fragPos = pos;\n"
            "    gl_Position = matrixOrtho * matrixModel * vec4(pos, 0.0, 1.0);\n"
            "    texUV = uv;\n"
            "    vertColor = vcol;\n"
            "}\n";
    static const char* __colored_text_shader_fs =
            "#version 330 core\n"
            "uniform sampler2D textureSampler0;\n"
            "uniform ivec4 rectMask;\n"
            "uniform int rectMaskCornerRadius;\n"
            "in vec2 fragPos;\n"
            "in vec2 texUV;\n"
            "in vec3 vertColor;\n"
            "out vec4 colour;\n"
            "void main() {\n"
            "    float textAlpha = texture(textureSampler0, texUV).x;\n"
            "    \n"
            "    if (rectMaskCornerRadius < 0)\n"
            "    {\n"
            "        colour = vec4(vertColor, textAlpha);\n"
            "    }\n"
            "    else\n"
            "    {\n"
            "        vec4 frect = vec4(rectMask);\n"
            "        float fradius = float(rectMaskCornerRadius);\n"
            "\n"
            "        bool xbad = fragPos.x < frect.x || (frect.x + frect.z) < fragPos.x;\n"
            "        bool ybad = fragPos.y < frect.y || (frect.y + frect.w) < fragPos.y;\n"
            "\n"
            "        if (xbad || ybad) {\n"
            "            colour = vec4(0.0, 0.0, 0.0, 0.0);\n"
            "        } else {\n"
            "            bool xokay = (frect.x + fradius) < fragPos.x && fragPos.x < (frect.x + frect.z - fradius);\n"
            "            bool yokay = (frect.y + fradius) < fragPos.y && fragPos.y < (frect.y + frect.w - fradius);\n"
            "\n"
            "            if (xokay || yokay) { \n"
            "                colour = vec4(vertColor, textAlpha);\n"
            "            } else {\n"
            "                vec2 cornerPoint;\n"
            "                if (fragPos.x < frect.x + fradius && fragPos.y < frect.y + fradius) { // top left\n"
            "                    cornerPoint = vec2(frect.x + fradius, frect.y + fradius);\n"
            "                } else if (fragPos.x < frect.x + fradius && fragPos.y > frect.y + frect.w - fradius) { // bottom left\n"
            "                    cornerPoint = vec2(frect.x + fradius, frect.y + frect.w - fradius);\n"
            "                } else if (fragPos.x > frect.x + frect.z - fradius && fragPos.y < frect.y + fradius) { // top right\n"
            "                    cornerPoint = vec2(frect.x + frect.z - fradius, frect.y + fradius);\n"
            "                } else if (fragPos.x > frect.x + frect.z - fradius && fragPos.y > frect.y + frect.w - fradius) { // bottom right\n"
            "                    cornerPoint = vec2(frect.x + frect.z - fradius, frect.y + frect.w - fradius);\n"
            "                }\n"
            "                if (distance(cornerPoint, fragPos) < fradius) {\n"
            "                    colour = vec4(vertColor, textAlpha);\n"
            "                } else {\n"
            "                    colour = vec4(0.0, 0.0, 0.0, 0.0);\n"
            "                }\n"
            "            }\n"
            "        }\n"
            "    }\n"
            "}\n"
            "";

    static Gfx::Mesh __ui_mesh;
    static Gfx::Mesh __text_mesh;
    static Gfx::Mesh __colored_text_mesh;

    static char __reservedTextMemory[16000000];
    static u32 __reservedTextMemoryIndexer = 0;

    NiceArray<SDL_Keycode, 32> keyboardInputASCIIKeycodeThisFrame;

    static NiceArray<char, 128> activeTextInputBuffer;

    static std::stack<UIStyle> ui_ss; // UI Style Stack
    static UIZone activeZone;

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
        GLuint textureId = 0;

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

            Gfx::UseShader(__main_ui_shader);

            if (textureId != 0)
            {
                Gfx::GLBind1i(__main_ui_shader, "useColour", false);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureId);
                Gfx::GLBind1i(__main_ui_shader, "textureSampler0", 0);
            }
            else
            {
                Gfx::GLBind1i(__main_ui_shader, "useColour", true);
                Gfx::GLBind4f(__main_ui_shader, "uiColour", color.x, color.y, color.z, color.w);
            }

            RebindBufferObjects(__ui_mesh, vb, ib, ARRAY_COUNT(vb), ARRAY_COUNT(ib), GL_DYNAMIC_DRAW);
            RenderMesh(__ui_mesh);
        }
    };

    struct RoundedCornerRectDrawRequest : UIDrawRequest
    {
        UIRect rect;
        int radius = 10;

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

            Gfx::UseShader(__rounded_corner_rect_shader);
            Gfx::GLBind4i(__rounded_corner_rect_shader, "rect", rect.x, rect.y, rect.w, rect.h);
            Gfx::GLBind1i(__rounded_corner_rect_shader, "cornerRadius", radius);
            Gfx::GLBind4f(__rounded_corner_rect_shader, "uiColour", color.x, color.y, color.z, color.w);

            RebindBufferObjects(__ui_mesh, vb, ib, ARRAY_COUNT(vb), ARRAY_COUNT(ib), GL_DYNAMIC_DRAW);
            RenderMesh(__ui_mesh);
        }
    };

    struct CorneredRectDrawRequest : UIDrawRequest
    {
        UIRect rect;
        int radius = 10;

        GLuint textureId = 0;
        float normalizedCornerSizeInUV = 0.3f; // [0,1] with 0.5 being half way across texture

        void Draw()
        {
            float left = (float)rect.x;
            float top = (float)rect.y;
            float bottom = (float)rect.y + rect.h;
            float right = (float)rect.x + rect.w;
            float corner = (float)radius;
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

            Gfx::UseShader(__main_ui_shader);

            if (textureId != 0)
            {
                Gfx::GLBind1i(__main_ui_shader, "useColour", false);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureId);
                Gfx::GLBind1i(__main_ui_shader, "textureSampler0", 0);
            }
            else
            {
                Gfx::GLBind1i(__main_ui_shader, "useColour", true);
                Gfx::GLBind4f(__main_ui_shader, "uiColour", color.x, color.y, color.z, color.w);
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

        UIRect rectMask = UIRect(0, 0, 9999, 9999);
        int rectMaskCornerRadius = -1;

        void Draw() final
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
            if (_txt.vertices_array_count > 0)
                RebindBufferObjects(__text_mesh, _txt.vertex_buffer, _txt.index_buffer, _txt.vertices_array_count, _txt.indices_array_count, GL_DYNAMIC_DRAW);
            vtxt_clear_buffer();

            mat4 matrixModel = mat4();
            Gfx::UseShader(__text_shader);
            Gfx::GLBindMatrix4fv(__text_shader, "matrixModel", 1, matrixModel.ptr());
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, font.textureId);
            Gfx::GLBind1i(__text_shader, "textureSampler0", 0);
            Gfx::GLBind4f(__text_shader, "uiColour", color.x, color.y, color.z, color.w);

            Gfx::GLBind4i(__text_shader, "rectMask", rectMask.x, rectMask.y, rectMask.w, rectMask.h);
            Gfx::GLBind1i(__text_shader, "rectMaskCornerRadius", rectMaskCornerRadius);

            RenderMesh(__text_mesh);
        }
    };

    vec3 CodeCharIndexToColor[32000];
    struct PipCodeDrawRequest : UIDrawRequest
    {
        const char* text = "";
        int size = 8;
        int x = 0;
        int y = 0;
        Font font;

        UIRect rectMask = UIRect(0, 0, 9999, 9999);
        int rectMaskCornerRadius = -1;

        void Draw() final
        {
            vtxt_setflags(VTXT_CREATE_INDEX_BUFFER);
            vtxt_clear_buffer();
            vtxt_move_cursor(x, y);
            vtxt_append_line_vertex_color_hack(text, font.ptr, size, (float*)CodeCharIndexToColor);
            vtxt_vertex_buffer _txt = vtxt_grab_buffer();
            _txt.vertices_array_count = _txt.vertex_count * 7;

            if (_txt.vertices_array_count > 0)
            {
                RebindBufferObjects(__colored_text_mesh, _txt.vertex_buffer, _txt.index_buffer, _txt.vertices_array_count, _txt.indices_array_count, GL_DYNAMIC_DRAW);

                mat4 matrixModel = mat4();
                Gfx::UseShader(__colored_text_shader);
                Gfx::GLBindMatrix4fv(__colored_text_shader, "matrixModel", 1, matrixModel.ptr());
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, font.textureId);
                Gfx::GLBind1i(__colored_text_shader, "textureSampler0", 0);

                Gfx::GLBind4i(__colored_text_shader, "rectMask", rectMask.x, rectMask.y, rectMask.w, rectMask.h);
                Gfx::GLBind1i(__colored_text_shader, "rectMaskCornerRadius", rectMaskCornerRadius);

                RenderMesh(__colored_text_mesh);
            }

            vtxt_clear_buffer();
        }
    };

    static MemoryLinearBuffer drawRequestsFrameStorageBuffer;
    static std::vector<UIDrawRequest*> drawQueue;
#define MESAIMGUI_NEW_DRAW_REQUEST(type) new (MEMORY_LINEAR_ALLOCATE(&drawRequestsFrameStorageBuffer, type)) type()

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
        drawRequest->font = ui_ss.top().textFont;

        drawQueue.push_back(drawRequest);
    }


    bool ImageButton(UIRect rect, u32 normalTexId, u32 hoveredTexId, u32 activeTexId)
    {
        ui_id id = FreshID();
        bool result = Behaviour_Button(id, rect);

        RectDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(RectDrawRequest);
        drawRequest->rect = rect;
        drawRequest->textureId = IsHovered(id) ? hoveredTexId : normalTexId;
        if (IsActive(id) || result) drawRequest->textureId = activeTexId;

        drawQueue.push_back(drawRequest);

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

        drawQueue.push_back(drawRequest);

        return result;
    }

    void PrimitivePanel(UIRect rect, vec4 colorRGBA)
    {
        RectDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(RectDrawRequest);
        drawRequest->rect = rect;
        drawRequest->color = colorRGBA;

        drawQueue.push_back(drawRequest);
    }

    void PrimitivePanel(UIRect rect, int cornerRadius, vec4 colorRGBA)
    {
        RoundedCornerRectDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(RoundedCornerRectDrawRequest);
        drawRequest->rect = rect;
        drawRequest->color = colorRGBA;
        drawRequest->radius = cornerRadius;

        drawQueue.push_back(drawRequest);
    }

    void PrimitivePanel(UIRect rect, u32 glTextureId)
    {
        RectDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(RectDrawRequest);
        drawRequest->rect = rect;
        drawRequest->textureId = glTextureId;

        drawQueue.push_back(drawRequest);
    }

    void PrimitivePanel(UIRect rect, int cornerRadius, u32 glTextureId, float normalizedCornerSizeInUV)
    {
        CorneredRectDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(CorneredRectDrawRequest);
        drawRequest->rect = rect;
        drawRequest->color = vec4(0.157f, 0.172f, 0.204f, 1.f);
        drawRequest->textureId = glTextureId;
        drawRequest->radius = cornerRadius;
        drawRequest->normalizedCornerSizeInUV = normalizedCornerSizeInUV;

        drawQueue.push_back(drawRequest);
    }

    void PrimitiveTextFmt(int x, int y, int size, TextAlignment alignment, const char* textFmt, ...)
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
        drawRequest->font = ui_ss.top().textFont;
        drawRequest->color = ui_ss.top().textColor;

        drawQueue.push_back(drawRequest);
    }

    void PrimitiveText(int x, int y, int size, TextAlignment alignment, const char* text)
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
        drawRequest->font = ui_ss.top().textFont;
        drawRequest->color = ui_ss.top().textColor;

        drawQueue.push_back(drawRequest);
    }

    void PrimitiveTextMasked(int x, int y, int size, TextAlignment alignment, const char* text, UIRect mask, int maskCornerRadius)
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
        drawRequest->font = ui_ss.top().textFont;
        drawRequest->color = ui_ss.top().textColor;
        drawRequest->rectMask = mask;
        drawRequest->rectMaskCornerRadius = maskCornerRadius;

        drawQueue.push_back(drawRequest);
    }

    void PrimtiveImage(UIRect rect, u32 glTextureId)
    {
        CorneredRectDrawRequest *drawRequest = MESAIMGUI_NEW_DRAW_REQUEST(CorneredRectDrawRequest);
        drawRequest->rect = rect;
        drawRequest->color = vec4(0.157f, 0.172f, 0.204f, 1.f);
        drawRequest->textureId = glTextureId;
        drawRequest->radius = 0;
        drawRequest->normalizedCornerSizeInUV = 0.f;

        drawQueue.push_back(drawRequest);
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

        drawQueue.push_back(drawRequest);

        if (IsActive(id))
        {
            if (activeTextInputBuffer.count > 0)
            {
                PrimitiveText(rect.x + rect.w, rect.y + rect.h, 9, TextAlignment::Right, activeTextInputBuffer.data);
            }
        }
        else
        {
            PrimitiveText(rect.x + rect.w, rect.y + rect.h, 9, TextAlignment::Right, std::to_string(*v).c_str());
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

        drawQueue.push_back(drawRequest);

        if (IsActive(id))
        {
            if (activeTextInputBuffer.count > 0)
            {
                PrimitiveText(rect.x + rect.w, rect.y + rect.h, 14, TextAlignment::Right, activeTextInputBuffer.data);
            }
        }
        else
        {
            char cbuf[32];
            stbsp_sprintf(cbuf, "%.2f", *v);
            PrimitiveText(rect.x + rect.w, rect.y + rect.h, 14, TextAlignment::Right, cbuf);
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

    bool LabelledButton(UIRect rect, const char* label, TextAlignment textAlignment)
    {
        int ascenderTextSize = ui_ss.top().textFont.ptr->font_height_px;
        float yTextPaddingRatio = (1.f - (float(ascenderTextSize) / float(rect.h))) / 2.f;
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

        bool buttonValue = PrimitiveButton(FreshID(), rect, ui_ss.top().buttonNormalColor, ui_ss.top().buttonHoveredColor, ui_ss.top().buttonActiveColor);
        PrimitiveText(textX, rect.y + rect.h - textPadding.y, ascenderTextSize, textAlignment, label);
        return buttonValue;
    }



    void BeginZone(UIRect windowRect)
    {
        if (activeZone.zoneId != null_ui_id)
        {
            PrintLog.Error("BeginZone and EndZone don't match.");
            return;
        }

        ui_id windowId = FreshID();
        activeZone.zoneId = windowId;
        activeZone.zoneRect = windowRect;
        activeZone.topLeftXOffset = 5;
        activeZone.topLeftYOffset = 5;
    }

    void GetWHOfZone(int *w, int *h)
    {
        *w = activeZone.zoneRect.w;
        *h = activeZone.zoneRect.h;
    }

    void GetXYInZone(int *x, int *y)
    {
        *x = activeZone.zoneRect.x + activeZone.topLeftXOffset;
        *y = activeZone.zoneRect.y + activeZone.topLeftYOffset;
    }

    void MoveXYInZone(int x, int y)
    {
        activeZone.topLeftXOffset += x;
        activeZone.topLeftYOffset += y;
    }

    void EndZone()
    {
        activeZone.zoneId = null_ui_id;
    }


    void EditorText(const char* text)
    {
        UIStyle& uistyle = ui_ss.top();

        int x;
        int y;
        GetXYInZone(&x, &y);

        int sz = ui_ss.top().textFont.ptr->font_height_px;

        PrimitiveText(x + uistyle.paddingLeft, y + sz + uistyle.paddingTop, sz, TextAlignment::Left, text);

        MoveXYInZone(0, sz + uistyle.paddingTop + uistyle.paddingBottom);
    }

    bool EditorLabelledButton(const char* label)
    {
        UIStyle& uistyle = ui_ss.top();

        int labelTextSize = uistyle.textFont.ptr->font_height_px;
        float textW;
        float textH;
        vtxt_get_text_bounding_box_info(&textW, &textH, label, uistyle.textFont.ptr, labelTextSize);

        int buttonX;
        int buttonY;
        GetXYInZone(&buttonX, &buttonY);
        buttonX += uistyle.paddingLeft;
        buttonY += uistyle.paddingTop;
        int buttonW = GM_max((int) textW + 4, 50);
        int buttonH = labelTextSize + 4;

        UIRect buttonRect = UIRect(buttonX, buttonY, buttonW, buttonH);
        bool result = LabelledButton(buttonRect, label, TextAlignment::Center);

        MoveXYInZone(0, uistyle.paddingTop + buttonH + uistyle.paddingBottom);

        return result;
    }

    void EditorIncrementableIntegerField(const char* label, int* v, int increment)
    {
        UIStyle& uistyle = ui_ss.top();

        int x;
        int y;
        GetXYInZone(&x, &y);

        int w = 50;
        int h = 9 + 4;
        int paddingAbove = uistyle.paddingTop;
        int paddingBelow = uistyle.paddingBottom;

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
//        PrimitiveText(x + w + 22, y + h, 9, TextAlignment::Left, label);

        MoveXYInZone(0, paddingAbove + h + paddingBelow);
    }

    void EditorIncrementableFloatField(const char* label, float* v, float increment)
    {
        UIStyle& uistyle = ui_ss.top();

        int x, y;
        GetXYInZone(&x, &y);

        int w = 80;
        int h = 20;
        int paddingAbove = uistyle.paddingTop;
        int paddingBelow = uistyle.paddingBottom;

        PrimitivePanel(UIRect(x, y, w-2, h), vec4(0.4f, 0.4f, 0.4f, 1.f));
        PrimitiveFloatInputField(FreshID(), UIRect(x + 1, y + 1, w-4, h - 2), v);
        if (PrimitiveButton(FreshID(), UIRect(x + w, y + 1, 20, (h / 2) - 1), ui_ss.top().buttonNormalColor, ui_ss.top().buttonHoveredColor, ui_ss.top().buttonActiveColor))
        {
            (*v) += increment;
        }
        if (PrimitiveButton(FreshID(), UIRect(x + w, y + (h / 2) + 1, 20, (h / 2) - 1), ui_ss.top().buttonNormalColor, ui_ss.top().buttonHoveredColor, ui_ss.top().buttonActiveColor))
        {
            (*v) -= increment;
        }
        PrimitiveText(x + w + 22, y + h, 20, TextAlignment::Left, label);

        MoveXYInZone(0, paddingAbove + h + paddingBelow);
    }

    bool EditorSelectable(const char *label, bool *selected)
    {
        int x, y;
        GetXYInZone(&x, &y);

        UIRect selectableRegion = UIRect(x, y, 100, 11);
        if (*selected)
        {
            PrimitivePanel(selectableRegion, vec4(0,0,0,0.4f));
            PrimitiveText(x + 1, y + 10, 9, TextAlignment::Left, label);
            MoveXYInZone(0, 11);
        }
        else
        {
            *selected = PrimitiveButton(FreshID(), selectableRegion, vec4(), vec4(0,0,0,0.2f), vec4(0,0,0,0.4f), true);
            PrimitiveText(x + 1, y + 10, 9, TextAlignment::Left, label);
            MoveXYInZone(0, 11);
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
        GetXYInZone(&x, &y);

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
        Gfx::GLCreateShaderProgram(__main_ui_shader, __main_ui_shader_vs, __main_ui_shader_fs);
        Gfx::GLCreateShaderProgram(__rounded_corner_rect_shader, __rounded_corner_rect_shader_vs, __rounded_corner_rect_shader_fs);
        Gfx::GLCreateShaderProgram(__text_shader, __text_shader_vs, __text_shader_fs);
        Gfx::GLCreateShaderProgram(__colored_text_shader, __colored_text_shader_vs, __colored_text_shader_fs);
        MeshCreate(__ui_mesh, nullptr, nullptr, 0, 0, 2, 2, 0, GL_DYNAMIC_DRAW);
        MeshCreate(__text_mesh, nullptr, nullptr, 0, 0, 2, 2, 0, GL_DYNAMIC_DRAW);
        MeshCreate(__colored_text_mesh, nullptr, nullptr, 0, 0, 2, 2, 3, GL_DYNAMIC_DRAW);

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

        keyboardInputASCIIKeycodeThisFrame.ResetCount();
        keyboardInputASCIIKeycodeThisFrame.ResetToZero();

        freshIdCounter = 0;

        __reservedTextMemoryIndexer = 0;
        drawRequestsFrameStorageBuffer.arenaOffset = 0;
        
        // clear draw queue
        drawQueue.clear();

    }

    bool Temp_Escape()
    {
        return keyboardInputASCIIKeycodeThisFrame.Contains(SDLK_ESCAPE);
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

    void Draw()
    {
        // int fontAtlasDebugY = 100;
        // for (int i = 6; i < 7; ++i)
        // {
        //     DoImage(UIRect(10, fontAtlasDebugY, __fonts[i].ptr->font_atlas.width, __fonts[i].ptr->font_atlas.height), __fonts[i].textureId);
        //     fontAtlasDebugY += __fonts[i].ptr->font_atlas.height + 5;
        // }

        i32 kevGuiScreenWidth = Gfx::GetCoreRenderer()->renderTargetGUI.width;
        i32 kevGuiScreenHeight = Gfx::GetCoreRenderer()->renderTargetGUI.height;
        mat4 projectionMatrix = ProjectionMatrixOrthographicNoZ(0.f, (float)kevGuiScreenWidth, (float)kevGuiScreenHeight, 0.f);

        Gfx::UseShader(__main_ui_shader);
        Gfx::GLBindMatrix4fv(__main_ui_shader, "matrixOrtho", 1, projectionMatrix.ptr());

        Gfx::UseShader(__rounded_corner_rect_shader);
        Gfx::GLBindMatrix4fv(__rounded_corner_rect_shader, "matrixOrtho", 1, projectionMatrix.ptr());

        Gfx::UseShader(__text_shader);
        Gfx::GLBindMatrix4fv(__text_shader, "matrixOrtho", 1, projectionMatrix.ptr());

        Gfx::UseShader(__colored_text_shader);
        Gfx::GLBindMatrix4fv(__colored_text_shader, "matrixOrtho", 1, projectionMatrix.ptr());

        for (int i = 0; i < drawQueue.size(); ++i)
        {
            UIDrawRequest *drawCall = drawQueue.at(i);
            drawCall->Draw();
        }
    }

}
