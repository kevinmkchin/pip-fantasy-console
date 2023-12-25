Optimization steps

first step: don't hash everytime an ID is being used - hash the ID upon parsing and save it along with the ID

second step: don't use modulus

keep slow ASSERTS only for debug build

IncrementRefGCObjs does transiency check against entire stack. Transiency checks should be limited to current scope I think.

Remove lots of redundant safety checks. Just let accesses be unsafe and handle exception if thrown. 


# Interpreter structure

InterpretExpression
    switch node type
        SIMPLYTVALUE
        BINOP
        RELOP
        LOGICALNOT
        VARIABLE
        ACCESS_LIST_OR_MAP_ELEMENT
        CREATETABLE
        CREATELIST
        NUMBER
        STRING
        BOOLEAN
        PROCEDURECALL
    handle transiency

InterpretStatement
    switch node type
        PROCEDURECALL
        ASSIGN
        ASSIGN_LIST_OR_MAP_ELEMENT
        RETURN
        BRANCH

InterpretStatementList
    loop interpretstatements

InterpretProcedureCall
    look at all scopes to find procedure name
    get ProcedureDefinition from NiceArray via PID trivial
    new function scope gets added to activeEnv
    loop InterpretExpression each function argument 
    InterpretStatementList the function body
    pop function scope

## Auxiliary mechanisms

MesaScript_ScriptEnvironment.KeyExists
MesaScript_ScriptEnvironment.KeyExistsInCurrentScope
MesaScript_ScriptEnvironment.AccessAtKey (heavy as fuck, checks every scope even if we're 1000 scopes deep)
MesaScript_ScriptEnvironment.ReplaceAtKey (heavy as fuck, checks every scope even if we're 1000 scopes deep)
MesaScript_ScriptEnvironment.EraseTransientObject (heavy? checks every scope)

MesaScript_Table wrapper for `std::unordered_map<std::string, TValue>` 
MesaScript_Table.Contains
MesaScript_Table.AccessMapEntry 
MesaScript_Table.CreateNewMapEntry
MesaScript_Table.ReplaceMapEntryAtKey
MesaScript_Table.DecrementReferenceCountOfEveryMapEntry


GCOBJECT_DATABASE.AccessMesaScriptTable



MesaScript_ScriptEnvironment.TransientObjectExists (trivial ?)
MesaScript_ScriptEnvironment.InsertTransientObject (trivial ?)
MesaScript_ScriptEnvironment.ClearTransientsInTopLevelScope (trivial ?)


## Measure

OF TOTAL RUNTIME:
    % of time spent in IE
    % of time spent in IS
    % of time spend in IPC
    % of time in other

OF TOTAL RUNTIME:
    % of time spent 


## Notes
There's a lot of if else if branches in BINOP RELOP handling



