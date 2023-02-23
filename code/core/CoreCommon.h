#pragma once

#include <fstream>
#include <string>

// Simple utility functions to avoid mistyping directory name
// audio_path("audio.ogg") -> data/audio/audio.ogg
inline std::string wd_path() { return std::string(""); }
inline std::string wd_path(const std::string& name) { return wd_path() + std::string(name); }
inline std::string data_path() { return wd_path() + "data/"; }
inline std::string data_path(const std::string& name) { return wd_path() + "data/" + name; }


#if (defined _WIN64)
#define ASSERT(predicate) if(!(predicate)) {*(int*)0 = 0;}
#else
#define ASSERT(predicate) if(!(predicate)) { __builtin_trap(); }
#endif

#define RGB255TO1(r,g,b) ((float)r)/255.f, ((float)g)/255.f, ((float)b)/255.f

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

#define EDITOR_FIXED_INTERNAL_RESOLUTION_W 514
#define EDITOR_FIXED_INTERNAL_RESOLUTION_H 514 //384


