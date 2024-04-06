#include "Game.h"
#include "ProjectData.h"

#include "PipAPI.h"

bool TemporaryGameInit()
{
    CompileRuntimeTextureAtlas(&runtimeTextureAtlas, &projectData);
//    runtimeTextureAtlas.textureAtlas.push_back(Gfx::CreateGPUTextureFromDisk(data_path("spr_ground_01.png").c_str()));
//    runtimeTextureAtlas.textureAtlas.push_back(Gfx::CreateGPUTextureFromDisk(data_path("spr_crosshair_00.png").c_str()));

    std::string& gamecode = projectData.codePage1;

    PipLangVM_InitVM();
    InitializePipAPI();
    InterpretResult rungamecodeResult = PipLangVM_RunGameCode(gamecode.c_str());
    if (rungamecodeResult != InterpretResult::OK)
    {
        return false;
    }
    ReadBackGfxValues();

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
    TearDownRuntimeTextureAtlas(&runtimeTextureAtlas);
}
