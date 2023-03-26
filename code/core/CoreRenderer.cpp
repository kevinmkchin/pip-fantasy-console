#include "CoreRenderer.h"

#include <SDL.h>


#define GL3W_IMPLEMENTATION
#include <gl3w.h>
#include "ArcadiaOpenGL.h"
#include "ArcadiaUtility.h"
#include "CorePrintLog.h"
#include "ArcadiaIMGUI.h"

static SDL_Window* s_ActiveSDLWindow = nullptr;
static CoreRenderer* s_TheCoreRenderer = nullptr;

CoreRenderer* GetCoreRenderer()
{
    return s_TheCoreRenderer;
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

bool CoreRenderer::Init()
{
#ifdef MESA_USING_GL3W
    if (gl3w_init())
    {
        fprintf(stderr, "Failed to initialize OpenGL\n");
        return false;
    }
    PrintLog.Message("OpenGL initialized.");
#endif
    s_ActiveSDLWindow = SDL_GL_GetCurrentWindow();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // alpha blending func: a * (rgb) + (1 - a) * (rgb) = final color output
    glBlendEquation(GL_FUNC_ADD);

    CreateFrameBuffers();
    UpdateBackBufferSize();

    Shader::GLCreateShaderProgram(finalPassShader, __finalpass_shader_vs, __finalpass_shader_fs);
    //Shader::GLLoadShaderProgramFromFile(gameSceneShader, shader_path("scene.vert").c_str(), shader_path("scene.frag").c_str());

    CreateMiscellaneous();

    s_TheCoreRenderer = this;

    return true;
}

void CoreRenderer::Render()
{
    RenderGameLayer();
    RenderGUILayer();
    //RenderDebugUILayer();

    FinalRenderToBackBuffer();
}

void CoreRenderer::RenderGameLayer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, gameLayer.FBO);
    glViewport(0, 0, gameLayer.width, gameLayer.height);
    glClearColor(0.674f, 0.847f, 1.0f, 1.f); //_RGB(46, 88, 120)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    //gameSceneShader.UseShader();

//    gameSceneShader.GLBindMatrix4fv("projMatrix", 1, activeGameCamera->perspectiveMatrix.ptr());
//    gameSceneShader.GLBindMatrix4fv("viewMatrix", 1, activeGameCamera->viewMatrix.ptr());

    //GameState* gameState = &(Engine.game->state);

    //glUseProgram(0);
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
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    ARCGUI::Draw();
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
    finalPassShader.UseShader();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, backBufferWidth, backBufferHeight);
    glDepthRange(0, 10);
    glClearColor(RGB255TO1(0, 0, 0), 1.f);
    glClearDepth(1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    // Draw game frame
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gameLayer.colorTexId);
    screenSizeQuad.RenderMesh();

    // Draw GUI frame
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, guiLayer.colorTexId);
    screenSizeQuad.RenderMesh();

//    // Draw Debug UI frame
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, debugUILayer.colorTexId);
//    screenSizeQuad.RenderMesh();

    GLHasErrors();
}

