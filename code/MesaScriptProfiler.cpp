#include "Timer.h"

#include "singleheaders/stb_sprintf.h"

#include <iostream>
#include <stack>

#define PLPROFILER_INTERPRETER              0

#define PLPROFILER_BINOP_RELOP              4

#define PLPROFILER_HASHING                  8
#define PLPROFILER_TRANSIENCY_HANDLING      9
#define PLPROFILER_PRINT                    10

static double measurements[16];
static double timestamp[16];

static void PLProfilerBegin(int tag)
{
    timestamp[tag] = Time.TimeSinceProgramStartInSeconds();
}

static void PLProfilerEnd(int tag)
{
    double duration = Time.TimeSinceProgramStartInSeconds() - timestamp[tag];
    measurements[tag] += duration;
}

enum class PLPROFILER_INTERPRETER_SYSTEM
{
    INTERPRET_STATEMENT,
    INTERPRET_EXPRESSION,
    INTERPRET_PROCEDURE_CALL,
    DEFAULT
};

static double __profiler_InterpreterSystemMeasurements[(int)PLPROFILER_INTERPRETER_SYSTEM::DEFAULT];
static std::stack<PLPROFILER_INTERPRETER_SYSTEM> __profiler_interpreterStack;
static double lastTimeStamp = 0.0;

static void PLStartProfiling()
{
    measurements[PLPROFILER_INTERPRETER] = 0.0;
    measurements[PLPROFILER_HASHING] = 0.0;
    measurements[PLPROFILER_TRANSIENCY_HANDLING] = 0.0;
    measurements[PLPROFILER_PRINT] = 0.0;

    timestamp[PLPROFILER_INTERPRETER] = Time.TimeSinceProgramStartInSeconds();
    timestamp[PLPROFILER_HASHING] = 0.0;
    timestamp[PLPROFILER_TRANSIENCY_HANDLING] = 0.0;
    timestamp[PLPROFILER_PRINT] = 0.0;

    for (int i = 0; i < (int)PLPROFILER_INTERPRETER_SYSTEM::DEFAULT; ++i)
    {
        __profiler_InterpreterSystemMeasurements[i] = 0.0;
    }
    lastTimeStamp = 0.0;
    while (!__profiler_interpreterStack.empty())
    {
        __profiler_interpreterStack.pop();
    }
}

static void PLProfilerPush(PLPROFILER_INTERPRETER_SYSTEM enteredSystem)
{
    double currentTimeStamp = Time.TimeSinceProgramStartInSeconds();

    if (!__profiler_interpreterStack.empty())
    {
        PLPROFILER_INTERPRETER_SYSTEM currentActiveSystem = __profiler_interpreterStack.top();
        double durationSpentInCurrentSystem = currentTimeStamp - lastTimeStamp;
        __profiler_InterpreterSystemMeasurements[(int)currentActiveSystem] += durationSpentInCurrentSystem;
    }

    lastTimeStamp = currentTimeStamp;
    __profiler_interpreterStack.push(enteredSystem);
}

static void PLProfilerPop()
{
    double currentTimeStamp = Time.TimeSinceProgramStartInSeconds();

    PLPROFILER_INTERPRETER_SYSTEM currentActiveSystem = __profiler_interpreterStack.top();
    double durationSpentInCurrentSystem = currentTimeStamp - lastTimeStamp;
    __profiler_InterpreterSystemMeasurements[(int)currentActiveSystem] += durationSpentInCurrentSystem;

    lastTimeStamp = currentTimeStamp;
    __profiler_interpreterStack.pop();
}

static void PLStopProfiling(std::ostream& profilerOutput)
{
    measurements[PLPROFILER_INTERPRETER] = Time.TimeSinceProgramStartInSeconds() - timestamp[PLPROFILER_INTERPRETER];
    double totalDuration = measurements[PLPROFILER_INTERPRETER];

    profilerOutput << "--- Results ---" << std::endl;
    profilerOutput << "Total duration: " << totalDuration << std::endl;
    profilerOutput << "Category\t\t\tDuration\tRatio" << std::endl;
    profilerOutput << "unordered_map hashing" << "\t" << measurements[PLPROFILER_HASHING] << "\t" << measurements[PLPROFILER_HASHING] / totalDuration * 100.0 << "%" << std::endl;
    profilerOutput << "Transiency handling" << "\t" << measurements[PLPROFILER_TRANSIENCY_HANDLING] << "\t" << measurements[PLPROFILER_TRANSIENCY_HANDLING] / totalDuration * 100.0 << "%" << std::endl;
    profilerOutput << "BinOp / RelOp" << "\t" << measurements[PLPROFILER_BINOP_RELOP] << "\t" << measurements[PLPROFILER_BINOP_RELOP] / totalDuration * 100.0 << "%" << std::endl;
    profilerOutput << "print()" << "\t\t\t" << measurements[PLPROFILER_PRINT] << "\t" << measurements[PLPROFILER_PRINT] / totalDuration * 100.0 << "%" << std::endl;
    profilerOutput << std::endl;

    double ieSample = __profiler_InterpreterSystemMeasurements[(int)PLPROFILER_INTERPRETER_SYSTEM::INTERPRET_EXPRESSION];
    double isSample = __profiler_InterpreterSystemMeasurements[(int)PLPROFILER_INTERPRETER_SYSTEM::INTERPRET_STATEMENT];
    double ipcSample = __profiler_InterpreterSystemMeasurements[(int)PLPROFILER_INTERPRETER_SYSTEM::INTERPRET_PROCEDURE_CALL];

    profilerOutput << "System\t\t\tDuration\tRatio" << std::endl;
    profilerOutput << "InterpretExpression" << "\t" << ieSample << "\t" << ieSample / totalDuration * 100.0 << "%" << std::endl;
    profilerOutput << "InterpretStatement" << "\t" << isSample << "\t" << isSample / totalDuration * 100.0 << "%" << std::endl;
    profilerOutput << "InterpretProcedureCall" << "\t" << ipcSample << "\t" << ipcSample / totalDuration * 100.0 << "%" << std::endl;
    profilerOutput << std::endl;

}

