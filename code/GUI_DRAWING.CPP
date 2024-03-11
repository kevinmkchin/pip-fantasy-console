#include <stack>
#include "GUI_DRAWING.H"

#include "singleheaders/vertext.h"
#include "GfxRenderer.h"
#include "PrintLog.h"

static Gfx::Shader __main_ui_shader;
static const char* __main_ui_shader_vs =
        "#version 330 core\n"
        "uniform mat4 matrixOrtho;\n"
        "layout (location = 0) in vec2 pos;\n"
        "layout (location = 1) in vec2 uv;\n"
        "out vec2 texUV;\n"
        "out vec2 fragPos;\n"
        "void main() {\n"
        "    gl_Position = matrixOrtho * vec4(pos, 0.0, 1.0);\n"
        "    texUV = uv;\n"
        "    fragPos = pos;\n"
        "}\n";
static const char* __main_ui_shader_fs =
        "#version 330 core\n"
        "uniform sampler2D textureSampler0;\n"
        "uniform bool useColour = false;\n"
        "uniform vec4 uiColour;\n"
        "uniform ivec4 windowMask;\n"
        "in vec2 texUV;\n"
        "in vec2 fragPos;\n"
        "out vec4 colour;\n"
        "void main() {\n"
        "    vec4 fmask = vec4(windowMask);\n"
        "    bool maskxokay = fmask.x <= fragPos.x && fragPos.x < (fmask.x + fmask.z);\n"
        "    bool maskyokay = fmask.y <= fragPos.y && fragPos.y < (fmask.y + fmask.w);\n"
        "    if (!maskxokay || !maskyokay) {"
        "        colour = vec4(0.0, 0.0, 0.0, 0.0);"
        "    } else if (useColour) {\n"
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
        "uniform ivec4 windowMask;\n"
        "in vec2 fragPos;\n"
        "out vec4 colour;\n"
        "void main() {\n"
        "    vec4 frect = vec4(rect);\n"
        "    float fradius = float(cornerRadius);\n"
        "    bool xokay = (frect.x + fradius) < fragPos.x && fragPos.x < (frect.x + frect.z - fradius);\n"
        "    bool yokay = (frect.y + fradius) < fragPos.y && fragPos.y < (frect.y + frect.w - fradius);\n"
        "\n"
        "    vec4 fmask = vec4(windowMask);\n"
        "    bool maskxokay = fmask.x <= fragPos.x && fragPos.x < (fmask.x + fmask.z);\n"
        "    bool maskyokay = fmask.y <= fragPos.y && fragPos.y < (fmask.y + fmask.w);\n"
        "    if (!maskxokay || !maskyokay) {"
        "        colour = vec4(0.0, 0.0, 0.0, 0.0);"
        "    } else if (xokay || yokay) { \n"
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
        "uniform ivec4 windowMask;\n"
        "uniform int rectMaskCornerRadius;\n"
        "in vec2 fragPos;\n"
        "in vec2 texUV;\n"
        "out vec4 colour;\n"
        "void main() {\n"
        "    float textAlpha = texture(textureSampler0, texUV).x;\n"
        "\n"
        "    vec4 fmask = vec4(windowMask);\n"
        "    bool maskxokay = fmask.x <= fragPos.x && fragPos.x < (fmask.x + fmask.z);\n"
        "    bool maskyokay = fmask.y <= fragPos.y && fragPos.y < (fmask.y + fmask.w);\n"
        "    if (!maskxokay || !maskyokay) {"
        "        colour = vec4(0.0, 0.0, 0.0, 0.0);"
        "    } else if (rectMaskCornerRadius < 0)\n"
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
        "uniform ivec4 windowMask;\n"
        "in vec2 fragPos;\n"
        "in vec2 texUV;\n"
        "in vec3 vertColor;\n"
        "out vec4 colour;\n"
        "void main() {\n"
        "    float textAlpha = texture(textureSampler0, texUV).x;\n"
        "    \n"
        "    vec4 fmask = vec4(windowMask);\n"
        "    bool maskxokay = fmask.x <= fragPos.x && fragPos.x < (fmask.x + fmask.z);\n"
        "    bool maskyokay = fmask.y <= fragPos.y && fragPos.y < (fmask.y + fmask.w);\n"
        "    if (!maskxokay || !maskyokay) {"
        "        colour = vec4(0.0, 0.0, 0.0, 0.0);"
        "    } else if (rectMaskCornerRadius < 0)\n"
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

#define MAX_DRAWCOLLECTIONS_ALLOWED 16

namespace MesaGUI
{
    struct DrawCollectionMetaData
    {
        UIRect windowMask;
        int depth = 0;
    };

    static UIRect activeWindowMask;
    static NiceArray<std::vector<UIDrawRequest*>, MAX_DRAWCOLLECTIONS_ALLOWED + 1> DRAWQSTORAGE;
    static NiceArray<DrawCollectionMetaData, MAX_DRAWCOLLECTIONS_ALLOWED + 1> DRAWQUEUE_METADATA;
    static std::stack<std::vector<UIDrawRequest*>*> DRAWREQCOLLECTIONSTACK;