void CoreRenderer::UpdateBackBufferSize()
{
    SDL_GL_GetDrawableSize(s_ActiveSDLWindow, &backBufferWidth, &backBufferHeight);
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

void CoreRenderer::SetGameResolution(i32 w, i32 h)
{
    internalGameResolutionW = w;
    internalGameResolutionH = h;
    UpdateFrameBuffersSize();
    UpdateScreenSizeQuad();
}

void CoreRenderer::UpdateFrameBuffersSize()
{
    UpdateBasicFrameBufferSize(&gameLayer, internalGameResolutionW, internalGameResolutionH);
    UpdateBasicFrameBufferSize(&guiLayer, internalGameResolutionW, internalGameResolutionH); // TODO(Kevin): gui layer should use different resolution to game
//    UpdateBasicFrameBufferSize(&debugUILayer, backBufferWidth, backBufferHeight);
}

void CoreRenderer::CreateBasicFrameBuffer(BasicFrameBuffer* buffer)
{
    buffer->FBO = 0;
    glGenFramebuffers(1, &buffer->FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, buffer->FBO);

    glGenTextures(1, &buffer->colorTexId);
    glBindTexture(GL_TEXTURE_2D, buffer->colorTexId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, buffer->width, buffer->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glGenRenderbuffers(1, &buffer->depthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, buffer->depthRBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, buffer->colorTexId, 0);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, buffer->width, buffer->height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, buffer->depthRBO);

    ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
}

void CoreRenderer::UpdateBasicFrameBufferSize(BasicFrameBuffer* buffer, i32 newWidth, i32 newHeight)
{
    buffer->width = newWidth;
    buffer->height = newHeight;
    glBindFramebuffer(GL_FRAMEBUFFER, buffer->FBO);
    glBindTexture(GL_TEXTURE_2D, buffer->colorTexId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, buffer->width, buffer->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindRenderbuffer(GL_RENDERBUFFER, buffer->depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, buffer->width, buffer->height);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        PrintLog.Error("Failed to change size of Internal FrameBuffer Object.");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

    Mesh::MeshCreate(screenSizeQuad, refQuadVertices, refQuadIndices, 16, 6, 2, 2, 0, GL_STATIC_DRAW);
}

void CoreRenderer::UpdateScreenSizeQuad()
{
    u32 refQuadIndices[6] = {
            0, 1, 3,
            0, 3, 2
    };
    float bw = (float)backBufferWidth;
    float bh = (float)backBufferHeight;
    float internal_ratio = (float)internalGameResolutionW / (float)internalGameResolutionH;
    float screen_ratio = (float)bw / (float)bh;
    float finalOutputQuadVertices[16] = {
            -1.f, -1.f, 0.f, 0.f,
            1.f, -1.f, 1.f, 0.f,
            -1.f, 1.f, 0.f, 1.f,
            1.f, 1.f, 1.f, 1.f
    };
    if(screen_ratio > internal_ratio)
    {
        float w = bh * internal_ratio;
        float f = (bw - w) / bw;
        finalOutputQuadVertices[0] = -1.f + f;
        finalOutputQuadVertices[4] = 1.f - f;
        finalOutputQuadVertices[8] = -1.f + f;
        finalOutputQuadVertices[12] = 1.f - f;
    }
    else
    {
        float h = bw / internal_ratio;
        float f = (bh - h) / bh;
        finalOutputQuadVertices[1] = -1.f + f;
        finalOutputQuadVertices[5] = -1.f + f;
        finalOutputQuadVertices[9] = 1.f - f;
        finalOutputQuadVertices[13] = 1.f - f;
    }
    screenSizeQuad.RebindBufferObjects(finalOutputQuadVertices, refQuadIndices, 16, 6);
}


void Mesh::RenderMesh(GLenum renderMode) const
{
    if (indicesCount == 0) // Early out if index_count == 0, nothing to draw
    {
        printf("WARNING: Attempting to Render a mesh with 0 index count!\n");
        return;
    }

    // Bind VAO, bind VBO, draw elements(indexed draw)
    glBindVertexArray(idVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idIBO);
    glDrawElements(renderMode, indicesCount, GL_UNSIGNED_INT, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Mesh::RebindBufferObjects(float* vertices, u32* indices,
                               u32 verticesArrayCount, u32 indicesArrayCount, GLenum drawUsage)
{
    if (idVBO == 0 || idIBO == 0)
    {
        return;
    }

    indicesCount = indicesArrayCount;
    glBindVertexArray(idVAO);
    glBindBuffer(GL_ARRAY_BUFFER, idVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) 4 * verticesArrayCount, vertices, drawUsage);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr) 4 * indicesArrayCount, indices, drawUsage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

Mesh Mesh::MeshCreate(Mesh& mesh, float* vertices, u32* indices,
                      u32 verticesArrayCount, u32 indicesArrayCount,
                      u8 positionAttribSize, u8 textureAttribSize, u8 normalAttribSize, GLenum drawUsage)
{
    u8 stride = 0;
    if (textureAttribSize)
    {
        stride += positionAttribSize + textureAttribSize;
        if (normalAttribSize)
        {
            stride += normalAttribSize;
        }
    }

    mesh.indicesCount = indicesArrayCount;

    glGenVertexArrays(1, &mesh.idVAO);
    glBindVertexArray(mesh.idVAO);
    glGenBuffers(1, &mesh.idVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.idVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) 4 /*bytes cuz float*/ * verticesArrayCount, vertices, drawUsage);
    glVertexAttribPointer(0, positionAttribSize, GL_FLOAT, GL_FALSE, sizeof(float) * stride, nullptr);
    glEnableVertexAttribArray(0);
    if (textureAttribSize > 0)
    {
        glVertexAttribPointer(1, textureAttribSize, GL_FLOAT, GL_FALSE, sizeof(float) * stride,
                              (void*)(sizeof(float) * positionAttribSize));
        glEnableVertexAttribArray(1);
        if (normalAttribSize > 0)
        {
            glVertexAttribPointer(2, normalAttribSize, GL_FLOAT, GL_FALSE, sizeof(float) * stride,
                                  (void*)(sizeof(float) * ((GLsizeiptr) positionAttribSize + textureAttribSize)));
            glEnableVertexAttribArray(2);
        }
    }

    glGenBuffers(1, &mesh.idIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.idIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr) 4 /*bytes cuz uint32*/ * indicesArrayCount, indices, drawUsage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0); // Unbind the VAO;

    return mesh;
}

void Mesh::MeshDelete(Mesh& mesh)
{
    if (mesh.idIBO != 0)
    {
        glDeleteBuffers(1, &mesh.idIBO);
        mesh.idIBO = 0;
    }
    if (mesh.idVBO != 0)
    {
        glDeleteBuffers(1, &mesh.idVBO);
        mesh.idVBO = 0;
    }
    if (mesh.idVAO != 0)
    {
        glDeleteVertexArrays(1, &mesh.idVAO);
        mesh.idVAO = 0;
    }

    mesh.indicesCount = 0;
}

