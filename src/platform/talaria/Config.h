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
 *          Utilities for interacting with the the Talaria KVS File system
 */

#pragma once

#include <string.h>

namespace chip {
namespace DeviceLayer {
namespace Internal {

/**
 * Provides functions and definitions for accessing device configuration information.
 */
class TalariaConfig
{
public:
    struct Key;

    static constexpr size_t kMaxConfigKeyNameLength = 64;

    static const char kConfigNamespace_ChipFactory[];
    static const char kConfigNamespace_ChipConfig[];
    static const char kConfigNamespace_ChipCounters[];
    static const char kConfigNamespace_KVS[];

    static const Key kConfigKey_SerialNum;
    static const Key kConfigKey_MfrDeviceId;
    static const Key kConfigKey_MfrDeviceCert;
    static const Key kConfigKey_MfrDeviceICACerts;
    static const Key kConfigKey_MfrDevicePrivateKey;
    static const Key kConfigKey_HardwareVersion;
    static const Key kConfigKey_HardwareVersionString;
    static const Key kConfigKey_ManufacturingDate;
    static const Key kConfigKey_SetupPinCode;
    static const Key kConfigKey_SetupDiscriminator;
    static const Key kConfigKey_Spake2pIterationCount;
    static const Key kConfigKey_Spake2pSalt;
    static const Key kConfigKey_Spake2pVerifier;
    static const Key kConfigKey_DACCert;
    static const Key kConfigKey_DACPrivateKey;
    static const Key kConfigKey_DACPublicKey;
    static const Key kConfigKey_PAICert;
    static const Key kConfigKey_CertDeclaration;
    static const Key kConfigKey_VendorId;
    static const Key kConfigKey_VendorName;
    static const Key kConfigKey_ProductId;
    static const Key kConfigKey_ProductName;
    static const Key kConfigKey_ProductLabel;
    static const Key kConfigKey_ProductURL;
    static const Key kConfigKey_SupportedCalTypes;
    static const Key kConfigKey_SupportedLocaleSize;
    static const Key kConfigKey_RotatingDevIdUniqueId;
    static const Key kConfigKey_LocationCapability;

    static const Key kConfigKey_ServiceConfig;
    static const Key kConfigKey_PairedAccountId;
    static const Key kConfigKey_ServiceId;
    static const Key kConfigKey_LastUsedEpochKeyId;
    static const Key kConfigKey_FailSafeArmed;
    static const Key kConfigKey_RegulatoryLocation;
    static const Key kConfigKey_CountryCode;
    static const Key kConfigKey_UniqueId;
    static const Key kConfigKey_SoftwareVersion;
    static const Key kConfigKey_SoftwareVersionStr;

    // CHIP Counter keys
    static const Key kCounterKey_RebootCount;
    static const Key kCounterKey_UpTime;
    static const Key kCounterKey_TotalOperationalHours;
    static const Key kCounterKey_BootReason;

    // Config value accessors.
    static CHIP_ERROR ReadConfigValue(Key key, bool & val);
    static CHIP_ERROR ReadConfigValue(Key key, uint8_t & val);
    static CHIP_ERROR ReadConfigValue(Key key, uint16_t & val);
    static CHIP_ERROR ReadConfigValue(Key key, uint32_t & val);
    static CHIP_ERROR ReadConfigValue(Key key, uint64_t & val);

    // If buf is NULL then outLen is set to the required length to fit the string/blob
    static CHIP_ERROR ReadConfigValueStr(Key key, char * buf, size_t bufSize, size_t & outLen);
    static CHIP_ERROR ReadConfigValueBin(Key key, uint8_t * buf, size_t bufSize, size_t & outLen);

    static CHIP_ERROR WriteConfigValue(Key key, bool val);
    static CHIP_ERROR WriteConfigValue(Key key, uint16_t val);
    static CHIP_ERROR WriteConfigValue(Key key, uint32_t val);
    static CHIP_ERROR WriteConfigValue(Key key, uint64_t val);
    static CHIP_ERROR WriteConfigValueStr(Key key, const char * str);
    static CHIP_ERROR WriteConfigValueStr(Key key, const char * str, size_t strLen);
    static CHIP_ERROR WriteConfigValueBin(Key key, const uint8_t * data, size_t dataLen);
    static CHIP_ERROR ClearConfigValue(Key key);
    static bool ConfigValueExists(Key key);
    static void FactoryDefaultConfigCotunters();
    static uint8_t *GetAppCipherKey();

    // static void RunConfigUnitTest(void);

private:
    static CHIP_ERROR ReadFromFS(Key key, char ** read_data, int *read_len);
    static CHIP_ERROR WriteToFS(Key key, const void * data, size_t data_len);
    static CHIP_ERROR ClearConfigFromFS(Key key);
    static bool ConfigExistsInFS(Key key);

};

struct TalariaConfig::Key
{
    const char * Namespace;
    const char * Name;

    bool operator==(const Key & other) const;

    template <typename T, typename std::enable_if_t<std::is_convertible<T, const char *>::value, int> = 0>
    Key(const char * aNamespace, T aName) : Namespace(aNamespace), Name(aName)
    {}

    template <size_t N>
    Key(const char * aNamespace, const char (&aName)[N]) : Namespace(aNamespace), Name(aName)
    {
        // Note: N includes null-terminator.
        static_assert(N <= TalariaConfig::kMaxConfigKeyNameLength + 1, "Key too long");
    }
};

inline bool TalariaConfig::Key::operator==(const Key & other) const
{
    return strcmp(Namespace, other.Namespace) == 0 && strcmp(Name, other.Name) == 0;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip
