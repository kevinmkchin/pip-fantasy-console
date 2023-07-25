#pragma once

#include <vector>

#include "Space.h"

/*
Keeps track of all assets in project.
Saves all project/assets/game data to disk.
Loads all project/assets/game data from disk.
*/

void CreateNewAsset_Entity(const char* name);
std::vector<EntityTemplate>* GetAll_Entity();

// void CreateNewAsset_Space();
// void CreateNewAsset_Sprite();

