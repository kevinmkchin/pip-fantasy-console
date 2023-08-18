#include "EditorState.h"

static EditorState globalEditorState;

EditorState *EditorState::ActiveEditorState()
{
    return &globalEditorState;
}

int EditorState::FreshAssetID()
{
    return assetIdTicker++;
}

int EditorState::CreateNewEntityAsset(const char *name)
{
    EntityAsset createdEntity;
    createdEntity.name = name;
    int assetId = FreshAssetID();
    projectEntityAssets.insert(std::make_pair(assetId, createdEntity));
    projectEntityAssetIds.push_back(assetId);
    return assetId;
}

void EditorState::DeleteEntityAsset(int assetId)
{
    projectEntityAssets.erase(projectEntityAssets.find(assetId));
    for (auto iter = projectEntityAssetIds.begin(); iter != projectEntityAssetIds.end(); ++iter)
    {
        projectEntityAssetIds.erase(iter);
    }
}

EntityAsset *EditorState::RetrieveEntityAssetById(int assetId)
{
    return &projectEntityAssets.at(assetId);
}

const std::vector<int> *EditorState::RetrieveAllEntityAssetIds()
{
    return &projectEntityAssetIds;
}

