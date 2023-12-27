#pragma once

#include "PipLangCommon.h"

struct RCObject
{
    enum OType
    {
        //MAP,
        //LIST,
        STRING
    };

    OType type;
    i32 refCount = 0;
};

struct RCString
{
    RCObject base;

    std::string text;

    RCString()
    {
        base.type = RCObject::STRING;
    }
};

struct HashMapEntry
{
    RCString* key;
    TValue value;
};

struct HashMap
{
    int count;
    int capacity;

    std::unordered_map<std::string, HashMapEntry> ds;
};

void InitHashMap(HashMap *map);
void FreeHashMap(HashMap *map);
bool HashMapSet(HashMap *map, RCString *key, TValue value);
bool HashMapGet(HashMap *map, RCString *key, TValue *value);
bool HashMapDelete(HashMap *map, RCString *key);

static inline bool IsRCObjType(TValue value, RCObject::OType type)
{
    return IS_RCOBJ(value) && AS_RCOBJ(value)->type == type;
}

#define RCOBJ_TYPE(value)      (AS_RCOBJ(value)->type)
#define RCOBJ_IS_STRING(value) IsRCObjType(value, RCObject::STRING)
#define RCOBJ_AS_STRING(value) ((RCString*)AS_RCOBJ(value))

RCObject *NewRCObject(RCObject::OType type);

RCString *CopyString(const char *buf, int length);

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
