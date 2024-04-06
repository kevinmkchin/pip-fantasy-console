#pragma once

#include "../MesaCommon.h"
#include "../GfxDataTypesAndUtility.h"

struct SpriteColor
{
    u8 r = 0;
    u8 g = 0;
    u8 b = 0;
    u8 a = 0;
};

struct SpriteImage
{
    SpriteColor *pixels;
    i32 w;
    i32 h;
};

struct SpriteData
{
    std::string name;
    //std::vector<SpriteFrame> frames;
    SpriteImage frame;
    // layers
    // animations
};

void AllocSpriteImage(SpriteImage *image, i32 w, i32 h, bool white);
void SpriteImageToGPUTexture(Gfx::TextureHandle *texture, SpriteImage *image);

struct SpriteEditorState
{
    SpriteImage frame;

    Gfx::TextureHandle gputex;

    std::vector<SpriteColor> palette;
};

void ResetSpriteEditorState();
void DoSpriteEditorGUI();
