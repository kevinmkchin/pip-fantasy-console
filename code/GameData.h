#pragma once

#include <vector>
#include <unordered_map>

#include "MesaCommon.h"
#include "MesaMath.h"
#include "GfxDataTypesAndUtility.h"

/*
Keeps track of all assets in project.
Saves all project/assets/game data to disk.
Loads all project/assets/game data from disk.
*/

//struct EditorWorldViewInfo
//{
//    int zoomLevel = 1; // 1, 2, 3, 4
//    ivec2 pan;
//    ivec2 dimAfterZoom;
//    ivec2 dimInUIScale;
//    // dim = actual dim / zoom level
//};

// struct Sprite
// {
//     Gfx::TextureHandle sprite;
// }

struct GameData
{
    std::vector<Gfx::TextureHandle> sprites;
    std::string codePage1;
};

void SerializeGameData(GameData *gameData, const char *path);
void DeserializeGameData(const char *path, GameData *gameData);
void ClearGameData(GameData *gameData);

extern GameData gamedata;
