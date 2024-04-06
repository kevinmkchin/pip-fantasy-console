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

/*
Keeps track of all assets in project.
*/
struct ProjectData
{
    std::vector<SpriteData> spriteData;
    std::string codePage1;
};

void SerializeProjectData(ProjectData *gameData, const char *path);
void DeserializeProjectData(const char *path, ProjectData *gameData);
void ClearProjectData(ProjectData *gameData);


struct RuntimeTextureAtlas
{
    // need a mapping from sprite name/index + frame/animation to uv in TextureAtlas
    std::vector<Gfx::TextureHandle> textureAtlas;
};

void CompileRuntimeTextureAtlas(RuntimeTextureAtlas *atlas, ProjectData *gameData);
void TearDownRuntimeTextureAtlas(RuntimeTextureAtlas *atlas);
Gfx::TextureHandle MapIntoTextureAtlas(RuntimeTextureAtlas *atlas, u32 spriteId);
//void MapSpriteNameToSpriteId();


extern ProjectData projectData;
extern RuntimeTextureAtlas runtimeTextureAtlas;
