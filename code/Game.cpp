#include "Game.h"
#include "GameData.h"

#include "PipAPI.h"

bool TemporaryGameInit()
{
    std::string& gamecode = gamedata.codePage1;

    PipLangVM_InitVM();
    InitializePipAPI();
    InterpretResult rungamecodeResult = PipLangVM_RunGameCode(gamecode.c_str());
    if (rungamecodeResult != InterpretResult::OK)
    {
        return false;
    }

    return true;
}

void TemporaryGameLoop()
{
    UpdatePipAPI();
//    PipLangVM_RunGameFunction("pretick");
    PipLangVM_RunGameFunction("tick");
//    PipLangVM_RunGameFunction("posttick");
//    PipLangVM_RunGameFunction("draw");
    ReadBackGfxValues();
}

void TemporaryGameShutdown()
{
    TeardownPipAPI();
    PipLangVM_FreeVM();
}
