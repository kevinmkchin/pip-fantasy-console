#pragma once

#include "MesaCommon.h"
#include "GfxShader.h"
#include "GfxDataTypesAndUtility.h"
#include "GameData.h"

namespace Gfx
{
    struct RenderQueueData
    {
        Gfx::TextureHandle sprite;
        vec2 position;
    };

    void QueueSpriteForRender(i64 spriteId, vec2 position);

    void SetGameLayerClearColor(vec4 color);

    void Primitive_DrawRect(float x, float y, float w, float h, vec4 color);

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

        //BasicFrameBuffer RenderTheFuckingWorldEditor(SpaceAsset *worldToView, EditorState *state, EditorWorldViewInfo worldViewInfo);

        void UpdateBackBufferAndGameSize();
        void GetBackBufferSize(i32 *widthOutput, i32 *heightOutput);
        void GetInternalRenderSize(i32 *widthOutput, i32 *heightOutput);

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
        // actual window size
        i32 windowDrawableWidth = -1;
        i32 windowDrawableHeight = -1;

        // window size minus whatever epsilon to make render pixel perfect
        i32 backBufferWidth = -1;
        i32 backBufferHeight = -1;

        // backbuffer size divided by screen/render scaling
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
        Shader primitiveShader;

    private:
        Mesh screenSizeQuad;

    };

    CoreRenderer* GetCoreRenderer();

}