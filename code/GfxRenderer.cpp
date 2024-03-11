#include "GfxRenderer.h"

#include <SDL.h>


#define GL3W_IMPLEMENTATION
#include <gl3w.h>
#include "MesaOpenGL.h"

#include "MesaUtility.h"
#include "PrintLog.h"
#include "MesaIMGUI.h"
#include "MesaMain.h"

namespace Gfx
{
    static SDL_Window *s_ActiveSDLWindow = nullptr;
    static CoreRenderer *s_TheGfxRenderer = nullptr;

    CoreRenderer *GetCoreRenderer()
    {
        return s_TheGfxRenderer;
    }

    static const char *__finalpass_shader_vs =
            "#version 330\n"
            "layout(location = 0) in vec2 pos;\n"
            "layout(location = 1) in vec2 uv;\n"
            "out vec2 texcoord;\n"
            "void main()\n"
            "{\n"
            "    gl_Position = vec4(pos, 0, 1.0);\n"
            "    texcoord = uv;\n"
            "}\n";

    static const char *__finalpass_shader_fs =
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

    static const char *__sprite_shader_vs =
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

    static const char *__sprite_shader_fs =
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

    static const char *__primitive_shader_vs =
            "#version 330\n"
            "layout (location = 0) in vec2 vs_pos;\n"
            "layout (location = 1) in vec4 vs_color;\n"
            "out vec4 fs_color;"
            "uniform mat3 model;"
            "uniform mat3 view;"
            "uniform mat3 projection;"
            "void main() {"
            "    fs_color = vs_color;"
            "    vec3 pos = projection * view * model * vec3(vs_pos, 1.0);"
            "    gl_Position = vec4(pos.xy, 0.0, 1.0);"
            "}";

    static const char *__primitive_shader_fs =
            "#version 330\n"
            "in vec4 fs_color;"
            "layout (location = 0) out vec4 color;"
            "void main() {"
            "    color = fs_color;"
            "}";

    static Mesh __final_render_output_quad;

    static std::vector<RenderQueueData> gameRenderQueue;
    ivec2 gameCamera0Position = ivec2();
    static bool gameClearFlag = false;
    static vec4 gameClearColor = vec4();
    static std::vector<float> gameLayer_PrimitiveVB;

    void QueueSpriteForRender(i64 spriteId, vec2 position)
    {
        RenderQueueData dat;
        dat.sprite = gamedata.sprites.at(spriteId);
        dat.position = position;
        gameRenderQueue.push_back(dat);
    }

    void SetGameLayerClearColor(vec4 color)
    {
        gameClearFlag = true;
        gameClearColor = color / 255.f;
    }

    void Primitive_DrawRect(float x, float y, float w, float h, vec4 color)
    {
        gameLayer_PrimitiveVB.push_back(x);
        gameLayer_PrimitiveVB.push_back(y);
        gameLayer_PrimitiveVB.push_back(color.x);
        gameLayer_PrimitiveVB.push_back(color.y);
        gameLayer_PrimitiveVB.push_back(color.z);
        gameLayer_PrimitiveVB.push_back(color.w);

        gameLayer_PrimitiveVB.push_back(x + w);
        gameLayer_PrimitiveVB.push_back(y);
        gameLayer_PrimitiveVB.push_back(color.x);
        gameLayer_PrimitiveVB.push_back(color.y);
        gameLayer_PrimitiveVB.push_back(color.z);
        gameLayer_PrimitiveVB.push_back(color.w);

        gameLayer_PrimitiveVB.push_back(x + w);
        gameLayer_PrimitiveVB.push_back(y + h);
        gameLayer_PrimitiveVB.push_back(color.x);
        gameLayer_PrimitiveVB.push_back(color.y);
        gameLayer_PrimitiveVB.push_back(color.z);
        gameLayer_PrimitiveVB.push_back(color.w);

        gameLayer_PrimitiveVB.push_back(x);
        gameLayer_PrimitiveVB.push_back(y);
        gameLayer_PrimitiveVB.push_back(color.x);
        gameLayer_PrimitiveVB.push_back(color.y);
        gameLayer_PrimitiveVB.push_back(color.z);
        gameLayer_PrimitiveVB.push_back(color.w);

        gameLayer_PrimitiveVB.push_back(x + w);
        gameLayer_PrimitiveVB.push_back(y + h);
        gameLayer_PrimitiveVB.push_back(color.x);
        gameLayer_PrimitiveVB.push_back(color.y);
        gameLayer_PrimitiveVB.push_back(color.z);
        gameLayer_PrimitiveVB.push_back(color.w);

        gameLayer_PrimitiveVB.push_back(x);
        gameLayer_PrimitiveVB.push_back(y + h);
        gameLayer_PrimitiveVB.push_back(color.x);
        gameLayer_PrimitiveVB.push_back(color.y);
        gameLayer_PrimitiveVB.push_back(color.z);
        gameLayer_PrimitiveVB.push_back(color.w);
    }

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
        GLCreateShaderProgram(primitiveShader, __primitive_shader_vs, __primitive_shader_fs);

