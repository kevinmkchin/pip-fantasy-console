#pragma once

#include "../core/CoreCommon.h"
#include "../core/gmath.h"

#include <vector>

struct EntityTemplate
{
    // what sprite / animation to start with, what animations are available?
    // what is the code / behaviour of all instances of this entity?

    std::string code = "";
};

struct EntityInstance
{
    //vec2 position;

    struct MesaScript_ScriptEnvironment* behaviourScriptEnv = NULL;
    u64 mesaGCObjMapRepresentationId = 0;
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
