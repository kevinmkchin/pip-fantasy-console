#pragma once

#include "CoreCommon.h"

#include <cfloat>
#include <cstring>

/* Call after OpenGL calls to check for errors. */
bool GLHasErrors();

void PickThreeRandomInts(int* x, int* y, int* z, int cap);

/* Pick random integer in range [min, max] inclusive. */
int RandomInt(int min, int max);

i32 ModifyASCIIBasedOnModifiers(i32 keycodeASCII, bool shift);

template<typename T>
inline bool IsOneOfArray(T v, T* array, int count)
{
    for (int i = 0; i < count; ++i)
        if (v == *(array + i)) return true;
    return false;
}

std::string& RemoveCharactersFromEndOfString(std::string& str, const char c);

template<typename T, int _count> struct NiceArray
{
    /** Nice array wrapper for when you want to keep track of how many active/relevant
        elements are in the array. Essentially a dynamic sized array/vector with a
        maximum defined capacity (s.t. it can be defined on stack or static storage). */

    T data[_count] = {};
    int count = 0;
    const int capacity = _count;

    void PushBack(T elem)
    {
        data[count] = elem;
        ++count;
    }

    T& At(int index)
    {
        return *(data + index);
    }

    T& At(unsigned int index)
    {
        return At((int) index);
    }

    T& Back()
    {
        return *(data + count - 1);
    }

    void ResetCount()
    {
        count = 0;
    }

    void ResetToZero()
    {
        memset(data, 0, capacity * sizeof(*data));
    }
};
