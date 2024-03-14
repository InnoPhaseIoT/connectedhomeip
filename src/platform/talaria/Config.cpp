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
 *          Utilities for interacting with the the Talaria KVS file system
 */
/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <platform/talaria/Config.h>

#include <lib/core/CHIPEncoding.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/CHIPMemString.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/talaria/TalariaUtils.h>

#ifdef __cplusplus
extern "C" {
#include <sys/stat.h>
#include <unistd.h>
#include <fs_utils.h>
}
#endif /* _cplusplus */

#define DATA_PARTITION_PATH "/data"

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
const TalariaConfig::Key TalariaConfig::kConfigKey_RegulatoryLocation = { kConfigNamespace_ChipConfig, "reg-location" };
const TalariaConfig::Key TalariaConfig::kConfigKey_CountryCode        = { kConfigNamespace_ChipConfig, "country-code" };
const TalariaConfig::Key TalariaConfig::kConfigKey_UniqueId           = { kConfigNamespace_ChipConfig, "unique-id" };

// Keys stored in the Chip-counters namespace
const TalariaConfig::Key TalariaConfig::kCounterKey_RebootCount           = { kConfigNamespace_ChipCounters, "reboot-count" };
const TalariaConfig::Key TalariaConfig::kCounterKey_UpTime                = { kConfigNamespace_ChipCounters, "up-time" };
const TalariaConfig::Key TalariaConfig::kCounterKey_TotalOperationalHours = { kConfigNamespace_ChipCounters, "total-hours" };
const TalariaConfig::Key TalariaConfig::kCounterKey_BootReason            = { kConfigNamespace_ChipCounters, "boot-reason" };


