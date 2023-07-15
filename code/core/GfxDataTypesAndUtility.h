#pragma once

#include "MesaCommon.h"
#include "GfxShader.h"

namespace Gfx
{

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
    };
    
    struct TextureHandle
    {
        GLuint  textureId   = 0;        // ID for the texture in GPU memory
        i32     width       = 0;        // Width of the texture
        i32     height      = 0;        // Height of the texture
        GLenum  format      = GL_NONE;  // format / bitdepth of texture (GL_RGB would be 3 byte bit depth)
    };

    void CreateBasicFrameBuffer(BasicFrameBuffer* buffer);

    void UpdateBasicFrameBufferSize(BasicFrameBuffer* buffer, i32 newWidth, i32 newHeight);

    /** Binds VAO and draws elements. Bind a shader program and texture
        before calling RenderMesh */
    void RenderMesh(Mesh mesh, GLenum renderMode = GL_TRIANGLES);
    
    /** Overwrite existing buffer data */
    void RebindBufferObjects(Mesh& mesh,
                             float* vertices,
                             u32* indices,
                             u32 verticesArrayCount,
                             u32 indicesArrayCount,
                             GLenum drawUsage = GL_DYNAMIC_DRAW);
    
    /** Create a Mesh with the given vertices and indices.
        vertex_attrib_size: vertex coords size (e.g. 3 if x y z)
        texture_attrib_size: texture coords size (e.g. 2 if u v)
        draw_usage: affects optimization; GL_STATIC_DRAW buffer data
        only set once, GL_DYNAMIC_DRAW if buffer modified repeatedly */
    Mesh MeshCreate(Mesh& mesh,
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
    void MeshDelete(Mesh& mesh);
    
    TextureHandle CreateGPUTextureFromBitmap(unsigned char*    bitmap,
                                             u32               bitmap_width,
                                             u32               bitmap_height,
                                             GLenum            target_format,
                                             GLenum            source_format,
                                             GLenum            filter_mode);
    
    TextureHandle CreateGPUTextureFromDisk(const char* filePath);

}