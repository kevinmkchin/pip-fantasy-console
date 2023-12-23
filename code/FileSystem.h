#pragma once

#include "MesaCommon.h"
#include <string>

/**
    Types to hold data in memory
*/
struct BinaryFileHandle
{
    /** Handle for a file in memory */
    u32     size   = 0;        // size of file in memory
    void*   memory = nullptr;  // pointer to file in memory
};

struct BitmapHandle : BinaryFileHandle
{
    /** Handle for an UNSIGNED BYTE bitmap in memory */
    u32 width    = 0;   // image width
    u32 height   = 0;   // image height
    u8  bitDepth = 0;   // bit depth of bitmap in bytes (e.g. bit depth = 3 means there are 3 bytes in the bitmap per pixel)
};

/**
    File system functions 
*/
void FreeFileBinary(BinaryFileHandle& binary_file_to_free);
void ReadFileBinary(BinaryFileHandle& mem_to_read_to, const char* file_path);
bool WriteFileBinary(const BinaryFileHandle& mem_to_write_from, const char* file_path);
std::string ReadFileString(const char* file_path);
void FreeImage(BitmapHandle& image_handle);
void ReadImage(BitmapHandle& image_handle, const char* image_file_path);