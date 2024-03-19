/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *          Provides an implementation of the PlatformManager object
 *          for Talaria platforms.
 */

#include <platform/PlatformManager.h>

#include <platform/internal/GenericPlatformManagerImpl_FreeRTOS.ipp>


#ifdef __cplusplus
extern "C" {
#include <unistd.h>
#include <utils.h>

/* Ideally we should be including the header file but that's a manual
   process as of now, hence declaring required function here.
   Header file name: <components/utils.h> */
// int __attribute__((warn_unused_result)) utils_mount_rootfs(void);
}
#endif
namespace chip {
namespace DeviceLayer {

PlatformManagerImpl PlatformManagerImpl::sInstance;

CHIP_ERROR PlatformManagerImpl::_InitChipStack()
{
    int ret_code;
    mStartTime = System::SystemClock().GetMonotonicTimestamp();

    // Make sure the LwIP core lock has been initialized
    // ReturnErrorOnFailure(Internal::InitLwIPCoreLock());

    // Call _InitChipStack() on the generic implementation base class to finish the initialization process.
    ChipLogDetail(DeviceLayer, "Mounting file system...");
    ret_code = utils_mount_rootfs();

    if (ret_code != 0)
    {
        ChipLogError(DeviceLayer, "Failed to mount file system. Err: 0x%x", ret_code);
        return CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
    }

    ReturnErrorOnFailure(Internal::GenericPlatformManagerImpl_FreeRTOS<PlatformManagerImpl>::_InitChipStack());

    return CHIP_NO_ERROR;
}

void PlatformManagerImpl::_Shutdown()
{
    uint64_t upTime = 0;

    if (GetDiagnosticDataProvider().GetUpTime(upTime) == CHIP_NO_ERROR)
    {
        uint32_t totalOperationalHours = 0;

        if (ConfigurationMgr().GetTotalOperationalHours(totalOperationalHours) == CHIP_NO_ERROR)
        {
            ConfigurationMgr().StoreTotalOperationalHours(totalOperationalHours + static_cast<uint32_t>(upTime / 3600));
        }
        else
        {
            ChipLogError(DeviceLayer, "Failed to get total operational hours of the Node");
        }
    }
    else
    {
        ChipLogError(DeviceLayer, "Failed to get current uptime since the Node’s last reboot");
    }

    Internal::GenericPlatformManagerImpl_FreeRTOS<PlatformManagerImpl>::_Shutdown();
}


} // namespace DeviceLayer
} // namespace chip
