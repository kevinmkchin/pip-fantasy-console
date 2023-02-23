#pragma once

#include "CoreCommon.h"
#include "CoreShader.h"

struct BasicFrameBuffer
{
    u32 FBO;
    u32 colorTexId;
    u32 depthRBO;
    i32 width;
    i32 height;
};

struct Mesh
{
    /** Stores mesh { VAO, VBO, IBO } info. Handle for VAO on GPU memory
        Holds the ID for the VAO, VBO, IBO in the GPU memory */
    u32  idVAO = 0;
    u32  idVBO = 0;
    u32  idIBO = 0;
    u32  indicesCount = 0;

    /** Binds VAO and draws elements. Bind a shader program and texture
        before calling RenderMesh */
    void RenderMesh(GLenum renderMode = GL_TRIANGLES) const;

    /** Overwrite existing buffer data */
    void RebindBufferObjects(float* vertices,
                             u32* indices,
                             u32 verticesArrayCount,
                             u32 indicesArrayCount,
                             GLenum drawUsage = GL_DYNAMIC_DRAW);

    /** Create a Mesh with the given vertices and indices.
        vertex_attrib_size: vertex coords size (e.g. 3 if x y z)
        texture_attrib_size: texture coords size (e.g. 2 if u v)
        draw_usage: affects optimization; GL_STATIC_DRAW buffer data
        only set once, GL_DYNAMIC_DRAW if buffer modified repeatedly */
    static Mesh MeshCreate(Mesh& mesh,
                           float* vertices,
                           u32* indices,
                           u32 verticesArrayCount,
                           u32 indicesArrayCount,
                           u8 positionAttribSize = 3,  // x y z
                           u8 textureAttribSize = 2,   // u v
                           u8 normalAttribSize = 3,    // x y z
                           GLenum drawUsage = GL_STATIC_DRAW);

    /** Clearing GPU memory: glDeleteBuffers and glDeleteVertexArrays deletes the buffer
        object and vertex array object off the GPU memory. */
    static void MeshDelete(Mesh& mesh);
};

class CoreRenderer
{
public:
    bool Init();

    void Render();

    void UpdateBackBufferSize();

    void GetBackBufferSize(i32* widthOutput, i32* heightOutput);

    void SetGameResolution(i32 w, i32 h);

private:

    void RenderGameLayer();

    void RenderGUILayer();

    // void RenderDebugUILayer();

    void FinalRenderToBackBuffer();

private:

    void CreateFrameBuffers();

    void UpdateFrameBuffersSize();

    void CreateBasicFrameBuffer(BasicFrameBuffer* buffer);

    void UpdateBasicFrameBufferSize(BasicFrameBuffer* buffer, i32 newWidth, i32 newHeight);

    void UpdateScreenSizeQuad();

    void CreateMiscellaneous();

private:
    i32 backBufferWidth = -1;
    i32 backBufferHeight = -1;

    // constant during the runtime of a game disregard of window resize
    i32 internalGameResolutionW = 800;
    i32 internalGameResolutionH= 600;

public:
    BasicFrameBuffer gameLayer;
    BasicFrameBuffer guiLayer;
    //BasicFrameBuffer debugUILayer;

private:
    Shader finalPassShader;
    Shader gameSceneShader;

private:
    Mesh screenSizeQuad;

};

CoreRenderer* GetCoreRenderer();
