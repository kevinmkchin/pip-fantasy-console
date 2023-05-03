#pragma once

#include <fstream>
#include <string>

// Simple utility functions to avoid mistyping directory name
// audio_path("audio.ogg") -> data/audio/audio.ogg
#include "../../ext/cmake_project_path.h"
inline std::string wd_path() { return std::string(PROJECT_WORKING_DIR); }
inline std::string wd_path(const std::string& name) { return wd_path() + std::string(name); }
inline std::string data_path() { return wd_path() + "data/"; }
inline std::string data_path(const std::string& name) { return wd_path() + "data/" + name; }

#if (defined _WIN32)
#define ASSERT(predicate) if(!(predicate)) {*(int*)0 = 0;}
#else
#define ASSERT(predicate) if(!(predicate)) { __builtin_trap(); }
#endif

#define ARRAY_COUNT(a) (sizeof(a) / (sizeof(a[0])))

#define RGB255TO1(r,g,b) ((float)r)/255.f, ((float)g)/255.f, ((float)b)/255.f

#define ISANYOF1(a, x) ((a) == (x))
#define ISANYOF2(a, x, y) ((a) == (x) || (a) == (y))
#define ISANYOF3(a, x, y, z) ((a) == (x) || (a) == (y) || (a) == (z))
#define ISANYOF4(a, x, y, z, w) ((a) == (x) || (a) == (y) || (a) == (z) || (a) == (w))


typedef uint8_t       u8;
typedef uint16_t      u16;
typedef uint32_t      u32;
typedef uint64_t      u64;
typedef int8_t        i8;
typedef int16_t       i16;
typedef int32_t       i32;
typedef int64_t       i64;
typedef uint_fast8_t  u8f;
typedef uint_fast16_t u16f;
typedef uint_fast32_t u32f;
typedef int_fast8_t   i8f;
typedef int_fast16_t  i16f;
typedef int_fast32_t  i32f;
typedef i16           bool16;
typedef i32           bool32;
typedef uint32_t EntityIndex;
#define EntityIndexInvalid UINT32_MAX; //0xFFFFFFFF

#define EDITOR_FIXED_INTERNAL_RESOLUTION_W 1280 //514
#define EDITOR_FIXED_INTERNAL_RESOLUTION_H 914//514 //384


