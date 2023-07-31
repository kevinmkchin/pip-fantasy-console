#pragma once

#include <vector>

#include "Space.h"

/*
Keeps track of all assets in project.
Saves all project/assets/game data to disk.
Loads all project/assets/game data from disk.
*/

void CreateBlankAsset_Entity(const char* name);
std::vector<EntityAsset>* GetAll_Entity();

// void CreateNewAsset_Space();
// void CreateNewAsset_Sprite();

