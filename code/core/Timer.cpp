#include "Timer.h"

#include <chrono>

Timer Time;

using Clock = std::chrono::high_resolution_clock;

Timer::Timer()
        : deltaTime(-1.f)
        , time(0.f)
        , timeScale(1.f)
        , unscaledDeltaTime(-1.f)
{}

float Timer::UpdateDeltaTime()
{
    static Clock::time_point timeAtLastUpdate = Clock::now();

    auto now = Clock::now();
    float elapsedMs = (float)(std::chrono::duration_cast<std::chrono::microseconds>(now - timeAtLastUpdate)).count() * 0.001f;
    timeAtLastUpdate = now;
    float deltaTimeInSeconds = elapsedMs * 0.001f; // elapsed time in SECONDS
    float currentTimeInSeconds = (float)(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() * 0.001f);
    time = currentTimeInSeconds;
    unscaledDeltaTime = deltaTimeInSeconds;
    deltaTime = unscaledDeltaTime * Time.timeScale;

    return deltaTime;
}
