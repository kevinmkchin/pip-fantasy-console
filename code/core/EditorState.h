#pragma once

#include "MesaCommon.h"
#include "MesaMath.h"

/*

# Let's think about entities / actors

## What is an entity?

Entities := something that executes logic
    - world/game manager
    - enemy spawner

Entities can have or not have sprites
Entities have a position. they can move around

Animations -> Sprites

## The Bridge Problem 
How do we bridge the gap between C++ side and MesaScript side? How is MS integrated?

The template (bad wording) is designed in the editor. The sprite and behaviour
of the entity is designed. Then, intances of that entity can be placed into the
game world via the world/level/space editor or via code during runtime. 
Each instance of this entity would have its own GCObject Map, own position, own
animation/sprite, own collider(?). These instances are tracked in a C++ list of
Objs to update and draw. To Kill an instance (e.g. destroy a bullet) would be to 
remove the instance from this array - the corresponding GCObject Map can still
exist should there still be references to it. 

set the "self" global variable before Update is run?
A template has code that defines the Init and Update functions.
I think this is the right approach. It decouples the Update function from
the object instance itself. I am very clearly and loudly saying that this
Update function should be able to operate on any entity instance of this type.
"self" can be any "Enemy_A" and the Enemy_A's Update function should work. It's
simply a function that operates on data. 

Now all of a sudden I'm going to start wanting to share functionality between
Enemy_A and Enemy_B. Then, Enemy_A's Update and Enemy_B's Update can call functions
that are accessible to both Update functions.

Currently, Parser::procedure_decl writes function identifiers into __MSRuntime.globalEnv,
but I probably want to set the active script before Parser.parse (and maybe Lexer) runs. Then, I can set
active the correct script and the correct function identifiers and variables for the entity I am updating.

The game starts:
    1. For every entity template:
        - Set "the script to be parsed" to the entity's behaviour script
        - Lexer.lex, Parser.parse, (and maybe the script is executed from start to finish)
    - at this point, there exists a MesaScript_ScriptObject for each script/EntiTemp.
The space begins:
    2. For every entity instance,
        - Create a new GCObject Map in the GCOBJECTS_DATABASE that represents this instance
    3. The script for space initialization is lexed, parsed, and executed?
    4. For every entity instance, 
        - Make active the MesaScript_ScriptObject for the EntiTemp type of this instance
        - Set self global variable to this instance's GCObject Map.
        - CallParameterlessFunction("Init")
    5. Space initialization script's PostGameObjectCreationInit should run? doesn't matter for now.
At every tick:
    7. Set the "time" and "key/input" global variables ("math" and "gui" should probably be set earlier since they don't need updating)
    6. For every entity instance,
        - Make active the MesaScript_ScriptObject for the EntiTemp type of this instance
        - Set self global variable to this instance's GCObject Map.
        - CallParameterlessFunction("Update")


Updating entity A
entity A needs to invoke entity B's methods or set entity B's map entries.
yeah, entity B is alive and its entries and methods are valid because they're
stored in GCOBJECT_DATABASE. 


## The Reference Problem
How do we let the code of one entity destroy another entity?

enemy = collision_data.other
enemy.destroy()
~enemy variable is going to be deleted at the end of this scope

EnemyUpdate ()
{
    ~there's a toChase variable

    if (toChase.dead) { toChase = FindNewEntityInstanceToChase() }
    MoveTowardsToChase();

    ~if you are going to reference other entities, then it should be
    ~a common practice to check if that entity has been destroyed or
    ~is dead. The "dead" field of 
}

bottom line is that: it's ok for the Map representation of an entity to remain 
in the global database as long as the C++ representation has been removed from 
the update/draw list.

collision_data.other -> E
enemy -> E
as well as other places in the code that reference E



*/

struct EditorState
{

};

