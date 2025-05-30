#pragma once

#include "MesaCommon.h"
#include "MesaMath.h"

#include <cfloat>
#include <cstring>

/* Call after OpenGL calls to check for errors. */
bool GLHasErrors();

void PickThreeRandomInts(int* x, int* y, int* z, int cap);

/* Pick random integer in range [min, max] inclusive. */
int RandomInt(int min, int max);

i32 ModifyASCIIBasedOnModifiers(i32 keycodeASCII, bool shift);

// normalized hsv to rgb
vec3 HSVToRGB(float h, float s, float v);
// normalized rgb to hsv
vec3 RGBToHSV(float r, float g, float b);

template<typename T>
inline bool IsOneOfArray(T v, T* array, int count)
{
    for (int i = 0; i < count; ++i)
        if (v == *(array + i)) return true;
    return false;
}

std::string& RemoveCharactersFromEndOfString(std::string& str, char c);

template<typename T, int _count> struct NiceArray
{
    /** Nice array wrapper for when you want to keep track of how many active/relevant
        elements are in the array. Essentially a dynamic sized array/vector with a
        maximum defined capacity (s.t. it can be defined on stack or static storage). */

    T data[_count] = {};
    int count = 0;
    const int capacity = _count;

    // todo maybe Insert and Erase?

    bool Contains(T v)
    {
        for (int i = 0; i < count; ++i)
        {
            if (*(data + i) == v) return true;
        }
        return false;
    }

    void EraseAt(int index)
    {
        if (index < count - 1)
        {
            memmove(data + index, data + index + 1, (count - index - 1) * sizeof(*data));
        }
        --count;
    }

    void EraseFirstOf(T v)
    {
        for (int i = 0; i < count; ++i)
        {
            if (*(data + i) == v)
            {
                EraseAt(i);
                break;
            }
        }
    }

    void EraseAllOf(T v)
    {
        for (int i = 0; i < count; ++i)
        {
            if (*(data + i) == v)
            {
                EraseAt(i);
            }
        }
    }

    bool NotAtCapacity()
    {
        return count < capacity;
    }

    void PushBack(T elem)
    {
        data[count] = elem;
        ++count;
    }

    void PopBack()
    {
        --count;
        memset(data + count, 0, sizeof(*data));
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

class Timer
{
public:
    Timer();

    float UpdateDeltaTime();

    /** Returns the time elapsed in seconds since the last timestamp call. */
    float TimeStamp();

    double TimeSinceProgramStartInSeconds();

public:
    // Global accessor for delta time. Could be in whatever unit you want, but I like seconds.
    float deltaTime;

    // Time since epoch (TODO(Kevin): wtf?)
    float time;

    // Time since program start
    float timeSinceStart;

    // The scale at which time passes
    float timeScale;

    float unscaledDeltaTime;

};

class CorePrintLog {
public:
    void Message(const std::string& message);
    void Warning(const std::string& warning_message);
    void Error(const std::string& error_message);
};

extern Timer Time;
extern CorePrintLog PrintLog;

