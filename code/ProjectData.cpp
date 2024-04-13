#include "ProjectData.h"
#include "UTILITY.H"
#define KEVIN_BYTE_BUFFER_IMPLEMENTATION
#include "ByteBuffer.h"

ProjectData projectData;
RuntimeTextureAtlas runtimeTextureAtlas;


static ByteBuffer gameDataSerializeBuffer;

static void SerializeSpriteData(SpriteData *spriteData)
{
    void *pixelsSource = spriteData->frame.pixels;
    i32 pixelsW = spriteData->frame.w;
    i32 pixelsH = spriteData->frame.h;
    i32 pixelsSz = pixelsW * pixelsH * (i32)sizeof(SpriteColor);
    ByteBufferWrite(&gameDataSerializeBuffer, i32, pixelsW);
    ByteBufferWrite(&gameDataSerializeBuffer, i32, pixelsH);
    ByteBufferWriteBulk(&gameDataSerializeBuffer, pixelsSource, pixelsSz);
}

static void DeserializeSpriteData(SpriteData *spriteData)
{
    i32 pixelsW, pixelsH;
    ByteBufferRead(&gameDataSerializeBuffer, i32, &pixelsW);
    ByteBufferRead(&gameDataSerializeBuffer, i32, &pixelsH);
    i32 pixelsSz = pixelsW * pixelsH * (i32)sizeof(SpriteColor);
    spriteData->frame.w = pixelsW;
    spriteData->frame.h = pixelsH;
    spriteData->frame.pixels = (SpriteColor*)malloc(pixelsSz * sizeof(SpriteColor));
    void *pixelsTarget = spriteData->frame.pixels;
    ByteBufferReadBulk(&gameDataSerializeBuffer, pixelsTarget, pixelsSz);
}

static void ActuallySerializeEverything(ProjectData *gameData)
{
    i32 numSpriteData = (i32)gameData->spriteData.size();
    ByteBufferWrite(&gameDataSerializeBuffer, i32, numSpriteData);
    for (i32 i = 0; i < numSpriteData; ++i)
    {
        SerializeSpriteData(&gameData->spriteData.at(i));
    }


    i32 codeBufferSz = (i32)gameData->codePage1.size();
    ByteBufferWrite(&gameDataSerializeBuffer, i32, codeBufferSz);
    void *codeBufferSource = (void*)gameData->codePage1.c_str();
    ByteBufferWriteBulk(&gameDataSerializeBuffer, codeBufferSource, codeBufferSz);

}

static void ActuallyDeserializeEverything(ProjectData *gameData)
{
    i32 numSpriteData = 0;
    ByteBufferRead(&gameDataSerializeBuffer, i32, &numSpriteData);
    for (i32 i = 0; i < numSpriteData; ++i)
    {
        SpriteData spr;
        DeserializeSpriteData(&spr);
        gameData->spriteData.push_back(spr);
    }


    i32 codeBufferSz;
    ByteBufferRead(&gameDataSerializeBuffer, i32, &codeBufferSz);
    gameData->codePage1.resize(codeBufferSz);
    void *codeBufferTarget = (void*)gameData->codePage1.c_str();
    ByteBufferReadBulk(&gameDataSerializeBuffer, codeBufferTarget, codeBufferSz);

}

void SerializeProjectData(ProjectData *gameData, const char *path)
{
    PrintLog.Message("Saving game data.");

    gameDataSerializeBuffer = ByteBufferNew();

    ActuallySerializeEverything(gameData);

    if (ByteBufferWriteToFile(&gameDataSerializeBuffer, path) == 0)
        PrintLog.Error("Failed to save game data.");

    ByteBufferFree(&gameDataSerializeBuffer);
}

void DeserializeProjectData(const char *path, ProjectData *gameData)
{
    PrintLog.Message("Loading new game data.");

    gameDataSerializeBuffer = ByteBufferNew();
    if(ByteBufferReadFromFile(&gameDataSerializeBuffer, path))
    {
        ActuallyDeserializeEverything(gameData);
    }
    else
    {
        PrintLog.Error("Failed to load game data.");
    }
    ByteBufferFree(&gameDataSerializeBuffer);
}

void ClearProjectData(ProjectData *gameData)
{
    PrintLog.Message("Clearing loaded game data.");
    gameData->spriteData.clear();
    gameData->codePage1.clear();

    ResetSpriteEditorState();
}


void CompileRuntimeTextureAtlas(RuntimeTextureAtlas *atlas, ProjectData *gameData)
{
    atlas->textureAtlas.clear();
    for (int i = 0; i < gameData->spriteData.size(); ++i)
    {
        SpriteData sprd = gameData->spriteData.at(i);
        Gfx::TextureHandle texForThisSprite;
        SpriteImageToGPUTexture(&texForThisSprite, &sprd.frame);

        atlas->textureAtlas.push_back(texForThisSprite);
    }
}

void TearDownRuntimeTextureAtlas(RuntimeTextureAtlas *atlas)
{
    // TODO glDeleteTextures in GfxDataTypesAndUtility
}

Gfx::TextureHandle MapIntoTextureAtlas(RuntimeTextureAtlas *atlas, u32 spriteId)
{
    if (atlas->textureAtlas.size() < spriteId)
    {
        PrintLog.Error("spriteId doesn't exist.");
        return {};
    }
    return atlas->textureAtlas.at(spriteId);
}


