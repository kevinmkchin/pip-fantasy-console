#include "Game.h"
#include "Space.h"
#include "script/MesaScript.h"


Space activeSpace;
MesaScript_ScriptEnvironment scriptEnvironments[3];

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

    et0.code = "fn Update() { print('et0 update') }";
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

    activeSpace.aliveUpdateAndDraw.push_back(e0);
    activeSpace.aliveUpdateAndDraw.push_back(e1);
    activeSpace.aliveUpdateAndDraw.push_back(e2);
    activeSpace.aliveUpdateAndDraw.push_back(e3);
    activeSpace.aliveUpdateAndDraw.push_back(e4);

    //RequestNewGCObject(MesaGCObject::GCObjectType::Table);
    //RequestNewGCObject(MesaGCObject::GCObjectType::Table);
    //RequestNewGCObject(MesaGCObject::GCObjectType::Table);
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

    for (size_t i = 0, max = activeSpace.aliveUpdateAndDraw.size(); i < max; ++i)
    {
        EntityInstance entity = activeSpace.aliveUpdateAndDraw.at(i);
        SetActiveScriptEnvironment(entity.behaviourScriptEnv);
        CallParameterlessFunctionInActiveScriptEnvironment("Update");
    }


}






