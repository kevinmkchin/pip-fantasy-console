#pragma once

#include "MesaCommon.h"
#include "GfxShader.h"
#include "GfxDataTypesAndUtility.h"
#include "EditorState.h"

namespace Gfx
{
    struct AutoLayoutHandle
    {
        int x;
        int y;
        int w;
        int h;
        bool xauto = true;
        bool yauto = true;
        bool wauto = true;
        bool hauto = true;

        std::vector<AutoLayoutHandle*> container;
    };

    enum class PixelPerfectRenderScale
    {
        OneHundredPercent = 1,
        TwoHundredPercent = 2,
        ThreeHundredPercent = 3,
        FourHundredPercent = 4
    };

    class CoreRenderer
    {
    public:
        bool Init();

        void Render();

        void UpdateBackBufferAndGameSize();

        void GetBackBufferSize(i32* widthOutput, i32* heightOutput);

        BasicFrameBuffer RenderTheFuckingWorldEditor(SpaceAsset *worldToView, EditorState *state, EditorWorldViewInfo worldViewInfo);

    public:
        ivec2 TransformWindowCoordinateToInternalCoordinate(ivec2 winCoord);

    private:

        void RenderGameLayer();

        void RenderGUILayer();

        // void RenderDebugUILayer();

        void FinalRenderToBackBuffer();

    private:

        void CreateFrameBuffers();

        void UpdateFrameBuffersSize();

        void UpdateScreenSizeQuad();

        void CreateMiscellaneous();

    public:
        PixelPerfectRenderScale screenScaling = PixelPerfectRenderScale::OneHundredPercent;

    private:
        i32 windowDrawableWidth = -1;
        i32 windowDrawableHeight = -1;
        i32 backBufferWidth = -1;
        i32 backBufferHeight = -1;

        // constant during the runtime of a game disregard of window resize
        i32 internalGameResolutionW = 800;
        i32 internalGameResolutionH = 600;

    public:
        BasicFrameBuffer gameLayer;
        BasicFrameBuffer guiLayer;
        //BasicFrameBuffer debugUILayer;

        BasicFrameBuffer worldEditorView;

    private:
        Shader finalPassShader;
        Shader spriteShader;

    private:
        Mesh screenSizeQuad;

    public:

        void UpdateAutoLayouting();

        AutoLayoutHandle *RegisterAutoLayout(int absX, int absY, int absW, int absH);

    };

    CoreRenderer* GetCoreRenderer();

}