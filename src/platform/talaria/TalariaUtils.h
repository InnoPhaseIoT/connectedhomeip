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

#pragma once

#include "platform/internal/DeviceNetworkInfo.h"
#include <platform/internal/CHIPDeviceLayerInternal.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <wifi/network_profile.h>
#include <wifi/wcm.h>
#include <wifi_utils.h>

#ifdef __cplusplus
}
#endif

#define MAX_NW_SCANS    64
namespace chip {
namespace DeviceLayer {
namespace Internal {

class TalariaUtils
{
public:
    static struct wcm_handle * wcm_handle;
    static struct network_profile * profile;
    static bool wcm_conn_status;
    static char wcm_WiFiSSID[kMaxWiFiSSIDLength + 1];

    static void wcm_notifier(void * ctx, struct os_msg * msg);
    static CHIP_ERROR IsAPEnabled(bool & apEnabled);
    static CHIP_ERROR IsStationEnabled(bool & staEnabled);
    static bool IsStationProvisioned(void);
    static CHIP_ERROR IsStationConnected(bool & connected);
    static CHIP_ERROR StartWiFiLayer(void);
    static CHIP_ERROR EnableStationMode(void);
    static CHIP_ERROR SetAPMode(bool enabled);
    static int OrderScanResultsByRSSI(const void * _res1, const void * _res2);
    // static const char * WiFiModeToStr(wifi_mode_t wifiMode);
    static struct netif * GetNetif(const char * ifKey);
    static int GetAuthmode(struct wcm_security * security);
    static void GetBssid(uint8_t * bssid);
    static uint64_t Get_max_rate(void);
    static uint32_t Get_unicast_rx_pkt_count(void);
    static uint32_t Get_unicast_tx_pkt_count(void);
    static uint32_t Get_multicast_rx_pkt_count(void);
    static uint32_t Get_multicast_tx_pkt_count(void);
    static uint32_t Get_overrun_count(void);
    static uint32_t Get_wifi_beacon_rx_count(void);
    static uint32_t Get_wifi_beacon_lost_count(void);
    static void Reset_wifi_diagnostics_counter(void);
    static void GetChannel(struct wcm_status * wcmstat);
    static int GetRssi();
    static bool GetAddr6(struct wcm_in6_addr * wcm_addr6, int addr6_idx);
    static struct netif * GetStationNetif(void);
    static bool IsInterfaceUp(const char * ifKey);
    static bool HasIPv6LinkLocalAddress(const char * ifKey);

    static CHIP_ERROR GetWiFiStationProvision(Internal::DeviceNetworkInfo & netInfo, bool includeCredentials);
    static CHIP_ERROR SetWiFiStationProvision(const Internal::DeviceNetworkInfo & netInfo);
    static CHIP_ERROR ClearWiFiStationProvision(void);
    static CHIP_ERROR InitWiFiStack(void);

    static CHIP_ERROR MapError(bool error);
    static void RegisterTalariaErrorFormatter();
    static bool FormatError(char * buf, uint16_t bufSize, CHIP_ERROR err);
    static CHIP_ERROR RequestSEDActive(uint32_t listern_interval, uint32_t traffic_tmo);
    static void ScanWiFiNetwork(struct wifi_netinfo **scan_result, int *scanres_cnt, ByteSpan ssid);
    static CHIP_ERROR GetWiFiInterfaceMAC(uint8_t *mac_addr);
    static void retryConnectWiFi();
    static CHIP_ERROR DisableWcmPMConfig();
    static CHIP_ERROR RestoreWcmPMConfig();
    static struct wcm_handle * TalariaUtils::Get_wcm_handle(void);
};

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip
