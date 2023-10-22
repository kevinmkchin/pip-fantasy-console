#pragma once

class Timer
{
public:
    Timer();

    float UpdateDeltaTime();

    /** Returns the time elapsed in seconds since the last timestamp call. */
    float TimeStamp();

public:
    // Global accessor for delta time. Could be in whatever unit you want, but I like seconds.
    float deltaTime;

    // Time since epoch
    float time;

    // Time since program start
    float timeSinceStart;

    // The scale at which time passes
    float timeScale;

    float unscaledDeltaTime;

};

extern Timer Time;
