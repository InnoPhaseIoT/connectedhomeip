/*
 *
 *    Copyright (c) 2020-2021 Project CHIP Authors
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
/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <platform/CommissionableDataProvider.h>
#include <platform/ConnectivityManager.h>
#include <platform/talaria/CHIPDevicePlatformConfig.h>

#include <app/server/Dnssd.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/DeviceInstanceInfoProvider.h>
#include <platform/DiagnosticDataProvider.h>
#include <platform/talaria/NetworkCommissioningDriver.h>
#include <platform/talaria/TalariaUtils.h>
#include <platform/internal/BLEManager.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <wifi/network_profile.h>
#include <wifi/wcm.h>
#include <wifi_utils.h>

#ifdef __cplusplus
}
#endif

#include <lwip/dns.h>
#include <lwip/ip_addr.h>
#include <lwip/nd6.h>
#include <lwip/netif.h>

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI

using namespace ::chip;
using namespace ::chip::Inet;
using namespace ::chip::System;
using namespace ::chip::TLV;
using chip::DeviceLayer::Internal::TalariaUtils;

namespace chip {
namespace DeviceLayer {

CHIP_ERROR ConnectivityManagerImpl::_GetSEDIntervalsConfig(SEDIntervalsConfig & intervalsConfig)
{
    /*SED Feature is currently not supported */
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

