#include "GameData.h"
#include "FileSystem.h"
#include "PrintLog.h"
#define KEVIN_BYTE_BUFFER_IMPLEMENTATION
#include "ByteBuffer.h"

GameData gamedata;


static ByteBuffer gameDataSerializeBuffer;

void SerializeGameData(GameData *gameData, const char *path)
{
    PrintLog.Message("Saving game data.");

    gameDataSerializeBuffer = ByteBufferNew();

    i32 codeBufferSz = (i32)gameData->codePage1.size();
    ByteBufferWrite(&gameDataSerializeBuffer, i32, codeBufferSz);
    void *codeBufferSource = (void*)gameData->codePage1.c_str();
    ByteBufferWriteBulk(&gameDataSerializeBuffer, codeBufferSource, codeBufferSz);

    if (ByteBufferWriteToFile(&gameDataSerializeBuffer, path) == 0)
        PrintLog.Error("Failed to save game data.");

    ByteBufferFree(&gameDataSerializeBuffer);
}

void DeserializeGameData(const char *path, GameData *gameData)
{
    PrintLog.Message("Loading new game data.");

    gameDataSerializeBuffer = ByteBufferNew();
    if(ByteBufferReadFromFile(&gameDataSerializeBuffer, path))
    {

        i32 codeBufferSz;
        ByteBufferRead(&gameDataSerializeBuffer, i32, &codeBufferSz);
        gameData->codePage1.resize(codeBufferSz);
        void *codeBufferTarget = (void*)gameData->codePage1.c_str();
        ByteBufferReadBulk(&gameDataSerializeBuffer, codeBufferTarget, codeBufferSz);

    }
    else
    {
        PrintLog.Error("Failed to load game data.");
    }
    ByteBufferFree(&gameDataSerializeBuffer);
}

void ClearGameData(GameData *gameData)
{
    PrintLog.Message("Clearing loaded game data.");
    gamedata.sprites.clear();
    gamedata.codePage1.clear();
}
