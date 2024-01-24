/*
 *
 *    Copyright (c) 2021-2022 Project CHIP Authors
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
 *          Provides an implementation of the DiagnosticDataProvider object
 *          for ESP32 platform.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include <talaria_two.h>
#include <wifi/wifi.h>
#include <wifi/akm_suite.h>

#ifdef __cplusplus
}
#endif

#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <app-common/zap-generated/enums.h>
#include <crypto/CHIPCryptoPAL.h>
#include <lib/support/CHIPMemString.h>
#include <platform/DiagnosticDataProvider.h>
#include <platform/talaria/DiagnosticDataProviderImpl.h>
#include <platform/talaria/TalariaUtils.h>
#include <lib/support/logging/CHIPLogging.h>

#define os_avail_heap xPortGetFreeHeapSize

using namespace ::chip;
using namespace ::chip::TLV;
using namespace ::chip::DeviceLayer;
using namespace ::chip::DeviceLayer::Internal;
using namespace ::chip::app::Clusters::GeneralDiagnostics;

namespace {

InterfaceTypeEnum GetInterfaceType(const char * if_desc)
{
    if (strncmp(if_desc, "ap", strnlen(if_desc, 2)) == 0 || strncmp(if_desc, "sta", strnlen(if_desc, 3)) == 0)
        return InterfaceTypeEnum::kWiFi;
    return InterfaceTypeEnum::kUnspecified;
}

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI || 1
#if 0 /* Not implemented AP mode hence not declaring */
app::Clusters::WiFiNetworkDiagnostics::SecurityTypeEnum MapAuthModeToSecurityType(wifi_auth_mode_t authmode)
{
    using app::Clusters::WiFiNetworkDiagnostics::SecurityTypeEnum;
    switch (authmode)
    {
    case WIFI_AUTH_OPEN:
        return SecurityTypeEnum::kNone;
    case WIFI_AUTH_WEP:
        return SecurityTypeEnum::kWep;
    case WIFI_AUTH_WPA_PSK:
        return SecurityTypeEnum::kWpa;
    case WIFI_AUTH_WPA2_PSK:
        return SecurityTypeEnum::kWpa2;
    case WIFI_AUTH_WPA3_PSK:
        return SecurityTypeEnum::kWpa3;
    default:
        return SecurityTypeEnum::kUnspecified;
    }
}

app::Clusters::WiFiNetworkDiagnostics::WiFiVersionEnum GetWiFiVersionFromAPRecord(wifi_ap_record_t ap_info)
{
    using app::Clusters::WiFiNetworkDiagnostics::WiFiVersionEnum;
    if (ap_info.phy_11n)
        return WiFiVersionEnum::kN;
    else if (ap_info.phy_11g)
        return WiFiVersionEnum::kG;
    else if (ap_info.phy_11b)
        return WiFiVersionEnum::kB;
    else
        return WiFiVersionEnum::kUnknownEnumValue;
}
#endif
#endif // CHIP_DEVICE_CONFIG_ENABLE_WIFI

} // namespace

