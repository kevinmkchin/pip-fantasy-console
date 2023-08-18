#pragma once

#include "MesaCommon.h"
#include "MesaMath.h"

#include <vector>

struct EntityInstance
{
    //vec2 position;

    struct MesaScript_Table* assetScriptScope = NULL;
    i64                      selfMapId = 0;
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
