#pragma once

#include <vector>
#include <unordered_map>

#include "MesaCommon.h"
#include "MesaMath.h"

#include "Space.h"
#include "GfxDataTypesAndUtility.h"

/*
Keeps track of all assets in project.
Saves all project/assets/game data to disk.
Loads all project/assets/game data from disk.
*/

struct EditorWorldViewInfo
{
    int zoomLevel = 1; // 1, 2, 3, 4
    ivec2 pan;
    ivec2 dimAfterZoom;
    ivec2 dimInUIScale;

    // dim = actual dim / zoom level
};

struct EntityAsset
{
    // what sprite / animation to start with, what animations are available?
    // what is the code / behaviour of all instances of this entity?

    std::string name = "";
    std::string code = "";

    Gfx::TextureHandle sprite;
    ivec2 spriteOriginOffset;
};

struct EntityAssetInstanceInSpace
{
    int spaceX;
    int spaceY;
    int entityAssetId;
};

struct SpaceAsset
{
    // list of entities to instantiate in this space

    std::vector<EntityAssetInstanceInSpace> placedEntities;


};

struct EditorState
{

public:
    static EditorState *ActiveEditorState();

    int CreateNewEntityAsset(const char *name);
    void DeleteEntityAsset(int assetId);
    EntityAsset *RetrieveEntityAssetById(int assetId);
    const std::vector<int> *RetrieveAllEntityAssetIds();

    int CreateNewSpaceAsset(const char *name);
    SpaceAsset *RetrieveSpaceAssetById(int assetId);

private:
    int FreshAssetID();

public:
    int activeSpaceId = -1;

private:
    std::vector<int> projectEntityAssetIds;
    std::unordered_map<int, EntityAsset> projectEntityAssets;

    std::vector<int> projectSpaceAssetIds;
    std::unordered_map<int, SpaceAsset> projectSpaceAssets;

    int assetIdTicker = 100;
};


// void CreateNewAsset_Space();
// void CreateNewAsset_Sprite();

/*

The template (bad wording) is designed in the editor. The sprite and behaviour
of the entity is designed. Then, instances of that entity can be placed into the
game world via the world/level/space editor or via code during runtime. 
Each instance of this entity would have its own GCObject Map, own position, own
animation/sprite, own collider(?). These instances are tracked in a C++ list of
Objs to update and draw. To Kill an instance (e.g. destroy a bullet) would be to 
remove the instance from this array - the corresponding GCObject Map can still
exist should there still be references to it. 


What is an entity?
Entities := something that executes logic
    - world/game manager
    - enemy spawner
Entities can have or not have sprites
Entities have a position. they can move around
Animations -> Sprites

How do entities refer to themselves?
Should functions be "object-oriented" or be agnostic?
Currently, the entity to operate on must be passed into the function.
This decouples the Update function from the object instance itself. I am very 
clearly and loudly saying that this Update function should be able to operate 
on any entity instance of this type. "self" can be any "Enemy_A" and the Enemy_A's
Update function should work. 
Now all of a sudden I'm going to start wanting to share functionality between
Enemy_A and Enemy_B. Then, Enemy_A's Update and Enemy_B's Update can call functions
that are accessible to both Update functions.
2023-08-17: I thought this was the right approach, but maybe it's not the best in 
terms of user experience. Reevaluate later.

A template has code that defines the Init and Update functions.



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
    ~is dead. The "dead" field can be set when entity is destroyed. 
}

bottom line is that: it's ok for the Map representation of an entity to remain 
in the global database as long as the C++ representation has been removed from 
the update/draw list.

*/
