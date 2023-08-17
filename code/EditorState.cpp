#include "EditorState.h"

std::vector<EntityAsset> g_EntityAssets;

void CreateBlankAsset_Entity(const char* name)
{
    g_EntityAssets.emplace_back();
    g_EntityAssets.back().name = name;
}

std::vector<EntityAsset>* GetAll_Entity()
{
    return &g_EntityAssets;
}

