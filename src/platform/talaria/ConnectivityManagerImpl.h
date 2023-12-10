/*
 *
 *    Copyright (c) 2020-2021 Project CHIP Authors
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

#pragma once

#include <platform/CHIPDeviceConfig.h>
#include <platform/ConnectivityManager.h>

#include <platform/internal/GenericConnectivityManagerImpl.h>
#include <platform/internal/GenericConnectivityManagerImpl_UDP.h>
#if INET_CONFIG_ENABLE_TCP_ENDPOINT
#include <platform/internal/GenericConnectivityManagerImpl_TCP.h>
#endif
#include <platform/internal/GenericConnectivityManagerImpl_BLE.h>
#include <platform/internal/GenericConnectivityManagerImpl_NoThread.h>
#include <platform/internal/GenericConnectivityManagerImpl_WiFi.h>

#if CHIP_DEVICE_CONFIG_ENABLE_WPA

// #include <system/SystemMutex.h>

// #include <mutex>
#endif

// #include <platform/Linux/NetworkCommissioningDriver.h>
// #include <platform/NetworkCommissioning.h>
#include <vector>

#ifdef __cplusplus
    extern "C" {
#endif

#include <hwreg/dma.h>
#include <wifi/wcm.h>
#include <wifi/wifi_utils.h>

#ifdef __cplusplus
    }
#endif

namespace chip {
namespace Inet {
class IPAddress;
} // namespace Inet

namespace DeviceLayer {

#if CHIP_DEVICE_CONFIG_ENABLE_WPA
struct GDBusWpaSupplicant
{
    enum WpaState
    {
        INIT,
        WPA_CONNECTING,
        WPA_CONNECTED,
        WPA_NOT_CONNECTED,
        WPA_NO_INTERFACE_PATH,
        WPA_GOT_INTERFACE_PATH,
        WPA_INTERFACE_CONNECTED,
    };

    enum WpaScanning
    {
        WIFI_SCANNING_IDLE,
        WIFI_SCANNING,
    };

    WpaState state                          = INIT;
    WpaScanning scanState                   = WIFI_SCANNING_IDLE;
    // WpaFiW1Wpa_supplicant1 * proxy          = nullptr;
    // WpaFiW1Wpa_supplicant1Interface * iface = nullptr;
    // WpaFiW1Wpa_supplicant1BSS * bss         = nullptr;
    // gchar * interfacePath                   = nullptr;
    // gchar * networkPath                     = nullptr;
};
#endif

/**
 * Concrete implementation of the ConnectivityManager singleton object for Linux platforms.
 */
