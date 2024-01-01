#pragma once

/*
#include "MesaCommon.h"
#include "MesaMath.h"
#include "GfxDataTypesAndUtility.h"

#include <vector>

struct EntityInstance
{
    //vec2 position;

    struct MesaScript_Table* assetScriptScope = NULL;
    i64                      selfMapId = 0;

    Gfx::TextureHandle       sprite; // TODO(Kevin): do better...

    // collider 32 x 32 with origin top left

    // TODO(Kevin): probably create helpers to access values in the mesascript_table
};

struct Space
{
    std::vector<EntityInstance> aliveUpdateAndDraw;

};

/*
void StartSpace(Space* space)
{

}
*/
