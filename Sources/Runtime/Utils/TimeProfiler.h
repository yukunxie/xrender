#pragma once

#include <stdio.h>
#include <chrono>
#include <iostream>

class TimeProfiler
{
public:
    TimeProfiler() = delete;
    
    explicit TimeProfiler(const char* tag)
    {
        tag_ = tag;
        startTime_ = std::chrono::high_resolution_clock::now();
    }
    
    ~TimeProfiler()
    {
        auto nowTime = std::chrono::high_resolution_clock::now();
        long duration = static_cast<long>(std::chrono::duration_cast<std::chrono::microseconds>(
            nowTime - startTime_).count());
		printf("@@@@TimeProfiler: %s : %.03fms\n", tag_.c_str(), duration / 1000.0f);
        //LOGI("@@@@TimeProfiler: %s : %.03fms", tag_.c_str(), duration/1000.0f);
    }

protected:
    std::string tag_;
    std::chrono::steady_clock::time_point startTime_;
};

#define SCOPED_PROFILING_GUARD(name) TimeProfiler timeProfiler##__LINE__(name)