namespace chip {
namespace DeviceLayer {

DiagnosticDataProviderImpl & DiagnosticDataProviderImpl::GetDefaultInstance()
{
    static DiagnosticDataProviderImpl sInstance;
    return sInstance;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetCurrentHeapFree(uint64_t & currentHeapFree)
{
    currentHeapFree = xPortGetFreeHeapSize();
    return CHIP_NO_ERROR;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetCurrentHeapUsed(uint64_t & currentHeapUsed)
{
    /* TODO: Yet to implement for Talaria
    currentHeapUsed = heap_caps_get_total_size(MALLOC_CAP_DEFAULT) - esp_get_free_heap_size();
    */
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetCurrentHeapHighWatermark(uint64_t & currentHeapHighWatermark)
{
    // currentHeapHighWatermark = uxTaskGetStackHighWaterMark(NULL); /* Not enabled in freertos build */
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetRebootCount(uint16_t & rebootCount)
{
    uint32_t count = 0;

    CHIP_ERROR err = ConfigurationMgr().GetRebootCount(count);

    if (err == CHIP_NO_ERROR)
    {
        VerifyOrReturnError(count <= UINT16_MAX, CHIP_ERROR_INVALID_INTEGER_VALUE);
        rebootCount = static_cast<uint16_t>(count);
    }

    return err;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetUpTime(uint64_t & upTime)
{
    System::Clock::Timestamp currentTime = System::SystemClock().GetMonotonicTimestamp();
    System::Clock::Timestamp startTime   = PlatformMgrImpl().GetStartTime();

    if (currentTime >= startTime)
    {
        upTime = std::chrono::duration_cast<System::Clock::Seconds64>(currentTime - startTime).count();
        return CHIP_NO_ERROR;
    }

    return CHIP_ERROR_INVALID_TIME;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetTotalOperationalHours(uint32_t & totalOperationalHours)
{
    uint64_t upTime = 0;

    if (GetUpTime(upTime) == CHIP_NO_ERROR)
    {
        uint32_t totalHours = 0;
        if (ConfigurationMgr().GetTotalOperationalHours(totalHours) == CHIP_NO_ERROR)
        {
            VerifyOrReturnError(upTime / 3600 <= UINT32_MAX, CHIP_ERROR_INVALID_INTEGER_VALUE);
            totalOperationalHours = totalHours + static_cast<uint32_t>(upTime / 3600);
            return CHIP_NO_ERROR;
        }
    }

    return CHIP_ERROR_INVALID_TIME;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetBootReason(BootReasonType & bootReason)
{
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
#if 0 /* os_get_reset_reason() not implemented in current sdk */
    bootReason = BootReasonType::kUnspecified;
    uint32_t reason;
    reason = os_get_reset_reason();
    if (reason == 0x01) /* Reset reason: Power on Reset */
    {
        bootReason = BootReasonType::kPowerOnReboot;
    }
    else if (reason == 0x04) /* Reset reason: Watch dog reset */
    {
        bootReason = BootReasonType::kSoftwareWatchdogReset;
    }
    else
    {
        bootReason = BootReasonType::kUnspecified;
    }
    return CHIP_NO_ERROR;
#endif
}

CHIP_ERROR DiagnosticDataProviderImpl::GetNetworkInterfaces(NetworkInterface ** netifpp)
{
    struct netif * netif    = TalariaUtils::GetNetif(NULL);
    NetworkInterface * head = NULL;
    struct wcm_in6_addr ip6_addr[LWIP_IPV6_NUM_ADDRESSES];
    uint8_t ipv6_addr_count = 0;
    if (netif == NULL)
    {
        ChipLogError(DeviceLayer, "Failed to get network interfaces");
    }
    else
    {
            struct netif * ifa = netif;
            NetworkInterface * ifp = new NetworkInterface();
            Platform::CopyString(ifp->Name, ifa->name);
            ifp->name          = CharSpan::fromCharString(ifp->Name);
            ifp->isOperational = true;
            ifp->type          = GetInterfaceType("sta");
            ifp->offPremiseServicesReachableIPv4.SetNull();
            ifp->offPremiseServicesReachableIPv6.SetNull();
            memcpy(ifp->MacAddress, ifa->hwaddr, NETIF_MAX_HWADDR_LEN);
            ifp->hardwareAddress = ByteSpan(ifp->MacAddress, NETIF_MAX_HWADDR_LEN);

            for (uint8_t idx = 0; idx < LWIP_IPV6_NUM_ADDRESSES; ++idx)
            {
                if (true == TalariaUtils::GetAddr6(&ip6_addr[idx], idx))
                {
                    memcpy(ifp->Ipv6AddressesBuffer[idx], ip6_addr[idx].s6_addr16, kMaxIPv6AddrSize);
                    ifp->Ipv6AddressSpans[idx] = ByteSpan(ifp->Ipv6AddressesBuffer[idx], kMaxIPv6AddrSize);
                    ipv6_addr_count++;
                }
                else
                {
                    break;
                }
            }
            ifp->IPv6Addresses = app::DataModel::List<ByteSpan>(ifp->Ipv6AddressSpans, ipv6_addr_count);

            ifp->Next = head;
            head      = ifp;
        }
    *netifpp = head;
    return CHIP_NO_ERROR;
}

void DiagnosticDataProviderImpl::ReleaseNetworkInterfaces(NetworkInterface * netifp)
{
    while (netifp)
    {
        NetworkInterface * del = netifp;
        netifp                 = netifp->Next;
        delete del;
    }
}

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI || 1
CHIP_ERROR DiagnosticDataProviderImpl::GetWiFiBssId(MutableByteSpan & BssId)
{
    constexpr size_t bssIdSize = 6;
    VerifyOrReturnError(BssId.size() >= bssIdSize, CHIP_ERROR_BUFFER_TOO_SMALL);

    TalariaUtils::GetBssid(BssId.data());

    BssId.reduce_size(bssIdSize);
    return CHIP_NO_ERROR;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetWiFiSecurityType(app::Clusters::WiFiNetworkDiagnostics::SecurityTypeEnum & securityType)
{
    using app::Clusters::WiFiNetworkDiagnostics::SecurityTypeEnum;

    int8_t err = 0;
    struct wcm_security security;
    securityType = SecurityTypeEnum::kUnspecified;

    /* Get the authmode */
    err = TalariaUtils::GetAuthmode(&security);
    if (err != 0)
        return CHIP_ERROR_INCORRECT_STATE;

    /*Open Connection*/
    if (security.mask == 0) {
	    securityType = SecurityTypeEnum::kNone;
	    return CHIP_NO_ERROR;
    }

    /*Secured Connection*/
    switch (security.akm) {
        case AKM_SUITE_1X:
		if (security.mask & WCM_SECURITY_WPA || security.mask & WCM_SECURITY_RSN) {
			securityType = SecurityTypeEnum::kWep;
		}
		break;
        case AKM_SUITE_PSK:
        case AKM_SUITE_PSK_SHA256:
		if (security.mask & WCM_SECURITY_WPA) {
			securityType = SecurityTypeEnum::kWpa;
		}
		else if (security.mask & WCM_SECURITY_RSN) {
			if (security.akm == AKM_SUITE_SAE) {
				securityType = SecurityTypeEnum::kWpa3;
			}
			else {
				securityType = SecurityTypeEnum::kWpa2;
			}
		}
		break;
	case AKM_SUITE_SAE:
		securityType = SecurityTypeEnum::kWpa3;
		break;
        default:
		ChipLogError(DeviceLayer, "Wifi Security Type undefined");
		securityType = SecurityTypeEnum::kUnspecified;
		return CHIP_ERROR_INCORRECT_STATE;
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetWiFiVersion(app::Clusters::WiFiNetworkDiagnostics::WiFiVersionEnum & wifiVersion)
{
	/* Currently no sdk or wcm API available to get the wifi version.
	 * As per sdk guide 11n HT mode can be enable through boot argument.
	 * If HT mode enabled through boot arguments then return wifi version
	 * as 11n else 11g. */

	using app::Clusters::WiFiNetworkDiagnostics::WiFiVersionEnum;

	uint8_t ht_mode_enabled = os_get_boot_arg_int("wifi.ht", 0);

	if (ht_mode_enabled == 1) {
		wifiVersion = WiFiVersionEnum::kN;
	}
	else {
		wifiVersion = WiFiVersionEnum::kG;
	}
	return CHIP_NO_ERROR;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetWiFiChannelNumber(uint16_t & channelNumber)
{
    channelNumber = 0;
    struct wcm_status wcmstat;

    TalariaUtils::GetChannel(&wcmstat);
    if (wcmstat.link_up) {
	    channelNumber = wcmstat.channel;
    }
    else {
	    return CHIP_ERROR_INCORRECT_STATE;
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetWiFiRssi(int8_t & rssi)
{
    rssi = 0;
    rssi = TalariaUtils::GetRssi();

    return ((rssi != 0) ? CHIP_NO_ERROR : CHIP_ERROR_INCORRECT_STATE);
}

CHIP_ERROR DiagnosticDataProviderImpl::GetWiFiBeaconLostCount(uint32_t & beaconLostCount)
{
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetWiFiCurrentMaxRate(uint64_t & currentMaxRate)
{
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetWiFiPacketMulticastRxCount(uint32_t & packetMulticastRxCount)
{
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetWiFiPacketMulticastTxCount(uint32_t & packetMulticastTxCount)
{
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetWiFiPacketUnicastRxCount(uint32_t & packetUnicastRxCount)
{
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetWiFiPacketUnicastTxCount(uint32_t & packetUnicastTxCount)
{
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

CHIP_ERROR DiagnosticDataProviderImpl::GetWiFiOverrunCount(uint64_t & overrunCount)
{
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

CHIP_ERROR DiagnosticDataProviderImpl::ResetWiFiNetworkDiagnosticsCounts()
{
    return CHIP_NO_ERROR;
}
#endif // CHIP_DEVICE_CONFIG_ENABLE_WIFI

DiagnosticDataProvider & GetDiagnosticDataProviderImpl()
{
    return DiagnosticDataProviderImpl::GetDefaultInstance();
}

} // namespace DeviceLayer
} // namespace chip
