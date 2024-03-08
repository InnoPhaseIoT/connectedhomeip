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
#include <inet/InetBuildConfig.h>

#include <system/SystemClock.h>

#include <platform/ConnectivityManager.h>
#include <platform/talaria/ConnectivityManagerImpl.h>

#include <platform/internal/GenericConnectivityManagerImpl_UDP.ipp>
#if INET_CONFIG_ENABLE_TCP_ENDPOINT
#include <platform/internal/GenericConnectivityManagerImpl_TCP.ipp>
#endif
#include <platform/internal/GenericConnectivityManagerImpl_BLE.ipp>
#include <platform/internal/BLEManager.h>

#include <platform/internal/GenericConnectivityManagerImpl_WiFi.ipp>

using namespace ::chip;
using namespace ::chip::TLV;
using namespace ::chip::DeviceLayer;
using namespace ::chip::DeviceLayer::Internal;

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

void ConnectivityManagerImpl::wifi_test()
{
    _Init();
}

CHIP_ERROR ConnectivityManagerImpl::_Init()
{
    InitWiFi();
    return CHIP_NO_ERROR;
}

void ConnectivityManagerImpl::wifi_connect(void)
{
}

void ConnectivityManagerImpl::_OnPlatformEvent(const ChipDeviceEvent * event)
{
    OnWiFiPlatformEvent(event);
}

} // namespace DeviceLayer
} // namespace chip
