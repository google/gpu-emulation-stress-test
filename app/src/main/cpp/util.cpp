/*
* Copyright (C) 2017 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "util.h"

#ifndef _WIN32

#include <sys/time.h>

#else
#include <Windows.h>
#endif

std::vector<std::string> splitLines(const std::string &str) {
    std::vector<std::string> lines;
    const size_t kNull = std::string::npos;

    size_t currStart = 0;
    size_t linePos = str.find("\n");
    bool hasLine = linePos != kNull;

    while (hasLine) {
        lines.push_back(str.substr(currStart, linePos - currStart));
        currStart = linePos + 1;
        linePos = str.find("\n", currStart);
        hasLine = linePos != kNull;
    }

    if (currStart < str.size())
        lines.push_back(str.substr(currStart, kNull));

    return lines;
}

// From platform/external/qemu/android/android-emu/android/base/system/System.cpp
struct TickCountImpl {
private:
    uint64_t mStartTimeUs;
#ifdef _WIN32
    long long mFreqPerSec = 0;    // 0 means 'high perf counter isn't available'
#elif defined(__APPLE__)
    clock_serv_t mClockServ;
#endif
public:
    TickCountImpl() {
#ifdef _WIN32
        LARGE_INTEGER freq;
        if (::QueryPerformanceFrequency(&freq)) {
            mFreqPerSec = freq.QuadPart;
        }
#elif defined(__APPLE__)
        host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &mClockServ);
#endif
        mStartTimeUs = getUs();
    }

#ifdef __APPLE__
    ~TickCountImpl() {
        mach_port_deallocate(mach_task_self(), mClockServ);
    }
#endif

    uint64_t getStartTimeUs() const {
        return mStartTimeUs;
    }

    uint64_t getUs() const {
#ifdef _WIN32
        if (!mFreqPerSec) {
            return ::GetTickCount() * 1000;
        }
        LARGE_INTEGER now;
        ::QueryPerformanceCounter(&now);
        return (now.QuadPart * 1000000ull) / mFreqPerSec;
#elif defined __linux__
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec * 1000000ll + ts.tv_nsec / 1000;
#else // APPLE
        mach_timespec_t mts;
        clock_get_time(mClockServ, &mts);
        return mts.tv_sec * 1000000ll + mts.tv_nsec / 1000;
#endif
    }
};

// This is, maybe, the only static variable that may not be a LazyInstance:
// it holds the actual timestamp at startup, and has to be initialized as
// soon as possible after the application launch.
static const TickCountImpl kTickCount;

uint64_t currTimeUs() {
    return kTickCount.getUs();
}