    void GUIDraw_InitResources()
    {
        Gfx::GLCreateShaderProgram(__main_ui_shader, __main_ui_shader_vs, __main_ui_shader_fs);
        Gfx::GLCreateShaderProgram(__rounded_corner_rect_shader, __rounded_corner_rect_shader_vs, __rounded_corner_rect_shader_fs);
        Gfx::GLCreateShaderProgram(__text_shader, __text_shader_vs, __text_shader_fs);
        Gfx::GLCreateShaderProgram(__colored_text_shader, __colored_text_shader_vs, __colored_text_shader_fs);
        MeshCreate(__ui_mesh, nullptr, nullptr, 0, 0, 2, 2, 0, GL_DYNAMIC_DRAW);
        MeshCreate(__text_mesh, nullptr, nullptr, 0, 0, 2, 2, 0, GL_DYNAMIC_DRAW);
        MeshCreate(__colored_text_mesh, nullptr, nullptr, 0, 0, 2, 2, 3, GL_DYNAMIC_DRAW);

        ASSERT(DRAWQSTORAGE.count == 0);
        ASSERT(DRAWQUEUE_METADATA.count == 0);
        ASSERT(DRAWREQCOLLECTIONSTACK.empty());

        DRAWQSTORAGE.count++;
        DRAWQUEUE_METADATA.PushBack({ UIRect(0,0,9999,9999), 0 });
        DRAWREQCOLLECTIONSTACK.push(&DRAWQSTORAGE.Back());
    }

    void GUIDraw_NewFrame()
    {
        // clear draw queue
        DRAWQSTORAGE.count = 1;
        DRAWQUEUE_METADATA.count = 1;
        if (DRAWREQCOLLECTIONSTACK.size() > 1)
        {
            PrintLog.Error("GUI BeginZone and EndZone don't match.");
        }
    }

    void GUIDraw_DrawEverything()
    {
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

        // Note(Kevin): could sort so its O(n) but realistically how many windows am I going to have...
        for (int depth = 0; depth < 4; ++depth)
        {
            for (int i = 0; i < DRAWQSTORAGE.count; ++i)
            {
                if (DRAWQUEUE_METADATA.At(i).depth == depth)
                {
                    activeWindowMask = DRAWQUEUE_METADATA.At(i).windowMask;
                    std::vector<UIDrawRequest*>& drawQueue = DRAWQSTORAGE.At(i);
                    for (auto drawCall : drawQueue)
                        drawCall->Draw();
                    drawQueue.clear();
                }
            }
        }
    }

    void GUIDraw_PushDrawCollection(UIRect windowMask, int depth)
    {
        ASSERT(DRAWQSTORAGE.NotAtCapacity());
        DRAWQSTORAGE.count++;
        if (depth < 0)
            depth = (int)DRAWREQCOLLECTIONSTACK.size();
        DRAWQUEUE_METADATA.PushBack({ windowMask, depth });
        DRAWREQCOLLECTIONSTACK.push(&DRAWQSTORAGE.Back());
    }

    void GUIDraw_PopDrawCollection()
    {
        ASSERT(DRAWREQCOLLECTIONSTACK.size() > 1);
        DRAWREQCOLLECTIONSTACK.pop();
    }

    void AppendToCurrentDrawRequestsCollection(UIDrawRequest *drawRequest)
    {
        DRAWREQCOLLECTIONSTACK.top()->push_back(drawRequest);
    }

    void RectDrawRequest::Draw()
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
        Gfx::GLBind4i(__main_ui_shader, "windowMask", activeWindowMask.x, activeWindowMask.y, activeWindowMask.w, activeWindowMask.h);

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

    void RoundedCornerRectDrawRequest::Draw()
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
        Gfx::GLBind4i(__main_ui_shader, "windowMask", activeWindowMask.x, activeWindowMask.y, activeWindowMask.w, activeWindowMask.h);
        Gfx::GLBind4i(__rounded_corner_rect_shader, "rect", rect.x, rect.y, rect.w, rect.h);
        Gfx::GLBind1i(__rounded_corner_rect_shader, "cornerRadius", radius);
        Gfx::GLBind4f(__rounded_corner_rect_shader, "uiColour", color.x, color.y, color.z, color.w);

        RebindBufferObjects(__ui_mesh, vb, ib, ARRAY_COUNT(vb), ARRAY_COUNT(ib), GL_DYNAMIC_DRAW);
        RenderMesh(__ui_mesh);
    }

    void CorneredRectDrawRequest::Draw()
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
        Gfx::GLBind4i(__main_ui_shader, "windowMask", activeWindowMask.x, activeWindowMask.y, activeWindowMask.w, activeWindowMask.h);

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

    void TextDrawRequest::Draw()
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

        Gfx::GLBind4i(__text_shader, "windowMask", activeWindowMask.x, activeWindowMask.y, activeWindowMask.w, activeWindowMask.h);

        RenderMesh(__text_mesh);
    }

    void PipCodeDrawRequest::Draw()
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

            Gfx::GLBind4i(__colored_text_shader, "windowMask", activeWindowMask.x, activeWindowMask.y, activeWindowMask.w, activeWindowMask.h);

            RenderMesh(__colored_text_mesh);
        }

        vtxt_clear_buffer();
    }
}

