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
    extern ivec2 gameCamera0Position;

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

        void RenderEditor();
        void RenderGame();

        //BasicFrameBuffer RenderTheFuckingWorldEditor(SpaceAsset *worldToView, EditorState *state, EditorWorldViewInfo worldViewInfo);

        void WindowSizeChanged();
        void ChangeEditorIntegerScaleAndInvokeWindowSizeChanged(PixelPerfectRenderScale scale);
        void GetBackBufferSize(i32 *widthOutput, i32 *heightOutput) const;

    public:
        ivec2 TransformWindowCoordinateToGameWorldSpace(ivec2 winCoord) const;
        ivec2 TransformWindowCoordinateToEditorGUICoordinateSpace(ivec2 winCoord) const;
        i32 GetEditorIntegerScale() const { return (i32)editorIntegerScale; };

    private:
        void RenderGameLayer();
        void RenderGUILayer();

        void FinalRenderToBackBuffer();
        void ConfigureViewportForFinalRender() const;
        void GetViewportValuesForFixedGameResolution(int *viewportX, int *viewportY, int *viewportW, int *viewportH) const;

    private:
        void CreateFrameBuffers();
        void CreateMiscellaneous();

    private:
        // actual window size
        i32 windowDrawableWidth = -1;
        i32 windowDrawableHeight = -1;
        // window size minus whatever epsilon to make render pixel perfect
        i32 backBufferWidth = -1;
        i32 backBufferHeight = -1;
        // integer scale to use in editor for nice drawing without distortions
        PixelPerfectRenderScale editorIntegerScale = PixelPerfectRenderScale::OneHundredPercent;

    private:
        void UpdateBackBufferAndGUILayerSizeToMatchWindowSizeIntegerScaled();

    public:
        BasicFrameBuffer renderTargetGame;
        BasicFrameBuffer renderTargetGUI;

        BasicFrameBuffer worldEditorView;

    private:
        Shader finalPassShader;
        Shader spriteShader;
        Shader primitiveShader;
    };

    // todo remove probably make an extern variable
    CoreRenderer* GetCoreRenderer();

}