#pragma once

class Timer
{
public:
    Timer();

    float UpdateDeltaTime();

public:
    // Global accessor for delta time. Could be in whatever unit you want, but I like seconds.
    float deltaTime;

    // Time since epoch
    float time;

    // The scale at which time passes
    float timeScale;

    float unscaledDeltaTime;

};

extern Timer Time;
