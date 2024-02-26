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
 *          General utility methods for the Talaria platform.
 */
/* this file behaves like a config.h, comes first */
#include <platform/talaria/CHIPDevicePlatformConfig.h>
#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <app/server/Dnssd.h>
#include <lib/support/CodeUtils.h>
// #include <lib/support/ErrorStr.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/talaria/NetworkCommissioningDriver.h>
#include <platform/talaria/TalariaUtils.h>

using namespace ::chip::DeviceLayer::Internal;
using chip::DeviceLayer::Internal::DeviceNetworkInfo;
using namespace ::chip::DeviceLayer::NetworkCommissioning;
using chip::DeviceLayer::NetworkCommissioning::TalariaWiFiDriver;

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI

struct wcm_handle * TalariaUtils::wcm_handle = nullptr;
struct network_profile * TalariaUtils::profile;
bool TalariaUtils::wcm_conn_status;
char TalariaUtils::wcm_WiFiSSID[kMaxWiFiSSIDLength + 1];
static SemaphoreHandle_t wifi_disconnect_sem = NULL;

static struct wifi_counters base_count;
static uint32_t base_becont_rx_count;
static uint32_t base_becont_loss_count;

static void TalariaUtils::wcm_notifier(void * ctx, struct os_msg * msg)
{
    ChipDeviceEvent event;
    memset(&event, 0, sizeof(event));
    event.Type = DeviceEventType::kTalariaSystemEvent;

    switch (msg->msg_type)
    {
    case WCM_NOTIFY_MSG_LINK_UP:
        os_printf("WCM_NOTIFY_MSG_LINK_UP");
        break;
    case WCM_NOTIFY_MSG_LINK_DOWN:
        os_printf("WCM_NOTIFY_MSG_LINK_DOWN");
        break;
    case WCM_NOTIFY_MSG_ADDRESS:
        os_printf("WCM_NOTIFY_MSG_ADDRESS");
        break;
    case WCM_NOTIFY_MSG_DISCONNECT_DONE:
        wcm_conn_status = false;
        os_printf("WCM_NOTIFY_MSG_DISCONNECT_DONE");
        event.Platform.TalariaSystemEvent.Data = WCM_NOTIFY_MSG_DISCONNECT_DONE;
        if(wifi_disconnect_sem) {
            xSemaphoreGive(wifi_disconnect_sem);
        }
        break;
    case WCM_NOTIFY_MSG_CONNECTED:
        os_printf("WCM_NOTIFY_MSG_CONNECTED");
        event.Platform.TalariaSystemEvent.Data = WCM_NOTIFY_MSG_CONNECTED;
        wcm_conn_status                        = true;

        break;
    case WCM_NOTIFY_MSG_DEAUTH_REASON_CODES:
        os_printf("WCM_NOTIFY_MSG_DEAUTH_REASON_CODES");
        break;
    case WCM_NOTIFY_MSG_ASSOC_STATUS_CODES:
        os_printf("WCM_NOTIFY_MSG_ASSOC_STATUS_CODES");
        break;
    case WCM_NOTIFY_MSG_SYSTEM_CODES:
        os_printf("WCM_NOTIFY_MSG_SYSTEM_CODES");
        break;
    default:
        break;
    }
    PlatformMgr().PostEventOrDie(&event);
    os_msg_release(msg);
}

CHIP_ERROR TalariaUtils::IsAPEnabled(bool & apEnabled)
{
    return CHIP_ERROR_NOT_IMPLEMENTED;
}

CHIP_ERROR TalariaUtils::IsStationEnabled(bool & staEnabled)
{
    /* Expected init will be done on intialization itself */
    staEnabled = true;
    return CHIP_NO_ERROR;
}

bool TalariaUtils::IsStationProvisioned(void)
{
    bool val = chip::DeviceLayer::NetworkCommissioning::TalariaWiFiDriver::GetInstance().GetssidPasswordPresent();
    return (val);
}

