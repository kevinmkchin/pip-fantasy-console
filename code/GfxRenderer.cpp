#include "GfxRenderer.h"

#include <SDL.h>


#define GL3W_IMPLEMENTATION
#include <gl3w.h>
#include "MesaOpenGL.h"

#include "MesaUtility.h"
#include "PrintLog.h"
#include "MesaIMGUI.h"
#include "MesaMain.h"

// TODO(Kevin): maybe renderer shouldn't need to know about this shit:
#include "Game.h"
#include "Space.h"
#include "MesaScript.h"

namespace Gfx
{
    static SDL_Window* s_ActiveSDLWindow = nullptr;
    static CoreRenderer* s_TheGfxRenderer = nullptr;

    CoreRenderer* GetCoreRenderer()
    {
        return s_TheGfxRenderer;
    }

    static const char* __finalpass_shader_vs =
            "#version 330\n"
            "layout(location = 0) in vec2 pos;\n"
            "layout(location = 1) in vec2 uv;\n"
            "out vec2 texcoord;\n"
            "void main()\n"
            "{\n"
            "    gl_Position = vec4(pos, 0, 1.0);\n"
            "    texcoord = uv;\n"
            "}\n";

    static const char* __finalpass_shader_fs =
            "#version 330\n"
            "uniform sampler2D screen_texture;\n"
            "in vec2 texcoord;\n"
            "out vec4 color;\n"
            "void main()\n"
            "{\n"
            "    vec4 in_color = texture(screen_texture, texcoord);\n"
            "    if(in_color.w < 0.001)\n"
            "    {\n"
            "        discard;\n"
            "    }\n"
            "    color = in_color;\n"
            "}\n";

    static const char* __sprite_shader_vs =
            "#version 330\n"
            "// Input attributes\n"
            "layout (location = 0) in vec2 vs_pos;\n"
            "layout (location = 1) in vec2 vs_uv;\n"
            "// Passed to fragment shader\n"
            "out vec2 fs_uv;\n"
            "// Application data\n"
            "uniform mat3 model;\n"
            "uniform mat3 view;\n"
            "uniform mat3 projection;\n"
            "void main()\n"
            "{\n"
            "    fs_uv = vs_uv;\n"
            "    vec3 pos = projection * view * model * vec3(vs_pos, 1.0);\n"
            "    gl_Position = vec4(pos.xy, 0.0, 1.0);\n"
            "}\n";

    static const char* __sprite_shader_fs =
            "#version 330\n"
            "// From vertex shader\n"
            "in vec2 fs_uv;\n"
            "// Application data\n"
            "uniform sampler2D sampler0;\n"
            "uniform vec3 fragmentColor;\n"
            "// Output color\n"
            "layout (location = 0) out vec4 color;\n"
            "void main()\n"
            "{\n"
            "    color = vec4(fragmentColor, 1.0) * texture(sampler0, fs_uv);\n"
            "}\n";


    bool CoreRenderer::Init()
    {
    #ifdef MESA_USING_GL3W
        if (gl3w_init())
        {
            fprintf(stderr, "Failed to initialize OpenGL\n");
            return false;
        }
        //PrintLog.Message("--OpenGL initialized.");
    #endif
        s_ActiveSDLWindow = SDL_GL_GetCurrentWindow();

        // alpha blending func: (srcRGB) * srcA + (dstRGB) * (1 - srcA)  = final color output
        // alpha blending func: (srcA) * a + (dstA) * 1 = final alpha output
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
        glBlendEquation(GL_FUNC_ADD);

        CreateFrameBuffers();

        GLCreateShaderProgram(finalPassShader, __finalpass_shader_vs, __finalpass_shader_fs);
        GLCreateShaderProgram(spriteShader, __sprite_shader_vs, __sprite_shader_fs);

        CreateMiscellaneous();

        UpdateBackBufferAndGameSize();

        s_TheGfxRenderer = this;

        return true;
    }

    void CoreRenderer::Render()
    {
        if (CurrentProgramMode() == MesaProgramMode::Game)
        {
            RenderGameLayer();            
        }
        RenderGUILayer();
        //RenderDebugUILayer();

        FinalRenderToBackBuffer();
    }

