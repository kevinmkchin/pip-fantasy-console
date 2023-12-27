#include "Object.h"
#include "VM.h"


void InitHashMap(HashMap *map)
{
    map->count = 0;
    map->capacity = 0;
    // TODO
}

void FreeHashMap(HashMap *map)
{
    // TODO
}

bool HashMapSet(HashMap *map, RCString *key, TValue value)
{
    bool isNewKey = false;

    HashMapEntry e;
    e.key = key;
    e.value = value;

    auto entry = map->ds.find(key->text);
    if (entry != map->ds.end())
    {
        entry->second = e;
    }
    else
    {
        map->ds.emplace(key->text, e);
        isNewKey = true;
    }
    return isNewKey;
}

bool HashMapGet(HashMap *map, RCString *key, TValue *value)
{
    if (map->ds.size() == 0) return false;

    auto entry = map->ds.find(key->text);
    if (entry != map->ds.end())
    {
        *value = entry->second.value;
        return true;
    }
    else
    {
        return false;
    }
}

bool HashMapDelete(HashMap *map, RCString *key)
{
    return map->ds.erase(key->text) == 1;
}

static RCString *HashMapFindString(HashMap *map, const char *buf, int length) /*, u32 hash) */
{
    auto entry = map->ds.find(std::string(buf, length));
    if (entry != map->ds.end())
    {
        return entry->second.key;
    }
    else
    {
        return NULL;
    }
}



RCObject *NewRCObject(RCObject::OType type)
{
    RCObject *rcobj = NULL;

    switch (type)
    {
    case RCObject::STRING:
        rcobj = (RCObject *) new RCString(); // Throw somewhere on heap for now
        break;
        //case MesaGCObject::GCObjectType::String:
        //    gcobj = (MesaGCObject *) new MesaScript_String();
        //    break;
        //case MesaGCObject::GCObjectType::List: {
        //    gcobj = (MesaGCObject *) new MesaScript_List();
        //    break;
    }

    return rcobj;
}

RCString *CopyString(const char *buf, int length)
{
    RCString *internedString = HashMapFindString(&vm.interned_strings, buf, length);
    if (internedString)
    {
        return internedString;
    }

    RCString *string = (RCString *)NewRCObject(RCObject::STRING);
    string->text = std::string(buf, length);
    
    HashMapSet(&vm.interned_strings, string, TValue());

    return string;
}
