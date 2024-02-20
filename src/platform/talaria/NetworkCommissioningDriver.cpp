/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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


#include <lib/support/CodeUtils.h>
#include <lib/support/SafeInt.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/KeyValueStoreManager.h>
#include <platform/talaria/NetworkCommissioningDriver.h>
#include <platform/talaria/TalariaUtils.h>



#include <limits>
#include <string>

using namespace ::chip;
using namespace ::chip::DeviceLayer::Internal;
namespace chip {
namespace DeviceLayer {
namespace NetworkCommissioning {

namespace {
constexpr char kWiFiSSIDKeyName[]        = "wifi-ssid";
constexpr char kWiFiCredentialsKeyName[] = "wifi-pass";
static uint8_t WiFiSSIDStr[DeviceLayer::Internal::kMaxWiFiSSIDLength];
} // namespace


BitFlags<WiFiSecurity> ConvertSecurityType(struct wifi_netinfo *netIf)
{
    BitFlags<WiFiSecurity> securityType;
    struct wifi_authmode authmode;
    wifi_netinfo_get_authmode(netIf, &authmode);
    switch (authmode.authmask)
    {
    case 0:
        securityType.Set(WiFiSecurity::kUnencrypted);
        break;
    case IE_WPA_ENTERPRISE:
    case IE_WPA23_ENTERPRISE:
        securityType.Set(WiFiSecurity::kWep);
        break;
    case IE_WPA_PERSONAL:
        securityType.Set(WiFiSecurity::kWpaPersonal);
        break;
    case IE_WPA2_PERSONAL:
        securityType.Set(WiFiSecurity::kWpa2Personal);
        break;
    case IE_WPA_PERSONAL | IE_WPA2_PERSONAL:
        securityType.Set(WiFiSecurity::kWpa2Personal);
        securityType.Set(WiFiSecurity::kWpaPersonal);
        break;
    case IE_WPA3_PERSONAL:
        securityType.Set(WiFiSecurity::kWpa3Personal);
        break;
    case IE_WPA2_PERSONAL | IE_WPA3_PERSONAL:
        securityType.Set(WiFiSecurity::kWpa3Personal);
        securityType.Set(WiFiSecurity::kWpa2Personal);
        break;
    default:
        break;
    }
    return securityType;
}

CHIP_ERROR GetConfiguredNetwork(Network & network)
{
    /* TODO: yet to implement */
    chip::DeviceLayer::Internal::DeviceNetworkInfo netInfo;
    CHIP_ERROR err = CHIP_NO_ERROR;

    err = chip::DeviceLayer::Internal::TalariaUtils::GetWiFiStationProvision(netInfo, false);
    if (err != CHIP_NO_ERROR)
    {
        return err;
    }

    memcpy(network.networkID, netInfo.WiFiSSID, sizeof(netInfo.WiFiSSID));
    network.networkIDLen = strnlen(reinterpret_cast<const char *>(netInfo.WiFiSSID), DeviceLayer::Internal::kMaxWiFiSSIDLength);
    return err;
}

CHIP_ERROR TalariaWiFiDriver::Init(NetworkStatusChangeCallback * networkStatusChangeCallback)
{
    CHIP_ERROR err;
    size_t ssidLen        = 0;
    size_t credentialsLen = 0;

    err = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(kWiFiCredentialsKeyName, mSavedNetwork.credentials,
                                                   sizeof(mSavedNetwork.credentials), &credentialsLen);
    if (err == CHIP_ERROR_NOT_FOUND)
    {
        return CHIP_NO_ERROR;
    }

    err = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(kWiFiSSIDKeyName, mSavedNetwork.ssid,
                                                                      sizeof(mSavedNetwork.ssid), &ssidLen);

    if (err == CHIP_ERROR_NOT_FOUND)
    {
        return CHIP_NO_ERROR;
    }
    if(ssidLen > 0)
    {
	SetssidPasswordPresent(true);
    }
    if (!CanCastTo<uint8_t>(credentialsLen))
    {
        return CHIP_ERROR_INCORRECT_STATE;
    }
    mSavedNetwork.credentialsLen = static_cast<uint8_t>(credentialsLen);

    if (!CanCastTo<uint8_t>(ssidLen))
    {
        return CHIP_ERROR_INCORRECT_STATE;
    }
    mSavedNetwork.ssidLen = static_cast<uint8_t>(ssidLen);

    mStagingNetwork = mSavedNetwork;
    mpScanCallback         = nullptr;
    mpConnectCallback      = nullptr;
    mpStatusChangeCallback = networkStatusChangeCallback;
    return err;
}

void TalariaWiFiDriver::Shutdown()
{
    mpStatusChangeCallback = nullptr;
}

CHIP_ERROR TalariaWiFiDriver::CommitConfiguration()
{
#if 1
    
    ReturnErrorOnFailure(PersistedStorage::KeyValueStoreMgr().Put(kWiFiSSIDKeyName, mStagingNetwork.ssid, mStagingNetwork.ssidLen));
    ReturnErrorOnFailure(PersistedStorage::KeyValueStoreMgr().Put(kWiFiCredentialsKeyName, mStagingNetwork.credentials,
                                                                  mStagingNetwork.credentialsLen));
    SetssidPasswordPresent(true);


#endif
    mSavedNetwork = mStagingNetwork;
    return CHIP_NO_ERROR;
}

CHIP_ERROR TalariaWiFiDriver::RevertConfiguration()
{
    mStagingNetwork = mSavedNetwork;
    return CHIP_NO_ERROR;
}

bool TalariaWiFiDriver::NetworkMatch(const WiFiNetwork & network, ByteSpan networkId)
{
    return networkId.size() == network.ssidLen && memcmp(networkId.data(), network.ssid, network.ssidLen) == 0;
}

Status TalariaWiFiDriver::AddOrUpdateNetwork(ByteSpan ssid, ByteSpan credentials, MutableCharSpan & outDebugText,
                                             uint8_t & outNetworkIndex)
{
    outDebugText.reduce_size(0);
    outNetworkIndex = 0;
    VerifyOrReturnError(mStagingNetwork.ssidLen == 0 || NetworkMatch(mStagingNetwork, ssid), Status::kBoundsExceeded);
    VerifyOrReturnError(credentials.size() <= sizeof(mStagingNetwork.credentials), Status::kOutOfRange);
    VerifyOrReturnError(ssid.size() <= sizeof(mStagingNetwork.ssid), Status::kOutOfRange);

    memcpy(mStagingNetwork.credentials, credentials.data(), credentials.size());
    mStagingNetwork.credentialsLen = static_cast<decltype(mStagingNetwork.credentialsLen)>(credentials.size());

    memcpy(mStagingNetwork.ssid, ssid.data(), ssid.size());
    mStagingNetwork.ssidLen = static_cast<decltype(mStagingNetwork.ssidLen)>(ssid.size());

    return Status::kSuccess;
}

Status TalariaWiFiDriver::RemoveNetwork(ByteSpan networkId, MutableCharSpan & outDebugText, uint8_t & outNetworkIndex)
{
    outDebugText.reduce_size(0);
    outNetworkIndex = 0;
    VerifyOrReturnError(NetworkMatch(mStagingNetwork, networkId), Status::kNetworkIDNotFound);

    // Use empty ssid for representing invalid network
    mStagingNetwork.ssidLen = 0;
    return Status::kSuccess;
}

Status TalariaWiFiDriver::ReorderNetwork(ByteSpan networkId, uint8_t index, MutableCharSpan & outDebugText)
{
    outDebugText.reduce_size(0);

    // Only one network is supported now
    VerifyOrReturnError(index == 0, Status::kOutOfRange);
    VerifyOrReturnError(NetworkMatch(mStagingNetwork, networkId), Status::kNetworkIDNotFound);
    return Status::kSuccess;
}

CHIP_ERROR TalariaWiFiDriver::ConnectWiFiNetwork(const char * ssid, uint8_t ssidLen, const char * key, uint8_t keyLen)
{
    chip::DeviceLayer::Internal::DeviceNetworkInfo netInfo;
    // If device is already connected to WiFi, then disconnect the WiFi,
    // clear the WiFi configurations and add the newly provided WiFi configurations.
    bool val;
    chip::DeviceLayer::Internal::TalariaUtils::IsStationConnected(val);
    if (val)
    {
        ChipLogProgress(DeviceLayer, "Disconnecting WiFi station interface... ");
        CHIP_ERROR error = chip::DeviceLayer::Internal::TalariaUtils::ClearWiFiStationProvision();
        if (error != CHIP_NO_ERROR)
        {
            ChipLogError(DeviceLayer, "ClearWiFiStationProvision failed: %s", chip::ErrorStr(error));
            return error;
        }
    }

    ReturnErrorOnFailure(ConnectivityMgr().SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Disabled));
    
    memcpy(netInfo.WiFiSSID, ssid, std::min(ssidLen, static_cast<uint8_t>(sizeof(netInfo.WiFiSSID))));
    memcpy(netInfo.WiFiKey, key, std::min(keyLen, static_cast<uint8_t>(sizeof(netInfo.WiFiKey))));

    netInfo.WiFiSSID[ssidLen] = '\0';
    netInfo.WiFiKey[keyLen]   = '\0';

    chip::DeviceLayer::Internal::TalariaUtils::SetWiFiStationProvision(netInfo);

    ReturnErrorOnFailure(ConnectivityMgr().SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Disabled));
    return ConnectivityMgr().SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Enabled);
}

void TalariaWiFiDriver::OnConnectWiFiNetwork()
{
    DeviceLayer::SystemLayer().CancelTimer(OnConnectWiFiNetworkFailed, NULL);
    if (mpConnectCallback)
    {
        DeviceLayer::SystemLayer().CancelTimer(OnConnectWiFiNetworkFailed, NULL);
        mpConnectCallback->OnResult(Status::kSuccess, CharSpan(), 0);
        mpConnectCallback = nullptr;
    }
}

void TalariaWiFiDriver::OnConnectWiFiNetworkFailed()
{
    if (mpConnectCallback)
    {
        mpConnectCallback->OnResult(Status::kNetworkNotFound, CharSpan(), 0);
        mpConnectCallback = nullptr;
    }
}

void TalariaWiFiDriver::OnConnectWiFiNetworkFailed(chip::System::Layer * aLayer, void * aAppState)
{
    CHIP_ERROR error = chip::DeviceLayer::Internal::TalariaUtils::ClearWiFiStationProvision();
    if (error != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "ClearWiFiStationProvision failed: %s", chip::ErrorStr(error));
    }
    TalariaWiFiDriver::GetInstance().OnConnectWiFiNetworkFailed();
}

void TalariaWiFiDriver::TriggerConnectNetwork()
{
    if (mStagingNetwork.ssidLen != 0)
    {
	    ConnectWiFiNetwork(reinterpret_cast<const char *>(mStagingNetwork.ssid), mStagingNetwork.ssidLen,
			   reinterpret_cast<const char *>(mStagingNetwork.credentials), mStagingNetwork.credentialsLen);
    }
    else
    {
        ChipLogDetail(DeviceLayer, "mStagingNetwork not read, hence WiFi Trigger is not initiated");
    }
}

void TalariaWiFiDriver::ConnectNetwork(ByteSpan networkId, ConnectCallback * callback)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    Status networkingStatus = Status::kSuccess;
    Network configuredNetwork;
    const uint32_t secToMiliSec = 1000;

    VerifyOrExit(NetworkMatch(mStagingNetwork, networkId), networkingStatus = Status::kNetworkIDNotFound);
    VerifyOrExit(mpConnectCallback == nullptr, networkingStatus = Status::kUnknownError);
    ChipLogProgress(NetworkProvisioning, " NetworkCommissioningDelegate: SSID: %.*s", static_cast<int>(networkId.size()),
                    networkId.data());
    if (CHIP_NO_ERROR == GetConfiguredNetwork(configuredNetwork))
    {
        if (NetworkMatch(mStagingNetwork, ByteSpan(configuredNetwork.networkID, configuredNetwork.networkIDLen)))
        {
            if (callback)
            {
                callback->OnResult(Status::kSuccess, CharSpan(), 0);
            }
            return;
        }
    }
    err = ConnectWiFiNetwork(reinterpret_cast<const char *>(mStagingNetwork.ssid), mStagingNetwork.ssidLen,
                             reinterpret_cast<const char *>(mStagingNetwork.credentials), mStagingNetwork.credentialsLen);

    err = DeviceLayer::SystemLayer().StartTimer(
        static_cast<System::Clock::Timeout>(kWiFiConnectNetworkTimeoutSeconds * secToMiliSec), OnConnectWiFiNetworkFailed, NULL);
    mpConnectCallback = callback;

exit:
    if (err != CHIP_NO_ERROR)
    {
        networkingStatus = Status::kUnknownError;
    }
    if (networkingStatus != Status::kSuccess)
    {
        ChipLogError(NetworkProvisioning, "Failed to connect to WiFi network:%s", chip::ErrorStr(err));
        mpConnectCallback = nullptr;
        callback->OnResult(networkingStatus, CharSpan(), 0);
    }
}

CHIP_ERROR TalariaWiFiDriver::StartScanWiFiNetworks(ByteSpan ssid)
{
    struct wifi_netinfo **scan_result;
    int scanres_cnt;

    if (!mpScanCallback)
    {
        ChipLogProgress(DeviceLayer, "No scan callback");
        return;
    }

    /* Allocated scan results are getting freed after the Iterator has
       parsed the content insied NetworkCommissioningDriver.h->
       TalariaScanResponseIterator */
    scan_result = malloc(MAX_NW_SCANS * sizeof(void *));
    if (scan_result == NULL) {
        ChipLogError(DeviceLayer, "Failed to allocate memory");
        return;
    }

    TalariaUtils::ScanWiFiNetwork(scan_result, &scanres_cnt, ssid);
    if (scanres_cnt < 0) {
        ChipLogProgress(DeviceLayer, "No AP found");
        mpScanCallback->OnFinished(Status::kSuccess, CharSpan(), nullptr);
        mpScanCallback = nullptr;
        wcm_free_scanresult(scan_result, scanres_cnt);
        free(scan_result);
        goto exit;
    }

    if (CHIP_NO_ERROR == DeviceLayer::SystemLayer().ScheduleLambda([scanres_cnt, scan_result]() {
        TalariaScanResponseIterator iter(scanres_cnt, scan_result);
        if (GetInstance().mpScanCallback)
        {
            GetInstance().mpScanCallback->OnFinished(Status::kSuccess, CharSpan(), &iter);
            GetInstance().mpScanCallback = nullptr;
        }
        else
        {
            ChipLogError(DeviceLayer, "can't find the ScanCallback function");
        }
    }))
exit:
    return CHIP_NO_ERROR;
}

void TalariaWiFiDriver::OnScanWiFiNetworkDone()
{
    /* Not required to be implemented */
}

void TalariaWiFiDriver::OnNetworkStatusChange()
{
    Network configuredNetwork;
    bool staEnabled = false, staConnected = false;
    VerifyOrReturn(TalariaUtils::IsStationEnabled(staEnabled) == CHIP_NO_ERROR);
    VerifyOrReturn(staEnabled && mpStatusChangeCallback != nullptr);
    CHIP_ERROR err = GetConfiguredNetwork(configuredNetwork);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Failed to get configured network when updating network status: %s", err.AsString());
        return;
    }
    VerifyOrReturn(TalariaUtils::IsStationConnected(staConnected) == CHIP_NO_ERROR);
    if (staConnected)
    {
        mpStatusChangeCallback->OnNetworkingStatusChange(
            Status::kSuccess, MakeOptional(ByteSpan(configuredNetwork.networkID, configuredNetwork.networkIDLen)), NullOptional);
        return;
    }

    // The disconnect reason for networking status changes is allowed to have
    // manufacturer-specific values, which is why it's an int32_t, even though
    // we just store a uint16_t value in it.
    int32_t lastDisconnectReason = GetLastDisconnectReason();
    mpStatusChangeCallback->OnNetworkingStatusChange(
        Status::kUnknownError, MakeOptional(ByteSpan(configuredNetwork.networkID, configuredNetwork.networkIDLen)),
        MakeOptional(lastDisconnectReason));
}

void TalariaWiFiDriver::ScanNetworks(ByteSpan ssid, WiFiDriver::ScanCallback * callback)
{
    if (callback != nullptr)
    {
        mpScanCallback = callback;
        if (StartScanWiFiNetworks(ssid) != CHIP_NO_ERROR)
        {
            mpScanCallback = nullptr;
            callback->OnFinished(Status::kUnknownError, CharSpan(), nullptr);
        }
    }
}

CHIP_ERROR TalariaWiFiDriver::SetLastDisconnectReason(const ChipDeviceEvent * event)
{
    /* TODO */
    return CHIP_NO_ERROR;
}

uint16_t TalariaWiFiDriver::GetLastDisconnectReason()
{
    return mLastDisconnectedReason;
}

size_t TalariaWiFiDriver::WiFiNetworkIterator::Count()
{
    return mDriver->mStagingNetwork.ssidLen == 0 ? 0 : 1;
}

bool TalariaWiFiDriver::WiFiNetworkIterator::Next(Network & item)
{
    if (mExhausted || mDriver->mStagingNetwork.ssidLen == 0)
    {
        return false;
    }
    memcpy(item.networkID, mDriver->mStagingNetwork.ssid, mDriver->mStagingNetwork.ssidLen);
    item.networkIDLen = mDriver->mStagingNetwork.ssidLen;
    item.connected    = false;
    mExhausted        = true;

    Network configuredNetwork;
    CHIP_ERROR err = GetConfiguredNetwork(configuredNetwork);
    if (err == CHIP_NO_ERROR)
    {
        bool isConnected = false;
        err              = TalariaUtils::IsStationConnected(isConnected);
        if (err == CHIP_NO_ERROR && isConnected && configuredNetwork.networkIDLen == item.networkIDLen &&
            memcmp(configuredNetwork.networkID, item.networkID, item.networkIDLen) == 0)
        {
            item.connected = true;
        }
    }
    return true;
}

bool TalariaWiFiDriver::GetssidPasswordPresent()
{
    size_t ssidLen=0;
    bool ssid_pass = false;
    
    CHIP_ERROR err = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(kWiFiSSIDKeyName, mSavedNetwork.ssid,
                                                                      sizeof(mSavedNetwork.ssid), &ssidLen);
    if (err == CHIP_ERROR_NOT_FOUND)
    {
        ssid_pass = false;
    }
    else if (ssidLen > 0 )
    {
        ssid_pass = true;
    }
    SetssidPasswordPresent(ssid_pass);
    return ssid_pass;
}

void TalariaWiFiDriver::SetssidPasswordPresent(bool val)
{
    ssidPasswordPresent = val;
    return;
}

} // namespace NetworkCommissioning
} // namespace DeviceLayer
} // namespace chip