CHIP_ERROR ConnectivityManagerImpl::_SetSEDIntervalsConfig(const SEDIntervalsConfig & intervalsConfig)
{
    /*SED Feature is currently not supported */
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

CHIP_ERROR ConnectivityManagerImpl::_RequestSEDActiveMode(bool onOff, bool delayIdle)
{
    /*SED Feature is currently not supported */
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

ConnectivityManager::WiFiStationMode ConnectivityManagerImpl::_GetWiFiStationMode(void)
{
    return kWiFiStationMode_Enabled;
}

bool ConnectivityManagerImpl::_IsWiFiStationEnabled(void)
{
    return GetWiFiStationMode() == kWiFiStationMode_Enabled;
}

CHIP_ERROR ConnectivityManagerImpl::_SetWiFiStationMode(WiFiStationMode val)
{
    return CHIP_NO_ERROR;
}

bool ConnectivityManagerImpl::_IsWiFiStationProvisioned(void)
{
    return Internal::TalariaUtils::IsStationProvisioned();
}

void ConnectivityManagerImpl::_ClearWiFiStationProvision(void)
{
    if (mWiFiStationMode != kWiFiStationMode_ApplicationControlled)
    {
        CHIP_ERROR error = chip::DeviceLayer::Internal::TalariaUtils::ClearWiFiStationProvision();
        if (error != CHIP_NO_ERROR)
        {
            ChipLogError(DeviceLayer, "ClearWiFiStationProvision failed: %s", chip::ErrorStr(error));
            return;
        }
    }
}


CHIP_ERROR ConnectivityManagerImpl::InitWiFi()
{
    mLastStationConnectFailTime   = System::Clock::kZero;
    mWiFiStationMode              = kWiFiStationMode_Disabled;
    mWiFiStationState             = kWiFiStationState_NotConnected;
    mWiFiStationReconnectInterval = System::Clock::Milliseconds32(CHIP_DEVICE_CONFIG_WIFI_STATION_RECONNECT_INTERVAL);

    mFlags.SetRaw(0);

    ReturnErrorOnFailure(Internal::TalariaUtils::InitWiFiStack());

    ReturnErrorOnFailure(Internal::TalariaUtils::EnableStationMode());

    if (!IsWiFiStationProvisioned())
    {
        ReturnErrorOnFailure(SetWiFiStationMode(kWiFiStationMode_Disabled));
    }

    return CHIP_NO_ERROR;
}

void ConnectivityManagerImpl::OnWiFiPlatformEvent(const ChipDeviceEvent * event)
{
    ChipDeviceEvent eventx;
    if (event->Type == DeviceEventType::kTalariaSystemEvent)
    {
        switch (event->Platform.TalariaSystemEvent.Data)
        {
        case WCM_NOTIFY_MSG_CONNECTED:
            ChipLogProgress(DeviceLayer, "WCM_NOTIFY_MSG_CONNECTED");
            if (mWiFiStationState == kWiFiStationState_Connecting)
            {
                ChangeWiFiStationState(kWiFiStationState_Connected);
            }
            NetworkCommissioning::TalariaWiFiDriver::GetInstance().OnConnectWiFiNetwork();

            eventx.Type                          = DeviceEventType::kWiFiConnectivityChange;
            eventx.WiFiConnectivityChange.Result = kConnectivity_Established;
            PlatformMgr().PostEventOrDie(&eventx);
            break;
        case WCM_NOTIFY_MSG_ADDRESS:
            ChipLogProgress(DeviceLayer, "WCM_NOTIFY_MSG_ADDRESS");
            UpdateInternetConnectivityState();
            /* This Delay is to wait for ipv6 address to be assigned to the netif port */
            vTaskDelay(pdMS_TO_TICKS(300));
            chip::app::DnssdServer::Instance().StartServer();

            eventx.Type                           = DeviceEventType::kInterfaceIpAddressChanged;
            eventx.InterfaceIpAddressChanged.Type = InterfaceIpChangeType::kIpV6_Assigned;
            PlatformMgr().PostEventOrDie(&eventx);
            break;
        case WCM_NOTIFY_MSG_DISCONNECT_DONE:
            ChipLogProgress(DeviceLayer, "WCM_NOTIFY_MSG_DISCONNECT_DONE");
            if (mWiFiStationState == kWiFiStationState_Connected)
            {
                ChangeWiFiStationState(kWiFiStationState_NotConnected);
            }
            NetworkCommissioning::TalariaWiFiDriver::GetInstance().OnConnectWiFiNetwork();

            eventx.Type                          = DeviceEventType::kWiFiConnectivityChange;
            eventx.WiFiConnectivityChange.Result = kConnectivity_Lost;
            PlatformMgr().PostEventOrDie(&eventx);
            break;
        default:
            ChipLogDetail(DeviceLayer, "Wifi platform Event %d unhandled", event->Platform.TalariaSystemEvent.Data);
            break;
        }
    }
}

void ConnectivityManagerImpl::ChangeWiFiStationState(WiFiStationState newState)
{
    if (mWiFiStationState != newState)
    {
        ChipLogProgress(DeviceLayer, "WiFi station state change: %s -> %s", WiFiStationStateToStr(mWiFiStationState),
                        WiFiStationStateToStr(newState));
        mWiFiStationState = newState;
        SystemLayer().ScheduleLambda([]() { NetworkCommissioning::TalariaWiFiDriver::GetInstance().OnNetworkStatusChange(); });
    }
}

void ConnectivityManagerImpl::UpdateInternetConnectivityState(void)
{
    bool haveIPv4Conn      = false;
    bool haveIPv6Conn      = false;
    const bool hadIPv4Conn = mFlags.Has(ConnectivityFlags::kHaveIPv4InternetConnectivity);
    const bool hadIPv6Conn = mFlags.Has(ConnectivityFlags::kHaveIPv6InternetConnectivity);
    IPAddress addr;

    // If the WiFi station is currently in the connected state...
    if (mWiFiStationState == kWiFiStationState_Connected)
    {
        // Get the LwIP netif for the WiFi station interface.
        struct netif * netif = Internal::TalariaUtils::GetStationNetif();

        // If the WiFi station interface is up...
        if (netif != NULL && netif_is_up(netif) && netif_is_link_up(netif))
        {
            // Check if a DNS server is currently configured.  If so...
            ip_addr_t dnsServerAddr = *dns_getserver(0);
            if (!ip_addr_isany_val(dnsServerAddr))
            {
                // Search among the IPv6 addresses assigned to the interface for a Global Unicast
                // address (2000::/3) that is in the valid state.  If such an address is found...
                for (uint8_t i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++)
                {
                    if (ip6_addr_isglobal(netif_ip6_addr(netif, i)) && ip6_addr_isvalid(netif_ip6_addr_state(netif, i)))
                    {
                        // Determine if there is a default IPv6 router that is currently reachable
                        // via the station interface.  If so, presume for now that the device has
                        // IPv6 connectivity.
                        struct netif * found_if = nd6_find_route(IP6_ADDR_ANY6);
                        if (found_if && netif->num == found_if->num)
                        {
                            haveIPv6Conn = true;
                        }
                    }
                }
            }
        }
    }

    // If the internet connectivity state has changed...
    if (haveIPv4Conn != hadIPv4Conn || haveIPv6Conn != hadIPv6Conn)
    {
        // Update the current state.
        mFlags.Set(ConnectivityFlags::kHaveIPv4InternetConnectivity, haveIPv4Conn)
            .Set(ConnectivityFlags::kHaveIPv6InternetConnectivity, haveIPv6Conn);

        // Alert other components of the state change.
        ChipDeviceEvent event;
        event.Type = DeviceEventType::kInternetConnectivityChange;
        // event.InternetConnectivityChange.IPv4      = GetConnectivityChange(hadIPv4Conn, haveIPv4Conn);
        event.InternetConnectivityChange.IPv6 = GetConnectivityChange(hadIPv6Conn, haveIPv6Conn);
        // event.InternetConnectivityChange.ipAddress = addr;

        PlatformMgr().PostEventOrDie(&event);

        if (haveIPv4Conn != hadIPv4Conn)
        {
            ChipLogProgress(DeviceLayer, "%s Internet connectivity %s", "IPv4", (haveIPv4Conn) ? "ESTABLISHED" : "LOST");
        }

        if (haveIPv6Conn != hadIPv6Conn)
        {
            ChipLogProgress(DeviceLayer, "%s Internet connectivity %s", "IPv6", (haveIPv6Conn) ? "ESTABLISHED" : "LOST");
        }
    }
}

} // namespace DeviceLayer
} // namespace chip

#endif // CHIP_DEVICE_CONFIG_ENABLE_WIFI
