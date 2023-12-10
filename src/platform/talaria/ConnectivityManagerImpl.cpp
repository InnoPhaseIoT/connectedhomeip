/*
 *
 *    Copyright (c) 2020-2022 Project CHIP Authors
 *    Copyright (c) 2019 Nest Labs, Inc.
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

#include <platform/internal/CHIPDeviceLayerInternal.h>

// #include <lib/support/TypeTraits.h>
// #include <lib/core/CHIPError.h>
// #include <system/SystemError.h>

#include <system/SystemClock.h>

// #include <platform/CommissionableDataProvider.h>
#include <platform/ConnectivityManager.h>
#include <platform/talaria/ConnectivityManagerImpl.h>

// #include <platform/DiagnosticDataProvider.h>
// #include <platform/Linux/ConnectivityUtils.h>
// #include <platform/Linux/DiagnosticDataProviderImpl.h>
// #include <platform/Linux/NetworkCommissioningDriver.h>
// #include <platform/Linux/WirelessDefs.h>

// #include <cstdlib>
// #include <new>
// #include <string>
// #include <utility>
// #include <vector>

// #include <ifaddrs.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/types.h>

// #include <lib/support/CHIPMemString.h>
// #include <lib/support/CodeUtils.h>
// #include <lib/support/logging/CHIPLogging.h>

#include <platform/internal/GenericConnectivityManagerImpl_UDP.ipp>
#include <platform/internal/GenericConnectivityManagerImpl_BLE.ipp>
#include <platform/internal/BLEManager.h>

// #if CHIP_DEVICE_CONFIG_ENABLE_WPA
// #include <platform/Linux/GlibTypeDeleter.h>
#include <platform/internal/GenericConnectivityManagerImpl_WiFi.ipp>
// #endif

using namespace ::chip;
using namespace ::chip::TLV;
using namespace ::chip::DeviceLayer;
using namespace ::chip::DeviceLayer::Internal;
// using namespace ::chip::app::Clusters::GeneralDiagnostics;
// using namespace ::chip::app::Clusters::WiFiNetworkDiagnostics;

// using namespace ::chip::DeviceLayer::NetworkCommissioning;

void test_function_ConnectivityManagerImpl()
{
    chip::DeviceLayer::ConnectivityMgrImpl().wifi_test();
}

namespace chip {
namespace DeviceLayer {

ConnectivityManagerImpl ConnectivityManagerImpl::sInstance;
uint8_t ConnectivityManagerImpl::sInterestedSSID[Internal::kMaxWiFiSSIDLength];
uint8_t ConnectivityManagerImpl::sInterestedSSIDLen;
bool ConnectivityManagerImpl::mAssociationStarted = false;

// WiFiDriver::ScanCallback * ConnectivityManagerImpl::mpScanCallback;
// NetworkCommissioning::Internal::WirelessDriver::ConnectCallback * ConnectivityManagerImpl::mpConnectCallback;
// BitFlags<Internal::GenericConnectivityManagerImpl_WiFi<ConnectivityManagerImpl>::ConnectivityFlags>
//     ConnectivityManagerImpl::mConnectivityFlag;
// struct GDBusWpaSupplicant ConnectivityManagerImpl::mWpaSupplicant;
// std::mutex ConnectivityManagerImpl::mWpaSupplicantMutex;

void ConnectivityManagerImpl::wifi_test()
{
    _Init();
    // wifi_connect();
}

CHIP_ERROR ConnectivityManagerImpl::_Init()
{
    InitWiFi();
    return CHIP_NO_ERROR;
}

void ConnectivityManagerImpl::wifi_connect(void)
{
    // int rval = 0;

    // rval = wcm_auto_connect(wcm_handle, true);
    // if(rval < 0) {
    //     os_printf("Error in wcm_auto_connect %d\n", rval);
    // }
}

void ConnectivityManagerImpl::_OnPlatformEvent(const ChipDeviceEvent * event)
{
    OnWiFiPlatformEvent(event);
    // Forward the event to the generic base classes as needed
    // if (event->Type == DeviceEventType::kESPSystemEvent)
    {
        // if (event->Platform.ESPSystemEvent.Base == WIFI_EVENT)
        {

        }

        // if (event->Platform.ESPSystemEvent.Base == IP_EVENT)
        {

        }
    }
}

} // namespace DeviceLayer
} // namespace chip
