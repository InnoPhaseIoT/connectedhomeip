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

#ifdef __cplusplus
extern "C" {
#include <kernel/hwaddr.h>
}
#endif /* __cplusplus */

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
    uint32_t reboot_count;
    uint32_t tmp;
    err = Internal::GenericConfigurationManagerImpl<TalariaConfig>::Init();

    /* Update the Reboot Count value */
    if (GetRebootCount(reboot_count) == CHIP_NO_ERROR) {
        StoreRebootCount(reboot_count + 1);
    } else {
        reboot_count = 0;
        StoreRebootCount(reboot_count + 1);
    }
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
    os_hwaddr_get(HWADDR_SCOPE_WIFI, buf);
    return CHIP_NO_ERROR;
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
    TalariaConfig::Key configKey{ TalariaConfig::kConfigNamespace_ChipCounters, key };
    return TalariaConfig::ReadConfigValue(configKey, value);
}

CHIP_ERROR ConfigurationManagerImpl::WritePersistedStorageValue(::chip::Platform::PersistedStorage::Key key, uint32_t value)
{
    TalariaConfig::Key configKey{ TalariaConfig::kConfigNamespace_ChipCounters, key};
    return TalariaConfig::WriteConfigValue(configKey, value);
}

CHIP_ERROR ConfigurationManagerImpl::ReadConfigValue(Key key, bool & val)
{
    return TalariaConfig::ReadConfigValue(key, val);
}

CHIP_ERROR ConfigurationManagerImpl::ReadConfigValue(Key key, uint16_t & val)
{
    return TalariaConfig::ReadConfigValue(key, val);
}

CHIP_ERROR ConfigurationManagerImpl::ReadConfigValue(Key key, uint32_t & val)
{
    return TalariaConfig::ReadConfigValue(key, val);
}

CHIP_ERROR ConfigurationManagerImpl::ReadConfigValue(Key key, uint64_t & val)
{
    return TalariaConfig::ReadConfigValue(key, val);
}

CHIP_ERROR ConfigurationManagerImpl::ReadConfigValueStr(Key key, char * buf, size_t bufSize, size_t & outLen)
{
    return TalariaConfig::ReadConfigValueStr(key, buf, bufSize, outLen);
}

CHIP_ERROR ConfigurationManagerImpl::ReadConfigValueBin(Key key, uint8_t * buf, size_t bufSize, size_t & outLen)
{
    return TalariaConfig::ReadConfigValueBin(key, buf, bufSize, outLen);
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValue(Key key, bool val)
{
    return TalariaConfig::WriteConfigValue(key, val);
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValue(Key key, uint16_t val)
{
    return TalariaConfig::WriteConfigValue(key, val);
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValue(Key key, uint32_t val)
{
    return TalariaConfig::WriteConfigValue(key, val);
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValue(Key key, uint64_t val)
{
    return TalariaConfig::WriteConfigValue(key, val);
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValueStr(Key key, const char * str)
{
    return TalariaConfig::WriteConfigValueStr(key, str);
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValueStr(Key key, const char * str, size_t strLen)
{
    return TalariaConfig::WriteConfigValueStr(key, str, strLen);
}

CHIP_ERROR ConfigurationManagerImpl::WriteConfigValueBin(Key key, const uint8_t * data, size_t dataLen)
{
    return TalariaConfig::WriteConfigValueBin(key, data, dataLen);
}

void ConfigurationManagerImpl::RunConfigUnitTest()
{
    // PosixConfig::RunConfigUnitTest();
    //return CHIP_NO_ERROR;
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
    return WriteConfigValue(TalariaConfig::kConfigKey_VendorId, vendorId);
}

CHIP_ERROR ConfigurationManagerImpl::StoreProductId(uint16_t productId)
{
    return WriteConfigValue(TalariaConfig::kConfigKey_ProductId, productId);
}

CHIP_ERROR ConfigurationManagerImpl::GetRebootCount(uint32_t & rebootCount)
{
    return ReadConfigValue(TalariaConfig::kCounterKey_RebootCount, rebootCount);
}

CHIP_ERROR ConfigurationManagerImpl::StoreRebootCount(uint32_t rebootCount)
{
    return WriteConfigValue(TalariaConfig::kCounterKey_RebootCount, rebootCount);
}

CHIP_ERROR ConfigurationManagerImpl::GetTotalOperationalHours(uint32_t & totalOperationalHours)
{
    return ReadConfigValue(TalariaConfig::kCounterKey_TotalOperationalHours, totalOperationalHours);
}

CHIP_ERROR ConfigurationManagerImpl::StoreTotalOperationalHours(uint32_t totalOperationalHours)
{
    return WriteConfigValue(TalariaConfig::kCounterKey_TotalOperationalHours, totalOperationalHours);
}

CHIP_ERROR ConfigurationManagerImpl::GetBootReason(uint32_t & bootReason)
{
    return ReadConfigValue(TalariaConfig::kCounterKey_BootReason, bootReason);
}

CHIP_ERROR ConfigurationManagerImpl::StoreBootReason(uint32_t bootReason)
{
    return WriteConfigValue(TalariaConfig::kCounterKey_BootReason, bootReason);
}

CHIP_ERROR ConfigurationManagerImpl::GetRegulatoryLocation(uint8_t & location)
{
    return TalariaConfig::ReadConfigValue(TalariaConfig::kConfigKey_RegulatoryLocation, location);
}

CHIP_ERROR ConfigurationManagerImpl::GetLocationCapability(uint8_t & location)
{

    return TalariaConfig::ReadConfigValue(TalariaConfig::kConfigKey_LocationCapability, location);
}

ConfigurationManager & ConfigurationMgrImpl()
{
    return ConfigurationManagerImpl::GetDefaultInstance();
}

} // namespace DeviceLayer
} // namespace chip
