#include "Timer.h"

#include "singleheaders/stb_sprintf.h"

#include <iostream>

#define PLPROFILER_INTERPRETER              0

#define PLPROFILER_INTERPRET_STATEMENT      1
#define PLPROFILER_INTERPRET_EXPRESSION     2
#define PLPROFILER_INTERPRET_PROCEDURE_CALL 3

#define PLPROFILER_HASHING                  8
#define PLPROFILER_PRINT                    10

static double measurements[16];
static double timestamp[16];

static void PLStartProfiling()
{
    measurements[PLPROFILER_INTERPRETER] = 0.0;
    measurements[PLPROFILER_HASHING] = 0.0;
    measurements[PLPROFILER_PRINT] = 0.0;

    timestamp[PLPROFILER_INTERPRETER] = Time.TimeSinceProgramStartInSeconds();
    timestamp[PLPROFILER_HASHING] = 0.0;
    timestamp[PLPROFILER_PRINT] = 0.0;
}

static void PLProfilerEnter(int tag)
{
    timestamp[tag] = Time.TimeSinceProgramStartInSeconds();
}

static void PLProfilerExit(int tag)
{
    double duration = Time.TimeSinceProgramStartInSeconds() - timestamp[tag];
    measurements[tag] += duration;
}

static void PLStopProfiling(std::ostream& profilerOutput)
{
    measurements[PLPROFILER_INTERPRETER] = Time.TimeSinceProgramStartInSeconds() - timestamp[PLPROFILER_INTERPRETER];

    profilerOutput << "Total duration: " << measurements[PLPROFILER_INTERPRETER] << std::endl;
    profilerOutput << "Spent " << measurements[PLPROFILER_HASHING] / measurements[PLPROFILER_INTERPRETER] * 100.0 << "% of execution time in MesaScript_Table.Access and Contains" << std::endl;
    profilerOutput << "Spent " << measurements[PLPROFILER_PRINT] / measurements[PLPROFILER_INTERPRETER] * 100.0 << "% of execution time in print()" << std::endl;
    profilerOutput << std::endl;
}

