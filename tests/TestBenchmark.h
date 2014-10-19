
#pragma once

/** Code adapted from https://github.com/juj/MathGeoLib/blob/master/tests/TestRunner.h */

#include "CoreTypes.h"
#include "CoreDefines.h"
#include "CoreStringUtils.h"

#include "Time/Clock.h"

namespace Tundra
{
    namespace Benchmark
    {
        static int Iterations = 1000;
        const static int DefaultIterations = 1000;

        String FormatTime(double ticks)
        {
	        double msecs = ticks * 1000.0 / Clock::TicksPerSec();
	        double secs = msecs / 1000.0;
	        double usecs = msecs * 1000.0;
	        char str[256];
	        if (secs >= 1.0)
		        sprintf(str, "%.3f s", (float)secs);
	        else if (msecs >= 1.0)
		        sprintf(str, "%.3f ms", (float)msecs);
	        else if(usecs >= 1.0)
		        sprintf(str, "%.3f us", (float)usecs);
	        else
		        sprintf(str, "%.3f ns", (float)(usecs*1000.0));
	        return Tundra::String(str);
        }

        struct Result
        {
            Result()
	        {
                iterations = 0;
		        fastestTime = averageTime = worstTime = fastestCycles = 0.0;
	        }

            int iterations;

	        double fastestTime;
	        double averageTime;
	        double worstTime;
	        double fastestCycles;
        };
    }
}

#define BENCHMARK(name, logpad) \
{ \
    Tundra::String benchName = name; \
    int nameLogPad = logpad; \
	Tundra::Benchmark::Result result; \
    unsigned long long bestTsc = (unsigned long long)-1; \
    tick_t bestTicks = TICK_INF; \
    tick_t accumTicks = 0; \
    tick_t worstTicks = 0; \
	for(int i = 0; i < Tundra::Benchmark::Iterations; ++i) \
    { \
        unsigned long long startTsc = Clock::Rdtsc(); \
        tick_t start = Clock::Tick();
        // USER CODE GOES HERE

#define BENCHMARK_STEP_END \
	    unsigned long long endTsc = Clock::Rdtsc(); \
	    tick_t end = Clock::Tick(); \
	    tick_t elapsedTicks = end - start; \
	    unsigned long long elapsedTsc = (unsigned long long)((startTsc != 0 || endTsc != 0) ? (endTsc - startTsc) : elapsedTicks); \
	    bestTsc = Min(bestTsc, elapsedTsc); \
	    bestTicks = Min(bestTicks, elapsedTicks); \
	    worstTicks = Max(worstTicks, elapsedTicks); \
	    accumTicks += elapsedTicks; \

#define BENCHMARK_END \
    } \
	result.iterations = Tundra::Benchmark::Iterations; \
	result.fastestCycles = (double)bestTsc; \
	result.fastestTime = (double)bestTicks; \
	result.averageTime = (double)accumTicks / Tundra::Benchmark::Iterations; \
	result.worstTime = (double)worstTicks; \
    Tundra::String str = "Best " + PadString(Tundra::Benchmark::FormatTime(result.fastestTime), -10) + \
                         " / " + PadString(result.fastestCycles, 5) + \
                         "  Avg " + PadString(Tundra::Benchmark::FormatTime(result.averageTime), -10) + \
                         "  Worst " + PadString(Tundra::Benchmark::FormatTime(result.worstTime), -10) + \
                         "  " + String(Tundra::Benchmark::Iterations) + " iters"; \
    Log(PadString(benchName, nameLogPad) + str, 2); \
    Tundra::Benchmark::Iterations = Tundra::Benchmark::DefaultIterations; /* Reset to default iterations for next benchmark */ \
}