        CreateMiscellaneous();

        UpdateBackBufferAndGUILayerSizeToMatchWindowSizeIntegerScaled();

        s_TheGfxRenderer = this;

        return true;
    }

    // BasicFrameBuffer CoreRenderer::RenderTheFuckingWorldEditor(SpaceAsset *worldToView, EditorState *state, EditorWorldViewInfo worldViewInfo)
    // {
    //     UpdateBasicFrameBufferSize(&worldEditorView, worldViewInfo.dimInUIScale.x, worldViewInfo.dimInUIScale.y);

    //     glBindFramebuffer(GL_FRAMEBUFFER, worldEditorView.FBO);
    //     glViewport(0, 0, worldViewInfo.dimInUIScale.x, worldViewInfo.dimInUIScale.y);
    //     glClearColor(RGBHEXTO1(0x6495ed), 1.f);
    //     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //     glEnable(GL_BLEND);
    //     glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    //     glDisable(GL_DEPTH_TEST);

    //     UseShader(spriteShader);

    //     mat3 orthographicMatrix = mat3(ProjectionMatrixOrthographic2D(0.f, float(worldViewInfo.dimAfterZoom.x), 0.f, float(worldViewInfo.dimAfterZoom.y)));
    //     mat3 viewMatrix = mat3();
    //     viewMatrix[2][0] = (float)worldViewInfo.pan.x;
    //     viewMatrix[2][1] = (float)worldViewInfo.pan.y;

    //     GLBindMatrix3fv(spriteShader, "projection", 1, orthographicMatrix.ptr());
    //     GLBindMatrix3fv(spriteShader, "view", 1, viewMatrix.ptr());

    //     mat3 modelMatrix = mat3();

    //     static TextureHandle mushroom = CreateGPUTextureFromDisk(data_path("mushroom.png").c_str());

    //     const i32 numQuads = 1;
    //     const u32 verticesCount = 16 * numQuads;
    //     const u32 indicesCount = 6 * numQuads;
    //     float vb[verticesCount];
    //     u32 ib[indicesCount];

    //     // pass VBO (x, y, u, v) and IBO to shader
    //     static u32 spriteBatchVAO = 0;
    //     static u32 spriteBatchVBO = 0;
    //     static u32 spriteBatchIBO = 0;
    //     if(!spriteBatchVAO)
    //     {
    //         glGenVertexArrays(1, &spriteBatchVAO);
    //         glBindVertexArray(spriteBatchVAO);

    //         glGenBuffers(1, &spriteBatchVBO);
    //         glBindBuffer(GL_ARRAY_BUFFER, spriteBatchVBO);
    //         glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, nullptr, GL_DYNAMIC_DRAW);
    //         glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
    //         glEnableVertexAttribArray(0);
    //         glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2)); // i really feel like this can be nullptr
    //         glEnableVertexAttribArray(1);

    //         glGenBuffers(1, &spriteBatchIBO);
    //         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, spriteBatchIBO);
    //         glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * 6, nullptr, GL_DYNAMIC_DRAW);
    //     }

    //     // for (size_t i = 0; i < worldToView->placedEntities.size(); ++i)
    //     // {
    //     //     EntityAssetInstanceInSpace eai = worldToView->placedEntities[i];

    //     //     EntityAsset *ea = EditorState::ActiveEditorState()->RetrieveEntityAssetById(eai.entityAssetId);
    //     //     float sprw = (float)ea->sprite.width;
    //     //     float sprh = (float)ea->sprite.height;
    //     //     int sprtexid = ea->sprite.textureId;
    //     //     if (sprtexid == 0) sprtexid = mushroom.textureId;

    //     //     vb[0] = 0;
    //     //     vb[1] = 0;
    //     //     vb[2] = 0;
    //     //     vb[3] = 1;

    //     //     vb[4] = sprw;
    //     //     vb[5] = 0;
    //     //     vb[6] = 1;
    //     //     vb[7] = 1;

    //     //     vb[8] = 0;
    //     //     vb[9] = -sprh;
    //     //     vb[10] = 0;
    //     //     vb[11] = 0;

    //     //     vb[12] = sprw;
    //     //     vb[13] = -sprh;
    //     //     vb[14] = 1;
    //     //     vb[15] = 0;

    //     //     ib[0] = 0;
    //     //     ib[1] = 2;
    //     //     ib[2] = 1;
    //     //     ib[3] = 2;
    //     //     ib[4] = 3;
    //     //     ib[5] = 1;

    //     //     modelMatrix[2][0] = (float)eai.spaceX;
    //     //     modelMatrix[2][1] = (float)eai.spaceY;
    //     //     // int eaid = eai.entityAssetId;
    //     //     // state->RetrieveEntityAssetById(eaid);

    //     //     GLBindMatrix3fv(spriteShader, "model", 1, modelMatrix.ptr());

    //     //     glBindVertexArray(spriteBatchVAO);
    //     //     glBindBuffer(GL_ARRAY_BUFFER, spriteBatchVBO);
    //     //     glBufferData(GL_ARRAY_BUFFER, sizeof(float) * verticesCount, vb, GL_DYNAMIC_DRAW);
    //     //     glBindBuffer(GL_ARRAY_BUFFER, spriteBatchIBO);
    //     //     glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * indicesCount, ib, GL_DYNAMIC_DRAW);

    //     //     // set Sampler2D/int sampler0
    //     //     glActiveTexture(GL_TEXTURE0);
    //     //     glBindTexture(GL_TEXTURE_2D, sprtexid);
    //     //     GLBind1i(spriteShader, "sampler0", 0);
    //     //     // set vec3 fragmentColor 
    //     //     GLBind3f(spriteShader, "fragmentColor", 1.f, 1.f, 1.f);

    //     //     // draw
    //     //     glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, nullptr);
    //     // }

    //     return worldEditorView;
    // }

    void CoreRenderer::RenderEditor()
    {
        RenderGUILayer();
        FinalRenderToBackBuffer();
    }


    void CoreRenderer::RenderGame()
    {
        RenderGameLayer();
        RenderGUILayer();
        FinalRenderToBackBuffer();
    }

    void CoreRenderer::RenderGameLayer()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, renderTargetGame.FBO);
        glViewport(0, 0, renderTargetGame.width, renderTargetGame.height);
        if (gameClearFlag)
        {
            gameClearFlag = false;
            glClearColor(gameClearColor.x, gameClearColor.y,
                         gameClearColor.z, gameClearColor.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            //RGBHEXTO1(0x6495ed), 1.f);//(RGB255TO1(211, 203, 190), 1.f);//(0.674f, 0.847f, 1.0f, 1.f); //RGB255TO1(46, 88, 120)
        }
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
        glDisable(GL_DEPTH_TEST);

        UseShader(spriteShader);

        static mat3 projectionMatrix = ProjectionMatrixOrthographic2D(0.f, float(renderTargetGame.width), float(renderTargetGame.height), 0.f);
        static mat3 viewMatrix = mat3();
        viewMatrix[2][0] = (float)-gameCamera0Position.x;
        viewMatrix[2][1] = (float)-gameCamera0Position.y;

        GLBindMatrix3fv(spriteShader, "projection", 1, projectionMatrix.ptr());
        GLBindMatrix3fv(spriteShader, "view", 1, viewMatrix.ptr());

        static TextureHandle mushroom = CreateGPUTextureFromDisk(data_path("mushroom.png").c_str());

        const i32 numQuads = 1;
        const u32 verticesCount = 16 * numQuads;
        const u32 indicesCount = 6 * numQuads;
        float vb[verticesCount];
        u32 ib[indicesCount];

        // pass VBO (x, y, u, v) and IBO to shader
        static u32 spriteBatchVAO = 0;
        static u32 spriteBatchVBO = 0;
        static u32 spriteBatchIBO = 0;
        if (!spriteBatchVAO)
        {
            glGenVertexArrays(1, &spriteBatchVAO);
            glBindVertexArray(spriteBatchVAO);

            glGenBuffers(1, &spriteBatchVBO);
            glBindBuffer(GL_ARRAY_BUFFER, spriteBatchVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, nullptr, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4,
                                  (void *) (sizeof(float) * 2)); // i really feel like this can be nullptr
            glEnableVertexAttribArray(1);

            glGenBuffers(1, &spriteBatchIBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, spriteBatchIBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * 6, nullptr, GL_DYNAMIC_DRAW);
        }

        mat3 modelMatrix = mat3();

        for (RenderQueueData renderData : gameRenderQueue)
        {
            float sprw = (float) renderData.sprite.width;
            float sprh = (float) renderData.sprite.height;
            int sprtexid = renderData.sprite.textureId;
            if (sprtexid == 0) sprtexid = mushroom.textureId;

            vb[0] = 0;
            vb[1] = 0;
            vb[2] = 0;
            vb[3] = 1;

            vb[4] = sprw;
            vb[5] = 0;
            vb[6] = 1;
            vb[7] = 1;

            vb[8] = 0;
            vb[9] = sprh;
            vb[10] = 0;
            vb[11] = 0;

            vb[12] = sprw;
            vb[13] = sprh;
            vb[14] = 1;
            vb[15] = 0;

            ib[0] = 0;
            ib[1] = 2;
            ib[2] = 1;
            ib[3] = 2;
            ib[4] = 3;
            ib[5] = 1;

            modelMatrix[2][0] = renderData.position.x;
            modelMatrix[2][1] = renderData.position.y;

            GLBindMatrix3fv(spriteShader, "model", 1, modelMatrix.ptr());

            glBindVertexArray(spriteBatchVAO);
            glBindBuffer(GL_ARRAY_BUFFER, spriteBatchVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * verticesCount, vb, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, spriteBatchIBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * indicesCount, ib, GL_DYNAMIC_DRAW);

            // set Sampler2D/int sampler0
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sprtexid);
            GLBind1i(spriteShader, "sampler0", 0);
            // set vec3 fragmentColor 
            GLBind3f(spriteShader, "fragmentColor", 1.f, 1.f, 1.f);

            // draw
            glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, nullptr);
        }


        // PRIMITIVE STUFF
        UseShader(primitiveShader);
        GLBindMatrix3fv(primitiveShader, "projection", 1, projectionMatrix.ptr());
        GLBindMatrix3fv(primitiveShader, "view", 1, viewMatrix.ptr());
        modelMatrix = mat3();
        GLBindMatrix3fv(primitiveShader, "model", 1, modelMatrix.ptr());

        static u32 prmVAO = 0;
        static u32 prmVBO = 0;
        if (!prmVAO)
        {
            glGenVertexArrays(1, &prmVAO);
            glBindVertexArray(prmVAO);

            glGenBuffers(1, &prmVBO);
            glBindBuffer(GL_ARRAY_BUFFER, prmVBO);
            glBufferData(GL_ARRAY_BUFFER, (int) sizeof(float) * 1, nullptr, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6, nullptr);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *) (sizeof(float) * 2));
            glEnableVertexAttribArray(1);
        }

        int prmvbsz = (int) gameLayer_PrimitiveVB.size();
        glBindVertexArray(prmVAO);
        glBindBuffer(GL_ARRAY_BUFFER, prmVBO);
        glBufferData(GL_ARRAY_BUFFER, (int) sizeof(float) * prmvbsz, gameLayer_PrimitiveVB.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, prmvbsz / 6);

        // RESET GAME FRAME RENDER DATA
        gameRenderQueue.clear();
        gameClearColor = vec4(0.f, 0.f, 0.f, 0.f);
        gameLayer_PrimitiveVB.clear();
    }

    void CoreRenderer::RenderGUILayer()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, renderTargetGUI.FBO);
        glViewport(0, 0, renderTargetGUI.width, renderTargetGUI.height);
        glDepthRange(0.00001f, 10.f);
        glClearColor(RGB255TO1(244, 194, 194), 0.0f);
        glClearDepth(10.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
        glDisable(GL_DEPTH_TEST);

        Gui::Draw();
    }

    void CoreRenderer::ConfigureViewportForFinalRender() const
    {
        if (CurrentProgramMode() == MesaProgramMode::Game)
        {
            int viewportX, viewportY, viewportWidth, viewportHeight;
            GetViewportValuesForFixedGameResolution(&viewportX, &viewportY, &viewportWidth, &viewportHeight);
            glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
        } else
        {
            glViewport(0, windowDrawableHeight - backBufferHeight, backBufferWidth, backBufferHeight);
        }
    }

    void CoreRenderer::GetViewportValuesForFixedGameResolution(int *viewportX, int *viewportY, int *viewportW, int *viewportH) const
    {
        // Note(Kevin): Provides viewport region to uniformly upscale the game render. This
        // is basically what Celeste does in windowed mode. It adds black borders horizontally
        // or vertically to maintain the fixed aspect ratio of the game. It is inevitable that
        // some pixels are going to be rendered across more screen pixels than others
        // (e.g. 5 screen pixels for pixel A vs 4 for B).

        *viewportW = backBufferWidth;
        *viewportH = backBufferHeight;

        double bw = (double) backBufferWidth;
        double bh = (double) backBufferHeight;
        double internalRatio = (double) renderTargetGame.width / (double) renderTargetGame.height;
        double screenRatio = (double) bw / (double) bh;
        if (screenRatio > internalRatio)
        {
            *viewportW = int(backBufferHeight * internalRatio);
        } else if (screenRatio < internalRatio)
        {
            *viewportH = int(backBufferWidth / internalRatio);
        }

        *viewportX = (backBufferWidth - *viewportW) / 2;
        *viewportY = (backBufferHeight - *viewportH) / 2;
    }

    void CoreRenderer::FinalRenderToBackBuffer()
    {
        UseShader(finalPassShader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        ConfigureViewportForFinalRender();
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
            glBindTexture(GL_TEXTURE_2D, renderTargetGame.colorTexId);
            RenderMesh(__final_render_output_quad);
        }

        // Draw GUI frame
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderTargetGUI.colorTexId);
        RenderMesh(__final_render_output_quad);

        //    // Draw Debug UI frame
        //    glActiveTexture(GL_TEXTURE0);
        //    glBindTexture(GL_TEXTURE_2D, debugUILayer.colorTexId);
        //    RenderMesh(screenSizeQuad);

        GLHasErrors();
    }


    void CoreRenderer::GetBackBufferSize(i32 *widthOutput, i32 *heightOutput) const
    {
        *widthOutput = backBufferWidth;
        *heightOutput = backBufferHeight;
    }

    void CoreRenderer::WindowSizeChanged()
    {
        if (CurrentProgramMode() == MesaProgramMode::Game)
        {
            // Note(Kevin): For games, GUI render target size is not driven by window size.
            //              I just need to update the window size i.e. the actual back buffer size.
            SDL_GL_GetDrawableSize(s_ActiveSDLWindow, &windowDrawableWidth, &windowDrawableHeight);
            backBufferWidth = GM_max(windowDrawableWidth, 160);
            backBufferHeight = GM_max(windowDrawableHeight, 160);
        } else
        {
            UpdateBackBufferAndGUILayerSizeToMatchWindowSizeIntegerScaled();
        }
    }

    void CoreRenderer::ChangeEditorIntegerScaleAndInvokeWindowSizeChanged(PixelPerfectRenderScale scale)
    {
        editorIntegerScale = scale;
        WindowSizeChanged();
    }

    void CoreRenderer::UpdateBackBufferAndGUILayerSizeToMatchWindowSizeIntegerScaled()
    {
        SDL_GL_GetDrawableSize(s_ActiveSDLWindow, &windowDrawableWidth, &windowDrawableHeight);

        // Note (Kevin): Window size minus some pixels so it's divisible by screen integer scale
        backBufferWidth = GM_max(windowDrawableWidth - (windowDrawableWidth % (int) editorIntegerScale), 160);
        backBufferHeight = GM_max(windowDrawableHeight - (windowDrawableHeight % (int) editorIntegerScale), 160);

        // Note (Kevin): Update GUI render target size
        const i32 renderTargetGUIDesiredWidth = backBufferWidth / (int) editorIntegerScale;
        const i32 renderTargetGUIDesiredHeight = backBufferHeight / (int) editorIntegerScale;
        UpdateBasicFrameBufferSize(&renderTargetGUI, renderTargetGUIDesiredWidth, renderTargetGUIDesiredHeight);
    }


    void CoreRenderer::CreateFrameBuffers()
    {
        // Note(Kevin): dummy values for initialization
        renderTargetGame.width = 800;
        renderTargetGame.height = 600;
        CreateBasicFrameBuffer(&renderTargetGame);
        renderTargetGUI.width = 800;
        renderTargetGUI.height = 600;
        CreateBasicFrameBuffer(&renderTargetGUI);
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
        MeshCreate(__final_render_output_quad, refQuadVertices, refQuadIndices, 16, 6, 2, 2, 0, GL_STATIC_DRAW);

        worldEditorView.width = 450;
        worldEditorView.height = 450;
        CreateBasicFrameBuffer(&worldEditorView);
    }

    ivec2 CoreRenderer::TransformWindowCoordinateToEditorGUICoordinateSpace(ivec2 winCoord) const
    {
        if (winCoord.x >= backBufferWidth || winCoord.y >= backBufferHeight)
            return {-1, -1};
        return {winCoord.x / (int)editorIntegerScale, winCoord.y / (int)editorIntegerScale};
    }

    ivec2 CoreRenderer::TransformWindowCoordinateToGameWorldSpace(ivec2 winCoord) const
    {
        int vx, vy, vw, vh;
        GetViewportValuesForFixedGameResolution(&vx, &vy, &vw, &vh);
        int vyTopRelative = backBufferHeight - vy - vh;
        int mouseXRelativeToGameViewport_InWindowSpace = winCoord.x - vx;
        int mouseYRelativeToGameViewport_InWindowSpace = winCoord.y - vyTopRelative;
        double gameViewportScale = (double) renderTargetGame.height / (double)vh;
        int mouseXUntranslated_InWorldSpace = int((double)mouseXRelativeToGameViewport_InWindowSpace * gameViewportScale);
        int mouseYUntranslated_InWorldSpace = int((double)mouseYRelativeToGameViewport_InWindowSpace * gameViewportScale);
        int mouseX_InWorldSpace = mouseXUntranslated_InWorldSpace + gameCamera0Position.x;
        int mouseY_InWorldSpace = mouseYUntranslated_InWorldSpace + gameCamera0Position.y;
        return { mouseX_InWorldSpace, mouseY_InWorldSpace };
    }
}

