#include "GfxDataTypesAndUtility.h"

#include "FileSystem.h"
#include "MesaUtility.h"
#include "PrintLog.h"

namespace Gfx
{
    void CreateBasicFrameBuffer(BasicFrameBuffer* buffer)
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

    void UpdateBasicFrameBufferSize(BasicFrameBuffer* buffer, i32 newWidth, i32 newHeight)
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

    void RenderMesh(const Mesh mesh, GLenum renderMode)
    {
        if (mesh.indicesCount == 0) // Early out if index_count == 0, nothing to draw
        {
            printf("WARNING: Attempting to Render a mesh with 0 index count!\n");
            return;
        }

        // Bind VAO, bind VBO, draw elements(indexed draw)
        glBindVertexArray(mesh.idVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.idIBO);
        glDrawElements(renderMode, mesh.indicesCount, GL_UNSIGNED_INT, nullptr);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void RebindBufferObjects(Mesh& mesh, float* vertices, u32* indices,
                             u32 verticesArrayCount, u32 indicesArrayCount, GLenum drawUsage)
    {
        if (mesh.idVBO == 0 || mesh.idIBO == 0)
        {
            return;
        }

        mesh.indicesCount = indicesArrayCount;
        glBindVertexArray(mesh.idVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.idVBO);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) 4 * verticesArrayCount, vertices, drawUsage);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.idIBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr) 4 * indicesArrayCount, indices, drawUsage);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    Mesh MeshCreate(Mesh& mesh, float* vertices, u32* indices,
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

    void MeshDelete(Mesh& mesh)
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


    TextureHandle CreateGPUTextureFromBitmap(unsigned char*    bitmap,
                                             u32               bitmap_width,
                                             u32               bitmap_height,
                                             GLenum            target_format,
                                             GLenum            source_format,
                                             GLenum            filter_mode)
    {
        TextureHandle texture;
        texture.width = bitmap_width;
        texture.height = bitmap_height;
        texture.format = source_format;

        glGenTextures(1, &texture.textureId);                              // generate texture and grab texture id
        glBindTexture(GL_TEXTURE_2D, texture.textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);       // wrapping
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_mode);  // filtering (e.g. GL_NEAREST)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_mode);
        glTexImage2D(
                GL_TEXTURE_2D,            // texture target type
                0,                        // level-of-detail number n = n-th mipmap reduction image
                target_format,            // format of data to store (target): num of color components
                bitmap_width,             // texture width
                bitmap_height,            // texture height
                0,                        // must be 0 (legacy)
                source_format,            // format of data being loaded (source)
                GL_UNSIGNED_BYTE,         // data type of the texture data
                bitmap);                  // data
        glBindTexture(GL_TEXTURE_2D, 0);

        return texture;
    }

    TextureHandle CreateGPUTextureFromDisk(const char* filePath)
    {
        TextureHandle texture;

        BitmapHandle textureBitmapHandle;
        ReadImage(textureBitmapHandle, filePath);
        if (textureBitmapHandle.memory == nullptr)
        {
            return texture;
        }
        bool bUseNearest = true; // TODO get from somewhere else
        texture = CreateGPUTextureFromBitmap((unsigned char*) textureBitmapHandle.memory,
                                             textureBitmapHandle.width,textureBitmapHandle.height,
                                             GL_RGBA,
                                             (textureBitmapHandle.bitDepth == 3 ? GL_RGB : GL_RGBA),
                                             (bUseNearest ? GL_NEAREST : GL_LINEAR_MIPMAP_LINEAR));
        GLHasErrors();
        FreeImage(textureBitmapHandle); // texture data has been copied to GPU memory, so we can free image from memory

        return texture;
    }

}
