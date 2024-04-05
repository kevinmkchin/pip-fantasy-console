#pragma once

#include <vector>
#include <unordered_map>

#include "MesaCommon.h"
#include "MesaMath.h"
#include "GfxDataTypesAndUtility.h"
#include "Editor/SpriteEditor.h"

//struct EditorWorldViewInfo
//{
//    int zoomLevel = 1; // 1, 2, 3, 4
//    ivec2 pan;
//    ivec2 dimAfterZoom;
//    ivec2 dimInUIScale;
//    // dim = actual dim / zoom level
//};

//struct SpriteDataLayer
//{
//
//};

//struct SpriteDataFrame
//{
//    spredit_Frame frame;
//};

struct SpriteData
{
    std::string name;
    //std::vector<SpriteDataFrame> frames;
    spredit_Frame frame;

    // layers
    // animations
};

/*
Keeps track of all assets in project.
*/
struct GameData
{
    // in-editor
    std::vector<SpriteData> spriteData;
    std::string codePage1;
};

void SerializeGameData(GameData *gameData, const char *path);
void DeserializeGameData(const char *path, GameData *gameData);
void ClearGameData(GameData *gameData);

struct RuntimeData
{
    // this shit should be transient just compile it from Game/Project data when we start play
    std::vector<Gfx::TextureHandle> sprites;
};

extern GameData gamedata;
extern RuntimeData runtimedata;