    void CoreRenderer::RenderGameLayer()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, gameLayer.FBO);
        glViewport(0, 0, gameLayer.width, gameLayer.height);
        glClearColor(RGB255TO1(211, 203, 190), 1.f);//(0.674f, 0.847f, 1.0f, 1.f); //RGB255TO1(46, 88, 120)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
        glDisable(GL_DEPTH_TEST);

        UseShader(spriteShader);

        static mat3 orthographicMatrix = mat3(ProjectionMatrixOrthographic2D(0.f, float(internalGameResolutionW), 0.f, float(internalGameResolutionH)));
        static mat3 identityMatrix = mat3();

        GLBindMatrix3fv(spriteShader, "projection", 1, orthographicMatrix.ptr());
        GLBindMatrix3fv(spriteShader, "view", 1, identityMatrix.ptr());

        mat3 modelMatrix = identityMatrix;

        Space* space = GetGameActiveSpace();
        if (space->aliveUpdateAndDraw.size() > 0)
        {
            EntityInstance e = space->aliveUpdateAndDraw[0];
            MesaScript_Table* table = AccessMesaScriptTable(e.selfMapId);
            TValue xtv = table->AccessMapEntry("x");
            TValue ytv = table->AccessMapEntry("y");
            modelMatrix[2][0] = float(xtv.type == TValue::ValueType::Integer ? xtv.integerValue : xtv.realValue);
            modelMatrix[2][1] = float(ytv.type == TValue::ValueType::Integer ? ytv.integerValue : ytv.realValue);
        }

        GLBindMatrix3fv(spriteShader, "model", 1, modelMatrix.ptr());

        static TextureHandle mushroom = CreateGPUTextureFromDisk(data_path("mushroom.png").c_str());

        const i32 numQuads = 1;
        const u32 verticesCount = 16 * numQuads;
        const u32 indicesCount = 6 * numQuads;
        float vb[verticesCount];
        u32 ib[indicesCount];

        vb[0] = 0;
        vb[1] = 0;
        vb[2] = 0;
        vb[3] = 1;
        vb[4] = 16;
        vb[5] = 0;
        vb[6] = 1;
        vb[7] = 1;
        vb[8] = 0;
        vb[9] = -16;
        vb[10] = 0;
        vb[11] = 0;
        vb[12] = 16;
        vb[13] = -16;
        vb[14] = 1;
        vb[15] = 0;

        ib[0] = 0;
        ib[1] = 2;
        ib[2] = 1;
        ib[3] = 2;
        ib[4] = 3;
        ib[5] = 1;

        // pass VBO (x, y, u, v) and IBO to shader
        static u32 spriteBatchVAO = 0;
        static u32 spriteBatchVBO = 0;
        static u32 spriteBatchIBO = 0;
        if(!spriteBatchVAO)
        {
            glGenVertexArrays(1, &spriteBatchVAO);
            glBindVertexArray(spriteBatchVAO);

            glGenBuffers(1, &spriteBatchVBO);
            glBindBuffer(GL_ARRAY_BUFFER, spriteBatchVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, nullptr, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2)); // i really feel like this can be nullptr
            glEnableVertexAttribArray(1);

            glGenBuffers(1, &spriteBatchIBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, spriteBatchIBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * 6, nullptr, GL_DYNAMIC_DRAW);
        }
        glBindVertexArray(spriteBatchVAO);
        glBindBuffer(GL_ARRAY_BUFFER, spriteBatchVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * verticesCount, vb, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, spriteBatchIBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * indicesCount, ib, GL_DYNAMIC_DRAW);

        // set Sampler2D/int sampler0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mushroom.textureId);
        GLBind1i(spriteShader, "sampler0", 0);
        // set vec3 fragmentColor 
        GLBind3f(spriteShader, "fragmentColor", 1.f, 1.f, 1.f);

        // draw
        glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, nullptr);
    }

    void CoreRenderer::RenderGUILayer()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, guiLayer.FBO);
        glViewport(0, 0, guiLayer.width, guiLayer.height);
        glDepthRange(0.00001f, 10.f);
        glClearColor(RGB255TO1(244,194,194), 0.0f);
        glClearDepth(10.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
        glDisable(GL_DEPTH_TEST);

        MesaGUI::Draw();
    }

    //void CoreRenderer::RenderDebugUILayer()
    //{
    //    glBindFramebuffer(GL_FRAMEBUFFER, debugUILayer.FBO);
    //    glViewport(0, 0, debugUILayer.width, debugUILayer.height);
    //    glDepthRange(0.00001f, 10.f);
    //    glClearColor(0.674f, 0.847f, 1.0f, 0.0f);
    //    glClearDepth(10.f);
    //    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //
    //    // CONSOLE RENDERING
    //    glDisable(GL_DEPTH_TEST);
    //    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    //    console_render();
    //    glEnable(GL_DEPTH_TEST);
    //}

    void CoreRenderer::FinalRenderToBackBuffer()
    {
        UseShader(finalPassShader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, windowDrawableHeight - backBufferHeight, backBufferWidth, backBufferHeight);
        glDepthRange(0, 10);
        glClearColor(RGB255TO1(0, 0, 0), 1.f);
        glClearDepth(1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
        glDisable(GL_DEPTH_TEST);


        if (CurrentProgramMode() == MesaProgramMode::Game)
        {
            // Draw game frame
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gameLayer.colorTexId);
            RenderMesh(screenSizeQuad);
        }

        // Draw GUI frame
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, guiLayer.colorTexId);
        RenderMesh(screenSizeQuad);

    //    // Draw Debug UI frame
    //    glActiveTexture(GL_TEXTURE0);
    //    glBindTexture(GL_TEXTURE_2D, debugUILayer.colorTexId);
    //    RenderMesh(screenSizeQuad);

        GLHasErrors();
    }

    void CoreRenderer::UpdateBackBufferAndGameSize()
    {
        SDL_GL_GetDrawableSize(s_ActiveSDLWindow, &windowDrawableWidth, &windowDrawableHeight);

        backBufferWidth = GM_max(windowDrawableWidth - (windowDrawableWidth % (int)screenScaling), 160);
        backBufferHeight = GM_max(windowDrawableHeight - (windowDrawableHeight % (int)screenScaling), 160);

        internalGameResolutionW = backBufferWidth / (int)screenScaling;
        internalGameResolutionH = backBufferHeight / (int)screenScaling;

        UpdateFrameBuffersSize();
        UpdateScreenSizeQuad();
    }

    void CoreRenderer::GetBackBufferSize(i32* widthOutput, i32* heightOutput)
    {
        *widthOutput = backBufferWidth;
        *heightOutput = backBufferHeight;
    }

    void CoreRenderer::CreateFrameBuffers()
    {
        gameLayer.width = internalGameResolutionW;
        gameLayer.height = internalGameResolutionH;
        CreateBasicFrameBuffer(&gameLayer);
        guiLayer.width = internalGameResolutionW;
        guiLayer.height = internalGameResolutionH;
        CreateBasicFrameBuffer(&guiLayer);
    //    debugUILayer.width = internalGameResolutionW;
    //    debugUILayer.height = internalGameResolutionH;
    //    CreateBasicFrameBuffer(&debugUILayer);
    }

    void CoreRenderer::UpdateFrameBuffersSize()
    {
        UpdateBasicFrameBufferSize(&gameLayer, internalGameResolutionW, internalGameResolutionH);
        UpdateBasicFrameBufferSize(&guiLayer, internalGameResolutionW, internalGameResolutionH); // TODO(Kevin): gui layer should use different resolution to game
    //    UpdateBasicFrameBufferSize(&debugUILayer, backBufferWidth, backBufferHeight);
    }

    void CoreRenderer::CreateMiscellaneous()
    {
        u32 refQuadIndices[6] = {
                0, 1, 3,
                0, 3, 2
        };
        float refQuadVertices[16] = {
                //  x   y    u    v
                -1.f, -1.f, 0.f, 0.f,
                1.f, -1.f, 1.f, 0.f,
                -1.f, 1.f, 0.f, 1.f,
                1.f, 1.f, 1.f, 1.f
        };

        MeshCreate(screenSizeQuad, refQuadVertices, refQuadIndices, 16, 6, 2, 2, 0, GL_STATIC_DRAW);
    }

    void CoreRenderer::UpdateScreenSizeQuad()
    {
        // 2023-10-03 (Kevin): No need anymore.

        //// 2023-08-08 (Kevin): This is basically what Celeste does in windowed mode. It
        //// adds black borders horizontally or vertically to maintain the fixed aspect ratio
        //// of the game. It is inevitable that some pixels are going to be rendered across 
        //// more screen pixels than others (e.g. 5 screen pixels for pixel A vs 4 for B).

        //u32 refQuadIndices[6] = {
        //        0, 1, 3,
        //        0, 3, 2
        //};
        //double bw = (double)backBufferWidth;
        //double bh = (double)backBufferHeight;
        //double internal_ratio = (double)internalGameResolutionW / (double)internalGameResolutionH;
        //double screen_ratio = (double)bw / (double)bh;
        //float finalOutputQuadVertices[16] = {
        //        -1.f, -1.f, 0.f, 0.f,
        //        1.f, -1.f, 1.f, 0.f,
        //        -1.f, 1.f, 0.f, 1.f,
        //        1.f, 1.f, 1.f, 1.f
        //};
        //if(screen_ratio > internal_ratio)
        //{
        //    double w = bh * internal_ratio;
        //    float f = float((bw - w) / bw);
        //    finalOutputQuadVertices[0] = -1.f + f;
        //    finalOutputQuadVertices[4] = 1.f - f;
        //    finalOutputQuadVertices[8] = -1.f + f;
        //    finalOutputQuadVertices[12] = 1.f - f;
        //}
        //else
        //{
        //    double h = bw / internal_ratio;
        //    float f = float((bh - h) / bh);
        //    finalOutputQuadVertices[1] = -1.f + f;
        //    finalOutputQuadVertices[5] = -1.f + f;
        //    finalOutputQuadVertices[9] = 1.f - f;
        //    finalOutputQuadVertices[13] = 1.f - f;
        //}
        //RebindBufferObjects(screenSizeQuad, finalOutputQuadVertices, refQuadIndices, 16, 6);
    }
   
}