CHIP_ERROR TalariaUtils::IsStationConnected(bool & connected)
{
    connected = (wcm_conn_status);
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaUtils::RequestSEDActive(uint32_t listern_interval, uint32_t traffic_tmo)
{
    /* RequestSEDActive can be called to set listen_interval and traffic_tmo in
       wcm power management configuration */
    if(wcm_handle == nullptr)
    {
        return CHIP_ERROR_NOT_CONNECTED;
    }

    wcm_pm_config(wcm_handle, listern_interval, traffic_tmo, WIFI_PM_DTIM_ONLY);
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaUtils::StartWiFiLayer(void)
{
    os_printf("\n ConnectivityManagerImpl::_Init \n");

    /* Set the wifi connection status false */
    wcm_conn_status = false;

    /*Create a Wi-Fi network interface*/
    wcm_handle = wcm_create(NULL);

    wcm_notify_enable(wcm_handle, wcm_notifier, NULL);
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaUtils::EnableStationMode(void)
{
    /* Nothing to do here */
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaUtils::SetAPMode(bool enabled)
{
    ChipLogError(DeviceLayer, "%s() not supported", __func__);
    return CHIP_NO_ERROR;
}

int TalariaUtils::OrderScanResultsByRSSI(const void * _res1, const void * _res2)
{
    return 0;
}

struct netif * TalariaUtils::GetStationNetif(void)
{
    return GetNetif("WIFI_STA_DEF");
}

CHIP_ERROR TalariaUtils::GetWiFiStationProvision(Internal::DeviceNetworkInfo & netInfo, bool includeCredentials)
{
    int retval = 0;
    struct cipher_buffer pwd;
    struct cipher_buffer pwd_id;

    /* Check whether the wcm status is connectd */
    if (!wcm_conn_status)
    {
        return TalariaUtils::MapError(wcm_conn_status);
    }
    netInfo.NetworkId              = kWiFiStationNetworkId;
    netInfo.FieldPresent.NetworkId = true;
    memcpy(netInfo.WiFiSSID, wcm_WiFiSSID, min(strlen(reinterpret_cast<char *>(wcm_WiFiSSID)) + 1, sizeof(netInfo.WiFiSSID)));
    // Enforce that netInfo wifiSSID is null terminated
    netInfo.WiFiSSID[kMaxWiFiSSIDLength] = '\0';

    if (includeCredentials)
    {
        retval = network_profile_get_sae_password(profile, &pwd, &pwd_id);
        if (!retval)
        {
            ChipLogError(DeviceLayer, "network_profile_get_sae_password() Failed. Retval: %d", retval);
            return TalariaUtils::MapError(retval);
        }
        static_assert(sizeof(netInfo.WiFiKey) < 255, "Our min might not fit in netInfo.WiFiKeyLen");
        netInfo.WiFiKeyLen = pwd.size;
        memcpy(netInfo.WiFiKey, pwd.ptr, netInfo.WiFiKeyLen);
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaUtils::SetWiFiStationProvision(const Internal::DeviceNetworkInfo & netInfo)
{
    int retval = 0;
    if(strlen(netInfo.WiFiKey) != 0)
    {
        retval = network_profile_new_from_function_parameters(&profile, netInfo.WiFiSSID, netInfo.WiFiKey, netInfo.WiFiKey, NULL, NULL, NULL);
    }
    else if (strlen(netInfo.WiFiKey) == 0)
    {
	    /* This changes are for open wifi network connection  */
        retval = network_profile_new_from_function_parameters(&profile, netInfo.WiFiSSID, NULL, NULL, NULL, NULL, NULL);
    }
  
    if (retval < 0)
    {
        ChipLogError(DeviceLayer, "network_profile_new_from_function_parameters() Failed. Retval: %d", retval);
        return TalariaUtils::MapError(retval);
    }

    retval = wcm_add_network_profile(wcm_handle, profile);
    if (retval < 0)
    {
        ChipLogError(DeviceLayer, "wcm_add_network_profile() Failed. Retval: %d", retval);
        return TalariaUtils::MapError(retval);
    }

    retval = wcm_auto_connect(wcm_handle, true);
    if (retval < 0)
    {
        ChipLogError(DeviceLayer, "wcm_auto_connect() Failed. Retval: %d", retval);
        return TalariaUtils::MapError(retval);
    }

    /* Store SSID */
    memcpy(wcm_WiFiSSID, netInfo.WiFiSSID, sizeof(netInfo.WiFiSSID));
    ChipLogProgress(DeviceLayer, "WiFi station provision set (SSID: %s)", netInfo.WiFiSSID);

    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaUtils::ClearWiFiStationProvision(void)
{
    int err;
    err = wcm_auto_connect(wcm_handle, 0);
    if (err < 0)
    {
        ChipLogError(DeviceLayer, "Wifi Disconnectiom Failed: %d", err);
    }
    err = wcm_delete_network_profile(wcm_handle,NULL);
    if (err < 0)
    {
        ChipLogError(DeviceLayer, "Wifi Could not delete Network Profile: %d", err);
    }
    wifi_disconnect_sem = xSemaphoreCreateBinary();
    xSemaphoreTake(wifi_disconnect_sem, pdMS_TO_TICKS(2000));
    vSemaphoreDelete(wifi_disconnect_sem);
    wifi_disconnect_sem = NULL;
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaUtils::InitWiFiStack(void)
{
    wcm_handle = wcm_create(NULL);

    wcm_notify_enable(wcm_handle, wcm_notifier, NULL);
    return CHIP_NO_ERROR;
}
#endif // CHIP_DEVICE_CONFIG_ENABLE_WIFI

struct netif * TalariaUtils::GetNetif(const char * ifKey)
{
    return wcm_get_netif(wcm_handle);
}

int TalariaUtils::GetAuthmode(struct wcm_security * security)
{
    return wcm_get_security(wcm_handle, security);
}

uint64_t TalariaUtils::Get_max_rate(void)
{
     return wcm_get_wifi_current_rate(wcm_handle);
}

uint32_t TalariaUtils::Get_unicast_rx_pkt_count(void)
{
     struct wifi_counters out;

     wcm_get_counters(wcm_handle, &out, sizeof(struct wifi_counters));
     return (out.rxframe - base_count.rxframe);
}

uint32_t TalariaUtils::Get_unicast_tx_pkt_count(void)
{
     struct wifi_counters out;

     wcm_get_counters(wcm_handle, &out, sizeof(struct wifi_counters));
     return (out.txframe - base_count.txframe);
}

uint32_t TalariaUtils::Get_multicast_rx_pkt_count(void)
{
     struct wifi_counters out;

     wcm_get_counters(wcm_handle, &out, sizeof(struct wifi_counters));
     return (out.rxmframe - base_count.rxmframe);
}

uint32_t TalariaUtils::Get_multicast_tx_pkt_count(void)
{
     struct wifi_counters out;

     wcm_get_counters(wcm_handle, &out, sizeof(struct wifi_counters));
     return (out.txmframe - base_count.txmframe);
}

uint32_t TalariaUtils::Get_overrun_count(void)
{
     return wcm_get_wifi_overrun_pkt_count(wcm_handle);
}

uint32_t TalariaUtils::Get_wifi_beacon_rx_count(void)
{
     return (wcm_get_wifi_beacon_rx_count(wcm_handle) - base_becont_rx_count);
}

uint32_t TalariaUtils::Get_wifi_beacon_lost_count(void)
{
     return (wcm_get_wifi_beacon_miss_count(wcm_handle) - base_becont_loss_count);
}

void TalariaUtils::Reset_wifi_diagnostics_counter(void)
{
    /* Workaround: We don't have the sdk api to reset the counters
        Store the base values from counters and deduct while the counters
        are read from actual values
    */
    base_becont_rx_count = wcm_get_wifi_beacon_rx_count(wcm_handle);
    base_becont_loss_count = wcm_get_wifi_beacon_miss_count(wcm_handle);
    wcm_get_counters(wcm_handle, &base_count, sizeof(struct wifi_counters));
}

void TalariaUtils::GetBssid(uint8_t * bssid)
{
    return wcm_get_bssid(wcm_handle, bssid);
}

void TalariaUtils::GetChannel(struct wcm_status * wcmstat)
{
    wcm_get_status(wcm_handle, wcmstat);
}

int TalariaUtils::GetRssi()
{
    return wcm_get_rssi(wcm_handle);
}

bool TalariaUtils::GetAddr6(struct wcm_in6_addr * wcm_addr6, int addr6_idx)
{
    return wcm_get_netif_addr6(wcm_handle, wcm_addr6, addr6_idx);
}

bool TalariaUtils::IsInterfaceUp(const char * ifKey)
{
    struct netif * netif = GetNetif(ifKey);
    return netif != NULL && netif_is_up(netif);
}

bool TalariaUtils::HasIPv6LinkLocalAddress(const char * ifKey)
{
    /* TODO:::::::::::::::*/
    struct wcm_in6_addr if_ip6_unused;
    return (wcm_conn_status == true);
}

CHIP_ERROR TalariaUtils::MapError(bool error)
{
    if (error == true)
    {
        return CHIP_NO_ERROR;
    }
    if (error == false)
    {
        return CHIP_ERROR_INCORRECT_STATE;
    }
    return CHIP_ERROR(ChipError::Range::kPlatform, error);
}

/**
 * Given a CHIP error value that represents an Talaria error, returns a
 * human-readable NULL-terminated C string describing the error.
 *
 * @param[in] buf                   Buffer into which the error string will be placed.
 * @param[in] bufSize               Size of the supplied buffer in bytes.
 * @param[in] err                   The error to be described.
 *
 * @return true                     If a description string was written into the supplied buffer.
 * @return false                    If the supplied error was not an Talaria error.
 *
 */
bool TalariaUtils::FormatError(char * buf, uint16_t bufSize, CHIP_ERROR err)
{
    if (!err.IsRange(ChipError::Range::kPlatform))
    {
        return false;
    }

    const char * desc = NULL;

    chip::FormatError(buf, bufSize, "Talaria", err, desc);

    return true;
}

/**
 * Register a text error formatter for Talaria errors.
 */
void TalariaUtils::RegisterTalariaErrorFormatter()
{
    static ErrorFormatter sErrorFormatter = { TalariaUtils::FormatError, NULL };

    RegisterErrorFormatter(&sErrorFormatter);
}

void TalariaUtils::ScanWiFiNetwork(struct wifi_netinfo **scan_result, int *scanres_cnt, ByteSpan ssid)
{
    struct wifi_scan_param scanparam;

    /* Init default scan parameters */
    wifi_init_scan_default(&scanparam);
    if (!ssid.empty()) {
        memcpy(scanparam.ssid.ws_ssid, ssid.data(), ssid.size());
        scanparam.ssid.ws_len = ssid.size();
    }

    *scanres_cnt = wcm_scan(wcm_handle, &scanparam, scan_result, MAX_NW_SCANS);
}

CHIP_ERROR TalariaUtils::GetWiFiInterfaceMAC(uint8_t *mac_addr)
{
    if (wcm_handle == NULL) {
        return CHIP_ERROR_INCORRECT_STATE;
    }
    memcpy(mac_addr, wcm_get_hwaddr(wcm_handle), IEEE80211_ADDR_LEN);
    return CHIP_NO_ERROR;
}