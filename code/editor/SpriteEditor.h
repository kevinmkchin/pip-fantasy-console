#pragma once

#include "../MesaCommon.h"
#include "../GfxDataTypesAndUtility.h"

struct spredit_Color
{
    u8 r = 0;
    u8 g = 0;
    u8 b = 0;
    u8 a = 0;
};

struct spredit_Frame
{
    spredit_Color *pixels;
    i32 w;
    i32 h;

    Gfx::TextureHandle gputex;
};

struct SpriteEditorState
{
    spredit_Frame frame;
};

void DoSpriteEditorGUI();
