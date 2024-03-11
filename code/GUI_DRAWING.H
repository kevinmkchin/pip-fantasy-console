#pragma once

#include "MesaIMGUI.h"
#include "GfxDataTypesAndUtility.h"

namespace MesaGUI {

    struct UIDrawRequest;

    void GUIDraw_InitResources();
    void GUIDraw_DrawEverything();
    void GUIDraw_NewFrame();

    void GUIDraw_PushDrawCollection(UIRect windowMask, int depth = -1);
    void GUIDraw_PopDrawCollection();
    void AppendToCurrentDrawRequestsCollection(UIDrawRequest *drawRequest);

    struct UIDrawRequest
    {
        vec4 color;

        virtual void Draw() = 0;
    };

    struct RectDrawRequest : UIDrawRequest
    {
        UIRect rect;
        GLuint textureId = 0;

        void Draw() final;
    };

    struct RoundedCornerRectDrawRequest : UIDrawRequest
    {
        UIRect rect;
        int radius = 10;

        void Draw() final;
    };

    struct CorneredRectDrawRequest : UIDrawRequest
    {
        UIRect rect;
        int radius = 10;

        GLuint textureId = 0;
        float normalizedCornerSizeInUV = 0.3f; // [0,1] with 0.5 being half way across texture

        void Draw() final;
    };

    struct TextDrawRequest : UIDrawRequest
    {
        const char* text = "";
        int size = 8;
        int x = 0;
        int y = 0;
        TextAlignment alignment = TextAlignment::Left;
        Font font;

        UIRect rectMask = UIRect(0, 0, 9999, 9999);
        int rectMaskCornerRadius = -1;

        void Draw() final;
    };

    struct PipCodeDrawRequest : UIDrawRequest
    {
        const char* text = "";
        int size = 8;
        int x = 0;
        int y = 0;
        Font font;

        UIRect rectMask = UIRect(0, 0, 9999, 9999);
        int rectMaskCornerRadius = -1;

        void Draw() final;
    };

}
