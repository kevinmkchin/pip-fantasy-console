#include "Game.h"
#include "InputSystem.h"
#include "Timer.h"
#include "EditorState.h"

#include "PipAPI.h"


bool TemporaryGameInit()
{
    EditorState *activeEditorState = EditorState::ActiveEditorState();
    std::string& gamecode = activeEditorState->codePage1;

    PipLangVM_InitVM();
    InitializePipAPI();
    InterpretResult rungamecodeResult = PipLangVM_RunGameCode(gamecode.c_str());
    if (rungamecodeResult != InterpretResult::OK)
    {
        return false;
    }

    return true;

//        MesaScript_Table *input = EmplaceMapInGlobalScope("input");
//        input->CreateNewMapEntry("left", left);
//        input->CreateNewMapEntry("right", right);
//        input->CreateNewMapEntry("up", up);
//        input->CreateNewMapEntry("down", down);
//        time->CreateNewMapEntry("timeSinceStart", timeSinceStart);
}

void TemporaryGameLoop()
{
    UpdatePipAPI();
    PipLangVM_RunGameFunction("tick");

    // time
    // ctrl/input

//    //CallFunction_Parameterless("pretick");
//    CallFunction_Parameterless("tick");
//    //CallFunction_Parameterless("posttick");
//    CallFunction_Parameterless("draw");

}

void TemporaryGameShutdown()
{
    TeardownPipAPI();
    PipLangVM_FreeVM();
}




/* Trash down here


// Space activeSpace;

// static std::unordered_map<int, MesaScript_Table> assetIdToEAScriptScopeMap;  


// Space* GetGameActiveSpace()
// {
//     return &activeSpace;
// }

// void ClearActiveSpace()
// {
//     for (int i = int(activeSpace.aliveUpdateAndDraw.size()) - 1, min = 0; i >= min; --i)
//     {
//         EntityInstance e = activeSpace.aliveUpdateAndDraw.at(i);

//         ReleaseReferenceGCObject(e.selfMapId);

//         activeSpace.aliveUpdateAndDraw.pop_back();
//     }
// }

// void CompileAllEntityCodeOnGameStart()
// {
//     assetIdToEAScriptScopeMap.clear();

//     EditorState *activeEditorState = EditorState::ActiveEditorState();
//     const std::vector<int> entityAssetIds = *activeEditorState->RetrieveAllEntityAssetIds();
//     for (const int eaid : entityAssetIds)
//     {
//         EntityAsset& ea = *activeEditorState->RetrieveEntityAssetById(eaid);

//         assetIdToEAScriptScopeMap.emplace(eaid, MesaScript_Table());

//         CompileMesaScriptCode(ea.code, &assetIdToEAScriptScopeMap.at(eaid));
//     }
// }


    // should be init space or start space, there should be separate func for game wide init
    /*
    The game starts:
        1. For every entity template:
            - Set "the script to be parsed" to the entity's behaviour script
            - Lexer.lex, Parser.parse, (and maybe the script is executed from start to finish)
        - at this point, there exists a MesaScript_ScriptEnvironment for each script/EntiTemp.
    The space begins:
        2. For every entity instance,
            - Create a new GCObject Map in the GCOBJECTS_DATABASE that represents this instance
        3. The script for space initialization is lexed, parsed, and executed?
        4. For every entity instance, 
            - Make active the MesaScript_ScriptEnvironment for the EntiTemp type of this instance
            - Set self global variable to this instance's GCObject Map.
            - CallParameterlessFunction("Init")
        5. Space initialization script's PostGameObjectCreationInit should run? doesn't matter for now.
    */

    // CompileAllEntityCodeOnGameStart();

    // ClearActiveSpace();







// bool Collide(EntityInstance a, EntityInstance b)
// {
//     MesaScript_Table *aMap = AccessMesaScriptTable(a.selfMapId);
//     MesaTValue xtv = aMap->AccessMapEntry("x");
//     MesaTValue ytv = aMap->AccessMapEntry("y");
//     float ax = float(xtv.type == MesaTValue::ValueType::Integer ? xtv.integerValue : xtv.realValue);
//     float ay = float(ytv.type == MesaTValue::ValueType::Integer ? ytv.integerValue : ytv.realValue);

//     MesaScript_Table *bMap = AccessMesaScriptTable(b.selfMapId);
//     xtv = bMap->AccessMapEntry("x");
//     ytv = bMap->AccessMapEntry("y");
//     float bx = float(xtv.type == MesaTValue::ValueType::Integer ? xtv.integerValue : xtv.realValue);
//     float by = float(ytv.type == MesaTValue::ValueType::Integer ? ytv.integerValue : ytv.realValue);

