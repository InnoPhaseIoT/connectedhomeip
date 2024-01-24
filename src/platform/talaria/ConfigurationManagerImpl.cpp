/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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
 *          Provides the implementation of the Device Layer ConfigurationManager object
 *          for Linux platforms.
 */

#include <app-common/zap-generated/cluster-objects.h>
// #include <ifaddrs.h>
#include <lib/core/CHIPVendorIdentifiers.hpp>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
// #include <netpacket/packet.h>
#include <platform/CHIPDeviceConfig.h>
#include <platform/ConfigurationManager.h>
#include <platform/DiagnosticDataProvider.h>
#include <platform/KeyValueStoreManager.h>
// #include <platform/Linux/PosixConfig.h>
#include <platform/internal/GenericConfigurationManagerImpl.ipp>

#include <algorithm>

namespace chip {
namespace DeviceLayer {

using namespace ::chip::DeviceLayer::Internal;

ConfigurationManagerImpl & ConfigurationManagerImpl::GetDefaultInstance()
{
    static ConfigurationManagerImpl sInstance;
    return sInstance;
}

CHIP_ERROR ConfigurationManagerImpl::Init()
{
    CHIP_ERROR err;
    err = Internal::GenericConfigurationManagerImpl<TalariaConfig>::Init();
    SuccessOrExit(err);

exit:
    return err;
}

CHIP_ERROR ConfigurationManagerImpl::GetInitialPairingHint(uint16_t & pairingHint)
{
#define CHIP_COMMISSIONING_HINT_SETUP_BUTTON_PRESS  15
    static int user_intent_commissioning_flow = os_get_boot_arg_int("matter.commissioning.flow_type", 0);

    if(user_intent_commissioning_flow == 1) {
        pairingHint = (1 << CHIP_COMMISSIONING_HINT_SETUP_BUTTON_PRESS);
    } else {
        GenericConfigurationManagerImpl<Internal::TalariaConfig>::GetInitialPairingHint(pairingHint);
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::GetSecondaryPairingHint(uint16_t & pairingHint)
{
#define CHIP_COMMISSIONING_HINT_SETUP_BUTTON_PRESS  15
    static int user_intent_commissioning_flow = os_get_boot_arg_int("matter.commissioning.flow_type", 0);

    if(user_intent_commissioning_flow == 1) {
        pairingHint = (1 << CHIP_COMMISSIONING_HINT_SETUP_BUTTON_PRESS | 1 << CHIP_COMMISSIONING_HINT_INDEX_SEE_ADMINISTRATOR_UX );
    } else {
        GenericConfigurationManagerImpl<Internal::TalariaConfig>::GetSecondaryPairingHint(pairingHint);
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::GetInitialPairingInstruction(char * buf, size_t bufSize)
{
#define CHIP_DEVICE_CONFIG_TALARIA_PAIRING_INITIAL_INSTRUCTION  "3"
    static int user_intent_commissioning_flow = os_get_boot_arg_int("matter.commissioning.flow_type", 0);

    if(user_intent_commissioning_flow == 1) {
        ReturnErrorCodeIf(bufSize < sizeof(CHIP_DEVICE_CONFIG_TALARIA_PAIRING_INITIAL_INSTRUCTION), CHIP_ERROR_BUFFER_TOO_SMALL);
        strcpy(buf, CHIP_DEVICE_CONFIG_TALARIA_PAIRING_INITIAL_INSTRUCTION);
    } else {
        GenericConfigurationManagerImpl<Internal::TalariaConfig>::GetInitialPairingInstruction(buf, bufSize);
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::GetSecondaryPairingInstruction(char * buf, size_t bufSize)
{
#define CHIP_DEVICE_CONFIG_TALARIA_PAIRING_SECONDARY_INSTRUCTION "3"
    static int user_intent_commissioning_flow = os_get_boot_arg_int("matter.commissioning.flow_type", 0);

    if(user_intent_commissioning_flow == 1) {
        ReturnErrorCodeIf(bufSize < sizeof(CHIP_DEVICE_CONFIG_TALARIA_PAIRING_SECONDARY_INSTRUCTION), CHIP_ERROR_BUFFER_TOO_SMALL);
        strcpy(buf, CHIP_DEVICE_CONFIG_TALARIA_PAIRING_SECONDARY_INSTRUCTION);
    } else {
        GenericConfigurationManagerImpl<Internal::TalariaConfig>::GetSecondaryPairingInstruction(buf, bufSize);
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::GetPrimaryWiFiMACAddress(uint8_t * buf)
{
    struct ifaddrs * addresses = nullptr;
    struct sockaddr_ll * mac   = nullptr;
    CHIP_ERROR error           = CHIP_NO_ERROR;
/*/*MJ-merge
    uint8_t hard_code_buf_esp32[] = {0xac,0x67,0xb2,0x53,0x8d,0xa8};
    uint8_t hard_code_t2_mac[] = {0xe0, 0x69, 0x3a, 0x00, 0x01, 0x01};
    memcpy(buf, hard_code_t2_mac, sizeof(hard_code_t2_mac));
*/
exit:
    return error;
}

bool ConfigurationManagerImpl::CanFactoryReset()
{
    // TODO(#742): query the application to determine if factory reset is allowed.
    return true;
}

void ConfigurationManagerImpl::InitiateFactoryReset()
{
    PlatformMgr().ScheduleWork(DoFactoryReset);
}

CHIP_ERROR ConfigurationManagerImpl::ReadPersistedStorageValue(::chip::Platform::PersistedStorage::Key key, uint32_t & value)
{
#if 0 /*Mj-merge*/
    TalariaConfig::Key configKey{ TalariaConfig::kConfigNamespace_ChipCounters, key };

    CHIP_ERROR err = ReadConfigValue(configKey, value);
    if (err == CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        err = CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
    }
    return err;
#endif
    // PosixConfig::Key configKey{ PosixConfig::kConfigNamespace_ChipCounters, key };

    // CHIP_ERROR err = ReadConfigValue(configKey, value);
    // if (err == CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND)
    // {
    //     err = CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
    // }
    // return err;
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::WritePersistedStorageValue(::chip::Platform::PersistedStorage::Key key, uint32_t value)
{
    // PosixConfig::Key configKey{ PosixConfig::kConfigNamespace_ChipCounters, key };
    ///*MJ-merge*/TalariaConfig::Key configKey{ TalariaConfig::kConfigNamespace_ChipCounters, key };
    // return WriteConfigValue(key, value);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::ReadConfigValue(Key key, bool & val)
{
    // return PosixConfig::ReadConfigValue(key, val);
    // return CHIP_NO_ERROR;
    return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    ///*MJ-merge*/return TalariaConfig::ReadConfigValue(key, val);
    ///*MJ-merge*/return CHIP_NO_ERROR;
}

///*MJ-merge*/#if 0
CHIP_ERROR ConfigurationManagerImpl::ReadConfigValue(Key key, uint16_t & val)
{
    // return PosixConfig::ReadConfigValue(key, val);
    // val = 1234;
    // return CHIP_NO_ERROR;
    return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    ///*MJ-merge*/return TalariaConfig::ReadConfigValue(key, val);
    ///*MJ-merge*/return CHIP_NO_ERROR;
}
///*MJ-merge*/#endif

CHIP_ERROR ConfigurationManagerImpl::ReadConfigValue(Key key, uint32_t & val)
{
    return TalariaConfig::ReadConfigValue(key, val);
}

CHIP_ERROR ConfigurationManagerImpl::ReadConfigValue(Key key, uint64_t & val)
{
    // return PosixConfig::ReadConfigValue(key, val);
    // return CHIP_NO_ERROR;
    return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    ///*MJ-merge*/return TalariaConfig::ReadConfigValue(key, val);
    ///*MJ-merge*/return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::ReadConfigValueStr(Key key, char * buf, size_t bufSize, size_t & outLen)
{
    // return PosixConfig::ReadConfigValueStr(key, buf, bufSize, outLen);
    // return CHIP_NO_ERROR;
    return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    ///*MJ-merge*/return TalariaConfig::ReadConfigValueStr(key, buf, bufSize, outLen);
    ///*MJ-merge*/return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::ReadConfigValueBin(Key key, uint8_t * buf, size_t bufSize, size_t & outLen)
{
    // return PosixConfig::ReadConfigValueBin(key, buf, bufSize, outLen);
    // return CHIP_NO_ERROR;
    return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    ///*MJ-merge*/return TalariaConfig::ReadConfigValueBin(key, buf, bufSize, outLen);
    ///*MJ-merge*/return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValue(Key key, bool val)
{
    // return PosixConfig::WriteConfigValue(key, val);
    ///*MJ-merge*/return TalariaConfig::WriteConfigValue(key, val);
    return CHIP_NO_ERROR;
}

//#if 0 /*MJ-merge*/
CHIP_ERROR ConfigurationManagerImpl::WriteConfigValue(Key key, uint16_t val)
{
    // return PosixConfig::WriteConfigValue(key, val);
    return CHIP_NO_ERROR;
}
//#endif

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValue(Key key, uint32_t val)
{
    // return PosixConfig::WriteConfigValue(key, val);
    ///*MJ-merge*/return TalariaConfig::WriteConfigValue(key, val);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValue(Key key, uint64_t val)
{
    // return PosixConfig::WriteConfigValue(key, val);
    ///*MJ-merge*/return TalariaConfig::WriteConfigValue(key, val);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValueStr(Key key, const char * str)
{
    // return PosixConfig::WriteConfigValueStr(key, str);
    ///*MJ-merge*/return TalariaConfig::WriteConfigValueStr(key, str);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValueStr(Key key, const char * str, size_t strLen)
{
    // return PosixConfig::WriteConfigValueStr(key, str, strLen);
    ///*MJ-merge*/return TalariaConfig::WriteConfigValueStr(key, str, strLen);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValueBin(Key key, const uint8_t * data, size_t dataLen)
{
    // return PosixConfig::WriteConfigValueBin(key, data, dataLen);
    ///*MJ-merge*/return TalariaConfig::WriteConfigValueBin(key, data, dataLen);
    return CHIP_NO_ERROR;
}

void ConfigurationManagerImpl::RunConfigUnitTest()
{
    // PosixConfig::RunConfigUnitTest();
    return CHIP_NO_ERROR;
}

void ConfigurationManagerImpl::DoFactoryReset(intptr_t arg)
{
    CHIP_ERROR err;
    
    err = PersistedStorage::KeyValueStoreMgrImpl().EraseAll();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Clear Key-Value Storage failed");
    }
    return;
}

CHIP_ERROR ConfigurationManagerImpl::StoreVendorId(uint16_t vendorId)
{
    // return WriteConfigValue(PosixConfig::kConfigKey_VendorId, vendorId);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::StoreProductId(uint16_t productId)
{
    // return WriteConfigValue(PosixConfig::kConfigKey_ProductId, productId);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::GetRebootCount(uint32_t & rebootCount)
{
    // return ReadConfigValue(PosixConfig::kCounterKey_RebootCount, rebootCount);
    // /*MJ-merge*/return ReadConfigValue(TalariaConfig::kCounterKey_RebootCount, rebootCount);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::StoreRebootCount(uint32_t rebootCount)
{
    // return WriteConfigValue(PosixConfig::kCounterKey_RebootCount, rebootCount);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::GetTotalOperationalHours(uint32_t & totalOperationalHours)
{
    // return ReadConfigValue(PosixConfig::kCounterKey_TotalOperationalHours, totalOperationalHours);
    // /*MJ-merge*/ return ReadConfigValue(TalariaConfig::kCounterKey_TotalOperationalHours, totalOperationalHours);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::StoreTotalOperationalHours(uint32_t totalOperationalHours)
{
    // return WriteConfigValue(PosixConfig::kCounterKey_TotalOperationalHours, totalOperationalHours);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::GetBootReason(uint32_t & bootReason)
{
    // return ReadConfigValue(PosixConfig::kCounterKey_BootReason, bootReason);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::StoreBootReason(uint32_t bootReason)
{
    // return WriteConfigValue(PosixConfig::kCounterKey_BootReason, bootReason);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::GetRegulatoryLocation(uint8_t & location)
{
    location = 0;

    // if (CHIP_NO_ERROR != ReadConfigValue(PosixConfig::kConfigKey_RegulatoryLocation, value))
    // {
    //     ReturnErrorOnFailure(GetLocationCapability(location));

    //     if (CHIP_NO_ERROR != StoreRegulatoryLocation(location))
    //     {
    //         ChipLogError(DeviceLayer, "Failed to store RegulatoryLocation");
    //     }
    // }
    // else
    // {
    //     location = static_cast<uint8_t>(value);
    // }

    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigurationManagerImpl::GetLocationCapability(uint8_t & location)
{
    location = 0;

    // CHIP_ERROR err = ReadConfigValue(PosixConfig::kConfigKey_LocationCapability, value);

    // if (err == CHIP_NO_ERROR)
    // {
    //     VerifyOrReturnError(value <= UINT8_MAX, CHIP_ERROR_INVALID_INTEGER_VALUE);
    //     location = static_cast<uint8_t>(value);
    // }

    // return err;
    return CHIP_NO_ERROR;
}

ConfigurationManager & ConfigurationMgrImpl()
{
    return ConfigurationManagerImpl::GetDefaultInstance();
}

} // namespace DeviceLayer
} // namespace chip
