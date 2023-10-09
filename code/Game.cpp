#include "Game.h"
#include "Space.h"
#include "MesaScript.h"
#include "InputSystem.h"
#include "Timer.h"
#include "EditorState.h"

Space activeSpace;

std::vector<MesaScript_Table> entityAssetScriptScopes; // TODO(Kevin): remplace le par stack allocator

Space* GetGameActiveSpace()
{
    return &activeSpace;
}

void ClearActiveSpace()
{
    for (int i = int(activeSpace.aliveUpdateAndDraw.size()) - 1, min = 0; i >= min; --i)
    {
        EntityInstance e = activeSpace.aliveUpdateAndDraw.at(i);

        ReleaseReferenceGCObject(e.selfMapId);

        activeSpace.aliveUpdateAndDraw.pop_back();
    }
}

void TemporaryGameInit() // should be init space or start space, there should be separate func for game wide init
{
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

    ClearActiveSpace();

    EditorState *activeEditorState = EditorState::ActiveEditorState();

    const std::vector<int> entityAssetIds = *activeEditorState->RetrieveAllEntityAssetIds();
    EntityAsset& et0 = *activeEditorState->RetrieveEntityAssetById(entityAssetIds.at(0));
    EntityAsset& et1 = *activeEditorState->RetrieveEntityAssetById(entityAssetIds.at(1));
    EntityAsset& et2 = *activeEditorState->RetrieveEntityAssetById(entityAssetIds.at(2));
    
    entityAssetScriptScopes.clear();
    entityAssetScriptScopes.emplace_back();
    entityAssetScriptScopes.emplace_back();
    entityAssetScriptScopes.emplace_back();
    CompileMesaScriptCode(et0.code, &entityAssetScriptScopes.at(0));
    CompileMesaScriptCode(et1.code, &entityAssetScriptScopes.at(1));
    CompileMesaScriptCode(et2.code, &entityAssetScriptScopes.at(2));


    SpaceAsset *spaceToInitialize = activeEditorState->RetrieveSpaceAssetById(activeEditorState->activeSpaceId);

    for (EntityAssetInstanceInSpace inst : spaceToInitialize->placedEntities)
    {
        EntityInstance e;

        e.assetScriptScope = &entityAssetScriptScopes.at(1);

        e.selfMapId = RequestNewGCObject(MesaGCObject::GCObjectType::Table);

        TValue tvalueInteger;
        tvalueInteger.type = TValue::ValueType::Integer;
        tvalueInteger.integerValue = inst.spaceX;
        AccessMesaScriptTable(e.selfMapId)->CreateNewMapEntry("x", tvalueInteger);
        tvalueInteger.integerValue = inst.spaceY;
        AccessMesaScriptTable(e.selfMapId)->CreateNewMapEntry("y", tvalueInteger);

        activeSpace.aliveUpdateAndDraw.push_back(e);
        IncrementReferenceGCObject(e.selfMapId); // TODO(Kevin): decrement when destroy EntityInstance
    }

    static bool doOnce = false;
    if (!doOnce)
    {
        doOnce = true; // TODO(Kevin): do something more robust like what should actually happen to these singletons on restart?

        MesaScript_Table *input = EmplaceMapInGlobalScope("input");
        TValue left;
        left.type = TValue::ValueType::Boolean;
        left.boolValue = false;
        TValue right;
        right.type = TValue::ValueType::Boolean;
        right.boolValue = false;
        TValue up;
        up.type = TValue::ValueType::Boolean;
        up.boolValue = false;
        TValue down;
        down.type = TValue::ValueType::Boolean;
        down.boolValue = false;
        input->CreateNewMapEntry("left", left);
        input->CreateNewMapEntry("right", right);
        input->CreateNewMapEntry("up", up);
        input->CreateNewMapEntry("down", down);

        MesaScript_Table *time = EmplaceMapInGlobalScope("time");
        TValue deltaTime;
        deltaTime.type = TValue::ValueType::Real;
        deltaTime.realValue = Time.deltaTime;
        time->CreateNewMapEntry("dt", deltaTime);
    }
}

void TemporaryGameLoop()
{
    /*

    7. Set the "time" and "key/input" global variables ("math" and "gui" should probably be set earlier since they don't need updating)
    6. For every entity instance,
        - Make active the MesaScript_ScriptEnvironment for the EntiTemp type of this instance
        - Set self global variable to this instance's GCObject Map.
        - CallParameterlessFunction("Update")

    */

    MesaScript_Table *input = AccessMapInGlobalScope("input");
    input->table.at("left").boolValue = Input.currentKeyState[SDL_SCANCODE_LEFT];
    input->table.at("right").boolValue = Input.currentKeyState[SDL_SCANCODE_RIGHT];
    input->table.at("up").boolValue = Input.currentKeyState[SDL_SCANCODE_UP];
    input->table.at("down").boolValue = Input.currentKeyState[SDL_SCANCODE_DOWN];
    MesaScript_Table *time = AccessMapInGlobalScope("time");
    time->table.at("dt").realValue = Time.deltaTime;

    for (size_t i = 0, max = activeSpace.aliveUpdateAndDraw.size(); i < max; ++i)
    {
        EntityInstance entity = activeSpace.aliveUpdateAndDraw.at(i);

        SetEnvironmentScope(entity.assetScriptScope);
        TValue self;
        self.type = TValue::ValueType::GCObject;
        self.GCReferenceObject = entity.selfMapId;
        CallFunction_OneParam("Update", self);
        ClearEnvironmentScope();
    }


}