//     vec2 min1;
//     min1.x = ax;//collider1.collider_position.x + ((float) -collider1.collision_neg.x);
//     min1.y = ay - 32.f;//collider1.collider_position.y + ((float) -collider1.collision_neg.y);
//     vec2 max1;
//     max1.x = ax + 32.f;//collider1.collider_position.x + ((float) collider1.collision_pos.x);
//     max1.y = ay;//collider1.collider_position.y + ((float) collider1.collision_pos.y);

//     vec2 min2;
//     min2.x = bx;//collider2.collider_position.x + ((float) -collider2.collision_neg.x);
//     min2.y = by - 32.f;//collider2.collider_position.y + ((float) -collider2.collision_neg.y);
//     vec2 max2;
//     max2.x = bx + 32.f;//collider2.collider_position.x + ((float) collider2.collision_pos.x);
//     max2.y = by;//collider2.collider_position.y + ((float) collider2.collision_pos.y);

//     //CollisionInfo cinfo;

//     if (min1.x < max2.x && max1.x > min2.x && min1.y < max2.y && max1.y > min2.y) {

//         // Note(Kevin): doesn't work when one box is fully enveloped by the other box
//         // Calculate the x and y overlap between the two colliding entities
//         float dx = min(max1.x, max2.x) - max(min1.x, min2.x);
//         float dy = min(max1.y, max2.y) - max(min1.y, min2.y);

//         if(max1.x - min2.x == dx)
//         {
//             dx = -dx;
//         }
//         if(max1.y - min2.y == dy)
//         {
//             dy = -dy;
//         }

//         vec2 collision_overlap = { dx, dy };
//         return true;
//     }

//     return false;
// }





    // for (size_t i = 0, max = activeSpace.aliveUpdateAndDraw.size(); i < max; ++i)
    // {
    //     EntityInstance entity = activeSpace.aliveUpdateAndDraw.at(i);

    //     SetEnvironmentScope(entity.assetScriptScope);
    //     MesaTValue self;
    //     self.type = MesaTValue::ValueType::GCObject;
    //     self.GCReferenceObject = entity.selfMapId;
    //     CallFunction_OneParam("Update", self);
    //     ClearEnvironmentScope();
    // }


    // // collisions
    // for (size_t i = 0, max = activeSpace.aliveUpdateAndDraw.size(); i < max; ++i)
    // {
    //     EntityInstance a = activeSpace.aliveUpdateAndDraw.at(i);
    //     for (size_t j = i + 1; j < max; ++j)
    //     {
    //         EntityInstance b = activeSpace.aliveUpdateAndDraw.at(j);
    //         if (Collide(a, b))
    //         {
    //             printf("collided?");
    //             // a.OnCollide(b)
    //             // b.OnCollide(a)
    //             SetEnvironmentScope(a.assetScriptScope);
    //             MesaTValue self;
    //             self.type = MesaTValue::ValueType::GCObject;
    //             self.GCReferenceObject = a.selfMapId;
    //             CallFunction_OneParam("OnCollide", self);
    //             ClearEnvironmentScope();

    //             SetEnvironmentScope(b.assetScriptScope);
    //             self.GCReferenceObject = b.selfMapId;
    //             CallFunction_OneParam("OnCollide", self);
    //             ClearEnvironmentScope();
    //         }
    //     }
    // }



    // SpaceAsset *spaceToInitialize = activeEditorState->RetrieveSpaceAssetById(activeEditorState->activeSpaceId);
    // for (EntityAssetInstanceInSpace inst : spaceToInitialize->placedEntities)
    // {
    //     EntityInstance e;

    //     // other shit
    //     auto entityAsset = activeEditorState->RetrieveEntityAssetById(inst.entityAssetId);
    //     e.sprite = entityAsset->sprite;

    //     // script shit
    //     e.assetScriptScope = &assetIdToEAScriptScopeMap.at(inst.entityAssetId);
    //     // Note(Kevin): 2023-10-31 actually...do we need to have a table for every entity type and then another table 
    //     //              for every entity instance? i probably had a reason for doing it this way...maybe revisit? i don't think
    //     //              every entity instance of asset A should have their own copy of the AST, but their "update" key can just
    //     //              point to the single version AST of A.update? 

    //     e.selfMapId = RequestNewGCObject(MesaGCObject::GCObjectType::Table);

    //     MesaTValue tvalueInteger;
    //     tvalueInteger.type = MesaTValue::ValueType::Integer;
    //     tvalueInteger.integerValue = inst.spaceX;
    //     AccessMesaScriptTable(e.selfMapId)->CreateNewMapEntry("x", tvalueInteger);
    //     tvalueInteger.integerValue = inst.spaceY;
    //     AccessMesaScriptTable(e.selfMapId)->CreateNewMapEntry("y", tvalueInteger);

    //     activeSpace.aliveUpdateAndDraw.push_back(e);
    //     IncrementReferenceGCObject(e.selfMapId); // TODO(Kevin): decrement when destroy EntityInstance
    // }