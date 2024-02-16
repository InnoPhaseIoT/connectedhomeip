/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *          Provides implementations of the CHIP System Layer platform
 *          time/clock functions that are suitable for use on the Posix platform.
 */
#include "system/SystemClock.h"

#ifdef __cplusplus
    extern "C" {
#endif
#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
    }
#endif


namespace chip {
namespace System {
namespace Clock {

namespace Internal {
// #pragma message ( "timesupport ClockImpl *** " )
// ClockImpl gClockImpl;
} // namespace Internal

Microseconds64 ClockImpl::GetMonotonicMicroseconds64()
{
    return Clock::Microseconds64(xTaskGetTickCount() * 1000 * 1000);
}

Milliseconds64 ClockImpl::GetMonotonicMilliseconds64()
{
    return Clock::Milliseconds64(xTaskGetTickCount() * 1000);
}

CHIP_ERROR ClockImpl::GetClock_RealTime(Microseconds64 & aCurTime)
{
    aCurTime = Clock::Microseconds64(xTaskGetTickCount());
    return CHIP_NO_ERROR;
}

CHIP_ERROR ClockImpl::GetClock_RealTimeMS(Milliseconds64 & aCurTime)
{
    aCurTime = Clock::Milliseconds64(xTaskGetTickCount());
    return CHIP_NO_ERROR;
}

CHIP_ERROR ClockImpl::SetClock_RealTime(Microseconds64 aNewCurTime)
{
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

} // namespace Clock
} // namespace System
} // namespace chip

unsigned int SystemTime_test()
{
    uint32_t out;
    chip::ChipError err;

    // test codes, enable one at a time for testing ***
    
    // chip::System::Clock::Timestamp tmp = chip::System::SystemClock().GetMonotonicTimestamp();
    // chip::System::Clock::Milliseconds64 tmp = chip::System::SystemClock().GetMonotonicMilliseconds64();
    // chip::System::Clock::Timestamp tmp = chip::System::SystemClock().GetMonotonicMilliseconds64();
    // chip::System::Clock::Microseconds64 tmp = chip::System::SystemClock().GetMonotonicMicroseconds64();

    // chip::System::Clock::Microseconds64 tmp;
    // err = chip::System::SystemClock().GetClock_RealTime(tmp);
    
    // chip::System::Clock::Milliseconds64 tmp;
    // err = chip::System::SystemClock().GetClock_RealTimeMS(tmp);

    chip::System::Clock::Microseconds64 tmp = chip::System::Clock::Microseconds64(1028);
    err = chip::System::SystemClock().SetClock_RealTime(tmp);
    if (err == CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE) {
        return 28;
    }

    out = tmp.count();
    return out;
}
