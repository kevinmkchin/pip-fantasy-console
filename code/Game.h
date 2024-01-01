#pragma once

#include "MesaCommon.h"
#include "MesaMath.h"


bool TemporaryGameInit();
void TemporaryGameLoop();
void TemporaryGameShutdown();

struct Space* GetGameActiveSpace();
