/*
 *
 *    Copyright (c) 2020-2022 Project CHIP Authors
 *    Copyright (c) 2019-2020 Google LLC.
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
 *          Utilities for interacting with the the ESP32 "NVS" key-value store.
 */
/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <platform/talaria/Config.h>

#include <lib/core/CHIPEncoding.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/CHIPMemString.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
// #include <platform/ESP32/ESP32Utils.h>
// #include <platform/ESP32/ScopedNvsHandle.h>

// #include "nvs.h"
// #include "nvs_flash.h"

namespace chip {
namespace DeviceLayer {
namespace Internal {

// *** CAUTION ***: Changing the names or namespaces of these values will *break* existing devices.

// NVS namespaces used to store device configuration information.
const char TalariaConfig::kConfigNamespace_ChipFactory[]  = "chip-factory";
const char TalariaConfig::kConfigNamespace_ChipConfig[]   = "chip-config";
const char TalariaConfig::kConfigNamespace_ChipCounters[] = "chip-counters";

// Keys stored in the chip-factory namespace
const TalariaConfig::Key TalariaConfig::kConfigKey_SerialNum             = { kConfigNamespace_ChipFactory, "serial-num" };
const TalariaConfig::Key TalariaConfig::kConfigKey_MfrDeviceId           = { kConfigNamespace_ChipFactory, "device-id" };
const TalariaConfig::Key TalariaConfig::kConfigKey_MfrDeviceCert         = { kConfigNamespace_ChipFactory, "device-cert" };
const TalariaConfig::Key TalariaConfig::kConfigKey_MfrDeviceICACerts     = { kConfigNamespace_ChipFactory, "device-ca-certs" };
const TalariaConfig::Key TalariaConfig::kConfigKey_MfrDevicePrivateKey   = { kConfigNamespace_ChipFactory, "device-key" };
const TalariaConfig::Key TalariaConfig::kConfigKey_HardwareVersion       = { kConfigNamespace_ChipFactory, "hardware-ver" };
const TalariaConfig::Key TalariaConfig::kConfigKey_HardwareVersionString = { kConfigNamespace_ChipFactory, "hw-ver-str" };
const TalariaConfig::Key TalariaConfig::kConfigKey_ManufacturingDate     = { kConfigNamespace_ChipFactory, "mfg-date" };
const TalariaConfig::Key TalariaConfig::kConfigKey_SetupPinCode          = { kConfigNamespace_ChipFactory, "pin-code" };
const TalariaConfig::Key TalariaConfig::kConfigKey_SetupDiscriminator    = { kConfigNamespace_ChipFactory, "discriminator" };
const TalariaConfig::Key TalariaConfig::kConfigKey_Spake2pIterationCount = { kConfigNamespace_ChipFactory, "iteration-count" };
const TalariaConfig::Key TalariaConfig::kConfigKey_Spake2pSalt           = { kConfigNamespace_ChipFactory, "salt" };
const TalariaConfig::Key TalariaConfig::kConfigKey_Spake2pVerifier       = { kConfigNamespace_ChipFactory, "verifier" };
const TalariaConfig::Key TalariaConfig::kConfigKey_DACCert               = { kConfigNamespace_ChipFactory, "dac-cert" };
const TalariaConfig::Key TalariaConfig::kConfigKey_DACPrivateKey         = { kConfigNamespace_ChipFactory, "dac-key" };
const TalariaConfig::Key TalariaConfig::kConfigKey_DACPublicKey          = { kConfigNamespace_ChipFactory, "dac-pub-key" };
const TalariaConfig::Key TalariaConfig::kConfigKey_PAICert               = { kConfigNamespace_ChipFactory, "pai-cert" };
const TalariaConfig::Key TalariaConfig::kConfigKey_CertDeclaration       = { kConfigNamespace_ChipFactory, "cert-dclrn" };
const TalariaConfig::Key TalariaConfig::kConfigKey_VendorId              = { kConfigNamespace_ChipFactory, "vendor-id" };
const TalariaConfig::Key TalariaConfig::kConfigKey_VendorName            = { kConfigNamespace_ChipFactory, "vendor-name" };
const TalariaConfig::Key TalariaConfig::kConfigKey_ProductId             = { kConfigNamespace_ChipFactory, "product-id" };
const TalariaConfig::Key TalariaConfig::kConfigKey_ProductName           = { kConfigNamespace_ChipFactory, "product-name" };
const TalariaConfig::Key TalariaConfig::kConfigKey_ProductLabel          = { kConfigNamespace_ChipFactory, "product-label" };
const TalariaConfig::Key TalariaConfig::kConfigKey_ProductURL            = { kConfigNamespace_ChipFactory, "product-url" };
const TalariaConfig::Key TalariaConfig::kConfigKey_SupportedCalTypes     = { kConfigNamespace_ChipFactory, "cal-types" };
const TalariaConfig::Key TalariaConfig::kConfigKey_SupportedLocaleSize   = { kConfigNamespace_ChipFactory, "locale-sz" };
const TalariaConfig::Key TalariaConfig::kConfigKey_RotatingDevIdUniqueId = { kConfigNamespace_ChipFactory, "rd-id-uid" };
const TalariaConfig::Key TalariaConfig::kConfigKey_LocationCapability    = { kConfigNamespace_ChipFactory, "loc-capability" };

// Keys stored in the chip-config namespace
const TalariaConfig::Key TalariaConfig::kConfigKey_ServiceConfig      = { kConfigNamespace_ChipConfig, "service-config" };
const TalariaConfig::Key TalariaConfig::kConfigKey_PairedAccountId    = { kConfigNamespace_ChipConfig, "account-id" };
const TalariaConfig::Key TalariaConfig::kConfigKey_ServiceId          = { kConfigNamespace_ChipConfig, "service-id" };
const TalariaConfig::Key TalariaConfig::kConfigKey_LastUsedEpochKeyId = { kConfigNamespace_ChipConfig, "last-ek-id" };
const TalariaConfig::Key TalariaConfig::kConfigKey_FailSafeArmed      = { kConfigNamespace_ChipConfig, "fail-safe-armed" };
const TalariaConfig::Key TalariaConfig::kConfigKey_WiFiStationSecType = { kConfigNamespace_ChipConfig, "sta-sec-type" };
const TalariaConfig::Key TalariaConfig::kConfigKey_RegulatoryLocation = { kConfigNamespace_ChipConfig, "reg-location" };
const TalariaConfig::Key TalariaConfig::kConfigKey_CountryCode        = { kConfigNamespace_ChipConfig, "country-code" };
const TalariaConfig::Key TalariaConfig::kConfigKey_UniqueId           = { kConfigNamespace_ChipConfig, "unique-id" };
const TalariaConfig::Key TalariaConfig::kConfigKey_LockUser           = { kConfigNamespace_ChipConfig, "lock-user" };
const TalariaConfig::Key TalariaConfig::kConfigKey_Credential         = { kConfigNamespace_ChipConfig, "credential" };
const TalariaConfig::Key TalariaConfig::kConfigKey_LockUserName       = { kConfigNamespace_ChipConfig, "lock-user-name" };
const TalariaConfig::Key TalariaConfig::kConfigKey_CredentialData     = { kConfigNamespace_ChipConfig, "credential-data" };
const TalariaConfig::Key TalariaConfig::kConfigKey_UserCredentials    = { kConfigNamespace_ChipConfig, "user-credential" };
const TalariaConfig::Key TalariaConfig::kConfigKey_WeekDaySchedules   = { kConfigNamespace_ChipConfig, "week-day-sched" };
const TalariaConfig::Key TalariaConfig::kConfigKey_YearDaySchedules   = { kConfigNamespace_ChipConfig, "year-day-sched" };
const TalariaConfig::Key TalariaConfig::kConfigKey_HolidaySchedules   = { kConfigNamespace_ChipConfig, "holiday-sched" };

// Keys stored in the Chip-counters namespace
const TalariaConfig::Key TalariaConfig::kCounterKey_RebootCount           = { kConfigNamespace_ChipCounters, "reboot-count" };
const TalariaConfig::Key TalariaConfig::kCounterKey_UpTime                = { kConfigNamespace_ChipCounters, "up-time" };
const TalariaConfig::Key TalariaConfig::kCounterKey_TotalOperationalHours = { kConfigNamespace_ChipCounters, "total-hours" };
#if 0
const char * TalariaConfig::GetPartitionLabelByNamespace(const char * ns)
{
    if (strcmp(ns, kConfigNamespace_ChipFactory) == 0)
    {
        return CHIP_DEVICE_CONFIG_CHIP_FACTORY_NAMESPACE_PARTITION;
    }
    else if (strcmp(ns, kConfigNamespace_ChipConfig) == 0)
    {
        return CHIP_DEVICE_CONFIG_CHIP_CONFIG_NAMESPACE_PARTITION;
    }
    else if (strcmp(ns, kConfigNamespace_ChipCounters) == 0)
    {
        return CHIP_DEVICE_CONFIG_CHIP_COUNTERS_NAMESPACE_PARTITION;
    }

    return NVS_DEFAULT_PART_NAME;
}
#endif

CHIP_ERROR TalariaConfig::ReadConfigValue(Key key, bool & val)
{
    val = true;

    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::ReadConfigValue(Key key, uint32_t & val)
{
    val = 1234; // 0x4D2

    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::ReadConfigValue(Key key, uint64_t & val)
{
    val = 2345; // 0x929

    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::ReadConfigValueStr(Key key, char * buf, size_t bufSize, size_t & outLen)
{
    memcpy(buf, bufSize, "A");
    outLen = bufSize;

    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::ReadConfigValueBin(Key key, uint8_t * buf, size_t bufSize, size_t & outLen)
{
    memcpy(buf, bufSize, 0x01);
    outLen = bufSize;

    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::WriteConfigValue(Key key, bool val)
{
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::WriteConfigValue(Key key, uint32_t val)
{
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::WriteConfigValue(Key key, uint64_t val)
{
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::WriteConfigValueStr(Key key, const char * str)
{
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::WriteConfigValueStr(Key key, const char * str, size_t strLen)
{
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::WriteConfigValueBin(Key key, const uint8_t * data, size_t dataLen)
{
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::ClearConfigValue(Key key)
{
    return CHIP_NO_ERROR;
}

bool TalariaConfig::ConfigValueExists(Key key)
{
    return true;
}

CHIP_ERROR TalariaConfig::EnsureNamespace(const char * ns)
{
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::ClearNamespace(const char * ns)
{
    return CHIP_NO_ERROR;
}

// void TalariaConfig::RunConfigUnitTest() {}

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip
