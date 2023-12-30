#include "Object.h"
#include "VM.h"


#define MAX_LOADFACTOR 0.7

static RCString *HashMapFindString(HashMap *map, const char *buf, int length, u32 hash)
{
    if (map->count == 0) return NULL;

    u32 index = hash & (map->capacity - 1);
    for (;;)
    {
        HashMapEntry *entry = &map->entries[index];
        if (entry->key == NULL)
        {
            // Check if empty non-tombstone entry
            if (!AS_BOOL(entry->value)) return NULL;
        }
        else if (entry->key->hash == hash && entry->key->text == std::string(buf, length))
        {
            return entry->key;
        }
        index = (index + 1) & (map->capacity - 1);
    }
}

static HashMapEntry *FindEntry(HashMapEntry *entries, int capacity, RCString *key)
{
    u32 index = key->hash & (capacity - 1);

    HashMapEntry *firstTombstone = NULL;
    for (;;)
    {
        HashMapEntry *entry = &entries[index];
        if (entry->key == NULL)
        {
            if (!AS_BOOL(entry->value))
            {
                return firstTombstone != NULL ? firstTombstone : entry;
            }
            else
            {
                if (firstTombstone == NULL) firstTombstone = entry;
            }
        }
        else if (entry->key == key)
        {
            return entry;
        }

        index = (index + 1) & (capacity - 1);
    }
}

static void AdjustCapacity(HashMap *map, int newCapacity)
{
    HashMapEntry *entries = (HashMapEntry *)calloc(newCapacity, sizeof(HashMapEntry));
    // calloc-ed so empty entry's value is BOOLEAN FALSE

    map->count = 0;
    for (int i = 0; i < map->capacity; ++i)
    {
        HashMapEntry *entry = &map->entries[i];
        if (entry->key == NULL) continue;
        HashMapEntry *dest = FindEntry(entries, newCapacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        ++map->count;
    }
    if (map->entries != NULL) free(map->entries);

    map->entries = entries;
    map->capacity = newCapacity;
}

void AllocateHashMap(HashMap *map)
{
    *map = HashMap();
    AdjustCapacity(map, 8);
}

void FreeHashMap(HashMap *map)
{
    free(map->entries);
    *map = HashMap();
}

bool HashMapSet(HashMap *map, RCString *key, TValue value, TValue *replaced)
{
    if ((double)map->count + 1 > (double)map->capacity * MAX_LOADFACTOR)
    {
        AdjustCapacity(map, map->capacity * 2);
    }

    HashMapEntry *entry = FindEntry(map->entries, map->capacity, key);
    bool isNewKey = entry->key == NULL;
    if (isNewKey && !AS_BOOL(entry->value)) ++(map->count);
    if (!isNewKey && replaced) *replaced = entry->value;
    entry->key = key;
    entry->value = value;
    return isNewKey;
}

bool HashMapGet(HashMap *map, RCString *key, TValue *value)
{
    // if (map->count == 0) return false;
    HashMapEntry *entry = FindEntry(map->entries, map->capacity, key);
    if (entry->key == NULL) return false;

    *value = entry->value;
    return true;
}

bool HashMapDelete(HashMap *map, RCString *key)
{
    HashMapEntry *entry = FindEntry(map->entries, map->capacity, key);
    if (entry->key == NULL) return false;

    // Place tombstone
    entry->key = NULL;
    entry->value = BOOL_VAL(true); // As opposed to BOOLEAN FALSE for empty entry
    return true;
}




static u32 HashString(const char *key, int length)
{
    u32 hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (u8)key[i];
        hash *= 16777619;
    }
    return hash;

    // TODO try diff hash https://cp-algorithms.com/string/string-hashing.html
    //for (int i = 0; i < length; ++i)
    //{
    //    (u8)(key[i]) 
    //}
}

RCObject *NewRCObject(RCObject::OType type)
{
    RCObject *rcobj = NULL;

    switch (type)
    {
        case RCObject::STRING:
            rcobj = (RCObject*) new RCString(); // Throw somewhere on heap for now
            break;
        case RCObject::MAP:
            rcobj = (RCObject*) new HashMap();
            AllocateHashMap((HashMap*)rcobj);
            break;
    }

    return rcobj;
}

void FreeRCObject(RCObject *obj)
{
    switch (obj->type)
    {
        case RCObject::STRING:
        {
            RCString *str = (RCString*)obj;
            if (!str->isConstant)
            {
                HashMapDelete(&vm.interned_strings, str);
                free(obj);
            }
            break;
        }
        case RCObject::MAP:
        {
            HashMap *map = (HashMap*)obj;
            // TODO Decrement ref of every map entry.
            FreeHashMap(map);
            free(obj);
            break;
        }
    }

}

RCString *CopyString(const char *buf, int length, bool isConstant)
{
    u32 stringhash = HashString(buf, length);

    RCString *internedString = HashMapFindString(&vm.interned_strings, buf, length, stringhash);
    if (internedString)
    {
        return internedString;
    }

    RCString *string = (RCString*)NewRCObject(RCObject::STRING);
    string->text = std::string(buf, length);
    string->hash = stringhash;
    string->isConstant = isConstant;

    HashMapSet(&vm.interned_strings, string, TValue(), NULL);

    return string;
}

#include <vector>
std::vector<PipFunction*> trackFunctions;

PipFunction *NewFunction()
{
    PipFunction * fn = new PipFunction();
    fn->arity = 0;
    fn->name = NULL;
    InitChunk(&fn->chunk);

    trackFunctions.push_back(fn);

    return fn;
}
