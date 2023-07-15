#include "Game.h"
#include "Space.h"
#include "script/MesaScript.h"
#include "../core/CoreInput.h"


Space activeSpace;
MesaScript_ScriptEnvironment scriptEnvironments[3];

Space* GetGameActiveSpace()
{
    return &activeSpace;
}

void TemporaryGameInit()
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

    EntityTemplate et0;
    EntityTemplate et1;
    EntityTemplate et2;

    et0.code = "fn Update(self) { \n"
        "   if (input['left']) {\n"
        "       self['x'] = self['x'] - 1\n" 
        "   }\n" 
        "   if (input['right']) {\n"
        "       self['x'] = self['x'] + 1\n" 
        "   }\n" 
        "   if (input['up']) {\n"
        "       self['y'] = self['y'] + 1\n" 
        "   }\n" 
        "   if (input['down']) {\n"
        "       self['y'] = self['y'] - 1\n" 
        "   }\n" 
        "}";
    et1.code = "fn Update() { print('et1 update') }";
    et2.code = "fn Update() { print('et2 update') }";

    scriptEnvironments[0] = CompileEntityBehaviourAsNewScriptEnvironment(et0.code);
    scriptEnvironments[1] = CompileEntityBehaviourAsNewScriptEnvironment(et1.code);
    scriptEnvironments[2] = CompileEntityBehaviourAsNewScriptEnvironment(et2.code);

    EntityInstance e0;
    EntityInstance e1;
    EntityInstance e2;
    EntityInstance e3;
    EntityInstance e4;

    e0.behaviourScriptEnv = &scriptEnvironments[0];
    e1.behaviourScriptEnv = &scriptEnvironments[1];
    e2.behaviourScriptEnv = &scriptEnvironments[2];
    e3.behaviourScriptEnv = &scriptEnvironments[0];
    e4.behaviourScriptEnv = &scriptEnvironments[0];
    e0.mesaGCObjMapRepresentationId = RequestNewGCObject(MesaGCObject::GCObjectType::Table);
    e1.mesaGCObjMapRepresentationId = RequestNewGCObject(MesaGCObject::GCObjectType::Table);
    e2.mesaGCObjMapRepresentationId = RequestNewGCObject(MesaGCObject::GCObjectType::Table);
    e3.mesaGCObjMapRepresentationId = RequestNewGCObject(MesaGCObject::GCObjectType::Table);
    e4.mesaGCObjMapRepresentationId = RequestNewGCObject(MesaGCObject::GCObjectType::Table);
    IncrementReferenceGCObject(e0.mesaGCObjMapRepresentationId); // TODO(Kevin): decrement when destroy EntityInstance
    IncrementReferenceGCObject(e1.mesaGCObjMapRepresentationId);
    IncrementReferenceGCObject(e2.mesaGCObjMapRepresentationId);
    IncrementReferenceGCObject(e3.mesaGCObjMapRepresentationId);
    IncrementReferenceGCObject(e4.mesaGCObjMapRepresentationId);

    TValue tvalueDefaultInteger;
    tvalueDefaultInteger.type = TValue::ValueType::Integer;
    tvalueDefaultInteger.integerValue = 0;
    AccessMesaScriptTable(e0.mesaGCObjMapRepresentationId)->CreateNewMapEntry("x", tvalueDefaultInteger);
    AccessMesaScriptTable(e0.mesaGCObjMapRepresentationId)->CreateNewMapEntry("y", tvalueDefaultInteger);

    activeSpace.aliveUpdateAndDraw.push_back(e0);
    //activeSpace.aliveUpdateAndDraw.push_back(e1);
    //activeSpace.aliveUpdateAndDraw.push_back(e2);
    //activeSpace.aliveUpdateAndDraw.push_back(e3);
    //activeSpace.aliveUpdateAndDraw.push_back(e4);

    MesaScript_Table* input = EmplaceMapInGlobalScope("input");
    MesaScript_Table* time = EmplaceMapInGlobalScope("time");
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

    MesaScript_Table* input = AccessMapInGlobalScope("input");
    MesaScript_Table* time = AccessMapInGlobalScope("time");
    input->table.at("left").boolValue = Input.currentKeyState[SDL_SCANCODE_LEFT];
    input->table.at("right").boolValue = Input.currentKeyState[SDL_SCANCODE_RIGHT];
    input->table.at("up").boolValue = Input.currentKeyState[SDL_SCANCODE_UP];
    input->table.at("down").boolValue = Input.currentKeyState[SDL_SCANCODE_DOWN];

    for (size_t i = 0, max = activeSpace.aliveUpdateAndDraw.size(); i < max; ++i)
    {
        EntityInstance entity = activeSpace.aliveUpdateAndDraw.at(i);
        SetActiveScriptEnvironment(entity.behaviourScriptEnv);

        TValue self;
        self.type = TValue::ValueType::GCObject;
        self.GCReferenceObject = entity.mesaGCObjMapRepresentationId;

        CallFunctionInActiveScriptEnvironmentWithOneParam("Update", self);
    }


}