class ConnectivityManagerImpl final : public ConnectivityManager,
                                      public Internal::GenericConnectivityManagerImpl<ConnectivityManagerImpl>,
                                      public Internal::GenericConnectivityManagerImpl_WiFi<ConnectivityManagerImpl>,
                                      public Internal::GenericConnectivityManagerImpl_NoThread<ConnectivityManagerImpl>,
                                      public Internal::GenericConnectivityManagerImpl_UDP<ConnectivityManagerImpl>,
                                      public Internal::GenericConnectivityManagerImpl_BLE<ConnectivityManagerImpl>
{
    // Allow the ConnectivityManager interface class to delegate method calls to
    // the implementation methods provided by this class.
    friend class ConnectivityManager;
    // ConnectivityManager::WiFiStationMode mWiFiStationMode;

public:
    // const char * GetEthernetIfName() { return (mEthIfName[0] == '\0') ? nullptr : mEthIfName; }

    static const char * GetWiFiIfName() { return (sWiFiIfName[0] == '\0') ? nullptr : sWiFiIfName; }
    void wifi_connect();
    void wifi_test();

private:
    struct SEDIntervalsConfig SEDConfig;

    CHIP_ERROR _GetSEDIntervalsConfig(SEDIntervalsConfig & intervalsConfig);
    CHIP_ERROR _SetSEDIntervalsConfig(const SEDIntervalsConfig & intervalsConfig);
    CHIP_ERROR _RequestSEDActiveMode(bool onOff, bool delayIdle);

    // ===== Members that implement the ConnectivityManager abstract interface.

    CHIP_ERROR _Init();
    void _OnPlatformEvent(const ChipDeviceEvent * event);
    using Flags = GenericConnectivityManagerImpl_WiFi::ConnectivityFlags;

    WiFiStationMode _GetWiFiStationMode();
    CHIP_ERROR _SetWiFiStationMode(ConnectivityManager::WiFiStationMode val);
    System::Clock::Timeout _GetWiFiStationReconnectInterval();
    CHIP_ERROR _SetWiFiStationReconnectInterval(System::Clock::Timeout val);
    bool _IsWiFiStationEnabled();
    bool _IsWiFiStationConnected();
    bool _IsWiFiStationApplicationControlled();
    bool _IsWiFiStationProvisioned();
    void _ClearWiFiStationProvision();
    bool _CanStartWiFiScan();
    CHIP_ERROR InitWiFi(void);

    WiFiAPMode _GetWiFiAPMode();
    CHIP_ERROR _SetWiFiAPMode(WiFiAPMode val);
    bool _IsWiFiAPActive();
    bool _IsWiFiAPApplicationControlled();
    void _DemandStartWiFiAP();
    void _StopOnDemandWiFiAP();
    void _MaintainOnDemandWiFiAP();
    System::Clock::Timeout _GetWiFiAPIdleTimeout();
    void _SetWiFiAPIdleTimeout(System::Clock::Timeout val);
    static void DriveStationState(::chip::System::Layer * aLayer, void * aAppState);
    void OnWiFiPlatformEvent(const ChipDeviceEvent * event);
    void DriveStationState(void);
    void ChangeWiFiStationState(WiFiStationState newState);

    void UpdateInternetConnectivityState(void);

    static bool mAssociationStarted;
    BitFlags<Flags> mFlags;
    // static BitFlags<ConnectivityFlags> mConnectivityFlag;
    // static GDBusWpaSupplicant mWpaSupplicant CHIP_GUARDED_BY(mWpaSupplicantMutex);
    // Access to mWpaSupplicant has to be protected by a mutex because it is accessed from
    // the CHIP event loop thread and dedicated D-Bus thread started by platform manager.
    // static std::mutex mWpaSupplicantMutex;

    // NetworkCommissioning::Internal::BaseDriver::NetworkStatusChangeCallback * mpStatusChangeCallback = nullptr;

    // ==================== ConnectivityManager Private Methods ====================


    // ===== Members for internal use by the following friends.

    friend ConnectivityManager & ConnectivityMgr();
    friend ConnectivityManagerImpl & ConnectivityMgrImpl();

    static ConnectivityManagerImpl sInstance;

    // ===== Private members reserved for use by this class only.

    // char mEthIfName[50]; //[IFNAMSIZ];

    System::Clock::Timestamp mLastStationConnectFailTime;
    ConnectivityManager::WiFiStationMode mWiFiStationMode;
    ConnectivityManager::WiFiStationState mWiFiStationState;
    System::Clock::Timeout mWiFiStationReconnectInterval;
    // BitFlags<Flags> mFlags;

    static char sWiFiIfName[50]; //[IFNAMSIZ];

    static uint8_t sInterestedSSID[Internal::kMaxWiFiSSIDLength];
    static uint8_t sInterestedSSIDLen;
    // static NetworkCommissioning::WiFiDriver::ScanCallback * mpScanCallback;
    // static NetworkCommissioning::Internal::WirelessDriver::ConnectCallback * mpConnectCallback;

};

/**
 * Returns the public interface of the ConnectivityManager singleton object.
 *
 * chip applications should use this to access features of the ConnectivityManager object
 * that are common to all platforms.
 */
inline ConnectivityManager & ConnectivityMgr()
{
    return ConnectivityManagerImpl::sInstance;
}

/**
 * Returns the platform-specific implementation of the ConnectivityManager singleton object.
 *
 * chip applications can use this to gain access to features of the ConnectivityManager
 * that are specific to the ESP32 platform.
 */
inline ConnectivityManagerImpl & ConnectivityMgrImpl()
{
    return ConnectivityManagerImpl::sInstance;
}

} // namespace DeviceLayer
} // namespace chip
