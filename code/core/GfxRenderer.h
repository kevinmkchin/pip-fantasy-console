#pragma once

#include "MesaCommon.h"
#include "GfxShader.h"
#include "GfxDataTypesAndUtility.h"

class GfxRenderer
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
    i32 internalGameResolutionH = 600;

public:
    BasicFrameBuffer gameLayer;
    BasicFrameBuffer guiLayer;
    //BasicFrameBuffer debugUILayer;

private:
    Shader finalPassShader;
    Shader spriteShader;

private:
    Mesh screenSizeQuad;

};

GfxRenderer* GetGfxRenderer();