CHIP_ERROR TalariaConfig::ReadFromFS(Key key, char ** read_data, int *read_len)
{
    char path[100];
    sprintf(path, "%s/%s/%s", DATA_PARTITION_PATH, key.Namespace, key.Name);
    ChipLogDetail(DeviceLayer, "Reading path: %s", path);
    if (utils_is_file_present(path) == 0) {
        return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }

    *read_data = utils_file_get(path, read_len);
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::WriteToFS(Key key, const void * data, size_t data_len)
{
    char path[100];
    int ret;
    struct stat st = {0};

    /* If the namespace directory isn't created then create */
    sprintf(path, "%s/%s", DATA_PARTITION_PATH, key.Namespace);
    if (stat(path, &st) == -1) {
        mkdir(path, 0666);
    }

    sprintf(path, "%s/%s/%s", DATA_PARTITION_PATH, key.Namespace, key.Name);
    ChipLogDetail(DeviceLayer, "Writing path: %s", path, data_len);

    ret = utils_file_store(path, data, data_len);
    if (ret < 0) {
        ChipLogError(DeviceLayer, "Error writing to FS");
        return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::ClearConfigFromFS(Key key)
{
    char path[100];
    int ret;
    FILE *fp;
    sprintf(path, "%s/%s/%s", DATA_PARTITION_PATH, key.Namespace, key.Name);
    ChipLogDetail(DeviceLayer, "Erasing path: %s", path);

    if (utils_is_file_present(path) != 0)
	{
		ret = unlink(path);
	}
    return CHIP_NO_ERROR;
}

bool TalariaConfig::ConfigExistsInFS(Key key)
{
    char path[100];
    int ret;
    FILE *fp;
    sprintf(path, "%s/%s/%s", DATA_PARTITION_PATH, key.Namespace, key.Name);
    ChipLogDetail(DeviceLayer, "Check path: %s exists?", path);

    if (utils_is_file_present(path) == 0) {
        return false;
    }
    return true;
}

CHIP_ERROR TalariaConfig::ReadConfigValue(Key key, bool & val)
{
    char *read_data;
    int read_len;
    if ((ReadFromFS(key, &read_data, &read_len) ==
            CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND) ||
            (read_len <= 0)) {
        return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }

    val = atoi(read_data)? true: false;
    free(read_data);

    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::ReadConfigValue(Key key, uint8_t & val)
{
    char *read_data;
    int read_len;
    if ((ReadFromFS(key, &read_data, &read_len) ==
            CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND) ||
            (read_len <= 0)) {
        return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    val = atoi(read_data);
    free(read_data);

    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::ReadConfigValue(Key key, uint16_t & val)
{
    char *read_data;
    int read_len;
    if ((ReadFromFS(key, &read_data, &read_len) ==
            CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND) ||
            (read_len <= 0)) {
        return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    val = atoi(read_data);
    free(read_data);

    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::ReadConfigValue(Key key, uint32_t & val)
{
    char *read_data;
    int read_len;
    if ((ReadFromFS(key, &read_data, &read_len) ==
            CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND) ||
            (read_len <= 0)) {
        return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    val = atoi(read_data);
    free(read_data);

    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::ReadConfigValue(Key key, uint64_t & val)
{
    char *read_data;
    int read_len;
    if ((ReadFromFS(key, &read_data, &read_len) ==
            CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND) ||
            (read_len <= 0)) {
        return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }

    val = atol(read_data);
    free(read_data);
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::ReadConfigValueStr(Key key, char * buf, size_t bufSize, size_t & outLen)
{
    char *read_data;
    int read_len;
    if ((ReadFromFS(key, &read_data, &read_len) ==
            CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND) ||
            (read_len <= 0)) {
        return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }

    memcpy(buf, read_data, std::min(read_len, (int) bufSize));
    free(read_data);
    outLen = std::min(read_len, (int) bufSize);

    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::ReadConfigValueBin(Key key, uint8_t * buf, size_t bufSize, size_t & outLen)
{
    char *read_data;
    int read_len;
    if ((ReadFromFS(key, &read_data, &read_len) ==
            CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND) ||
            (read_len <= 0)) {
        return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }

    /* For binary data remove EOF from the end */
    memcpy(buf, read_data, std::min(read_len - 1, (int) bufSize));
    free(read_data);
    outLen = std::min(read_len - 1, (int) bufSize);

    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaConfig::WriteConfigValue(Key key, bool val)
{
    char str[16];
    sprintf(str, "%d", val);
    return WriteToFS(key, str, strlen(str));
}

CHIP_ERROR TalariaConfig::WriteConfigValue(Key key, uint16_t val)
{
    char str[16];
    sprintf(str, "%u", val);
    return WriteToFS(key, str, strlen(str));
}

CHIP_ERROR TalariaConfig::WriteConfigValue(Key key, uint32_t val)
{
    char str[16];
    sprintf(str, "%u", val);
    return WriteToFS(key, str, strlen(str));
}

CHIP_ERROR TalariaConfig::WriteConfigValue(Key key, uint64_t val)
{
    char str[32];
    sprintf(str, "%llu", val);
    return WriteToFS(key, str, strlen(str));
}

CHIP_ERROR TalariaConfig::WriteConfigValueStr(Key key, const char * str)
{
    return WriteToFS(key, str, strlen(str));
}

CHIP_ERROR TalariaConfig::WriteConfigValueStr(Key key, const char * str, size_t strLen)
{
    return WriteToFS(key, str, strLen);
}

CHIP_ERROR TalariaConfig::WriteConfigValueBin(Key key, const uint8_t * data, size_t dataLen)
{
    return WriteToFS(key, data, dataLen);
}

CHIP_ERROR TalariaConfig::ClearConfigValue(Key key)
{
    return ClearConfigFromFS(key);
}

bool TalariaConfig::ConfigValueExists(Key key)
{
    return ConfigExistsInFS(key);
}

void TalariaConfig::FactoryDefaultConfigCotunters()
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    /* Set of Config and Counters to be removed at the time of Setting Factory Defaults */
    const Key *clear_key_set[] = { &kConfigKey_ServiceConfig, &kConfigKey_PairedAccountId, &kConfigKey_ServiceId,
                                  &kConfigKey_LastUsedEpochKeyId, &kConfigKey_FailSafeArmed, &kConfigKey_RegulatoryLocation,
                                  &kConfigKey_CountryCode, &kConfigKey_UniqueId, &kCounterKey_UpTime,
                                  &kCounterKey_TotalOperationalHours, &kCounterKey_BootReason };
    for (int i = 0; i < sizeof(clear_key_set) / sizeof(clear_key_set[0]); i++) {
        err = ClearConfigValue(*clear_key_set[i]);
        if (err != CHIP_NO_ERROR) {
            ChipLogDetail(DeviceLayer, "Failed to clear %s", clear_key_set[i].Name);
        }
    }

    err = WriteConfigValue(kCounterKey_RebootCount, (uint16_t) 0);
    if (err != CHIP_NO_ERROR) {
        ChipLogDetail(DeviceLayer, "Failed to reset %s", kCounterKey_RebootCount.Name);
    }
}

// void TalariaConfig::RunConfigUnitTest() {}

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip
