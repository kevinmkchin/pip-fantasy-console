#pragma once

#include "PipLangCommon.h"
#include "Chunk.h"

struct RCObject
{
    enum OType
    {
        MAP,
        //ARRAY,
        STRING
    };

    OType type;
    i32 refCount = 0;
};

struct RCString
{
    RCObject base;

    std::string text;
    u32 hash;
    bool isConstant;

    RCString()
    {
        base.type = RCObject::STRING;
        hash = 0;
        isConstant = false;
    }
};

struct HashMapEntry
{
    RCString* key = NULL;
    TValue value;
};

struct HashMap
{
    RCObject base;

    int count;
    int capacity;
    HashMapEntry *entries;

    HashMap()
    {
        base.type = RCObject::MAP;
        count = 0;
        capacity = 0;
        entries = NULL;
    }
};

void AllocateHashMap(HashMap *map);
void FreeHashMap(HashMap *map);
bool HashMapSet(HashMap *map, RCString *key, TValue value, TValue *replaced);
bool HashMapGet(HashMap *map, RCString *key, TValue *value);
bool HashMapDelete(HashMap *map, RCString *key);

static inline bool IsRCObjType(TValue value, RCObject::OType type)
{
    return IS_RCOBJ(value) && AS_RCOBJ(value)->type == type;
}

#define RCOBJ_TYPE(value)      (AS_RCOBJ(value)->type)
#define RCOBJ_IS_STRING(value) IsRCObjType(value, RCObject::STRING)
#define RCOBJ_AS_STRING(value) ((RCString*)AS_RCOBJ(value))
#define RCOBJ_IS_MAP(value)    IsRCObjType(value, RCObject::MAP)
#define RCOBJ_AS_MAP(value)    ((HashMap*)AS_RCOBJ(value))

RCObject *NewRCObject(RCObject::OType type);

void FreeRCObject(RCObject *obj);

RCString *CopyString(const char *buf, int length, bool isConstant);


struct PipFunction
{
    RCString *name;
    int arity;
    Chunk chunk;
};

PipFunction *NewFunction();






//static RCString *TakeString(const char *buf, int length)
//{
//}

//struct MesaScript_List
//{
//    MesaGCObject base;
//
//    std::vector<TValue> list;
//
//    MesaScript_List()
//        : base(MesaGCObject::GCObjectType::List)
//    {}
//
//    /// Simply returns the value at index. Does not increment reference count.
//    TValue AccessListEntry(const i64 index)
//    {
//        return list.at(index);
//    }
//
//    /// Append value to end of list. Increments reference count.
//    void Append(const TValue value);
//
//    /// Replace the value at the given index.
//    void ReplaceListEntryAtIndex(const i64 index, const TValue value);
//
//    /// Should only be called before deletion.
//    void DecrementReferenceCountOfEveryListEntry();
//
//    //void Insert();
//    //void ArrayFront();
//    //void ArrayBack();
//    // Length
//    // Contains
//};
//
//struct MesaScript_Table
//{
//    MesaGCObject base;
//
//    std::unordered_map<std::string, TValue> table;
//
//    MesaScript_Table()
//        : base(MesaGCObject::GCObjectType::Table)
//    {}
//
//    bool Contains(const std::string &key);
//
//    /// Simply returns the value at key. Does not increment reference count.
//    TValue AccessMapEntry(const std::string &key);
//
//    /// Create a new key value pair entry. Increments reference count.
//    void CreateNewMapEntry(const std::string &key, const TValue value);
//
//    /// Assign new value to existing entry perform proper reference counting.
//    void ReplaceMapEntryAtKey(const std::string &key, const TValue value);
//
//    /// Should only be called before deletion.
//    void DecrementReferenceCountOfEveryMapEntry();
//};
//
///// Simply returns the MesaScript_Table associated with the given GCObject id.
//MesaScript_Table *AccessMesaScriptTable(i64 gcObjectId);
//void IncrementReferenceGCObject(i64 gcObjectId);
//void ReleaseReferenceGCObject(i64 gcObjectId);
//i64 RequestNewGCObject(MesaGCObject::GCObjectType gcObjectType);
