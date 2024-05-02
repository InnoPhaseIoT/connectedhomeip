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

/**
 *    @file
 *          Provides an implementation of the BLEManager singleton object
 *          for the Talaria platform.
 */

#pragma once

#if CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE

#include <bt/bt_att.h>
#include <bt/bt_gap.h>
#include <bt/bt_gatt.h>
#include <bt/cmn.h>

#include <ble/Ble.h>

#define MAX_ADV_DATA_LEN 251 /* Same definition is in BLEManagerImpl.cpp, make sure to modify there as well */

namespace chip {
namespace DeviceLayer {
namespace Internal {

/**
 * Concrete implementation of the BLEManager singleton object for the Talaria platform.
 */
class BLEManagerImpl final : public BLEManager,
                             private Ble::BleLayer,
                             private Ble::BlePlatformDelegate,
                             private Ble::BleApplicationDelegate
{
public:
    BLEManagerImpl() {}

#if 0
    CHIP_ERROR ConfigureBle(uint32_t aAdapterId, bool aIsCentral);
#endif

private:
    // Allow the BLEManager interface class to delegate method calls to
    // the implementation methods provided by this class.
    friend BLEManager;

    // ===== Members that implement the BLEManager internal interface.

    CHIP_ERROR _Init(void);

    void _Shutdown() {}
    bool _IsAdvertisingEnabled(void);
    CHIP_ERROR _SetAdvertisingEnabled(bool val);
    bool _IsAdvertising(void);
    CHIP_ERROR _SetAdvertisingMode(BLEAdvertisingMode mode);
    CHIP_ERROR _GetDeviceName(char * buf, size_t bufSize);
    CHIP_ERROR _SetDeviceName(const char * deviceName);
    uint16_t _NumConnections(void);
    void _OnPlatformEvent(const ChipDeviceEvent * event);
    ::chip::Ble::BleLayer * _GetBleLayer(void);

    // ===== Members that implement virtual methods on BlePlatformDelegate.

    bool SubscribeCharacteristic(BLE_CONNECTION_OBJECT conId, const Ble::ChipBleUUID * svcId,
                                 const Ble::ChipBleUUID * charId) override;
    bool UnsubscribeCharacteristic(BLE_CONNECTION_OBJECT conId, const Ble::ChipBleUUID * svcId,
                                   const Ble::ChipBleUUID * charId) override;
    bool CloseConnection(BLE_CONNECTION_OBJECT conId) override;
    uint16_t GetMTU(BLE_CONNECTION_OBJECT conId) const override;
    bool SendIndication(BLE_CONNECTION_OBJECT conId, const Ble::ChipBleUUID * svcId, const Ble::ChipBleUUID * charId,
                        System::PacketBufferHandle pBuf) override;
    bool SendWriteRequest(BLE_CONNECTION_OBJECT conId, const Ble::ChipBleUUID * svcId, const Ble::ChipBleUUID * charId,
                          System::PacketBufferHandle pBuf) override;
    bool SendReadRequest(BLE_CONNECTION_OBJECT conId, const Ble::ChipBleUUID * svcId, const Ble::ChipBleUUID * charId,
                         System::PacketBufferHandle pBuf) override;
    bool SendReadResponse(BLE_CONNECTION_OBJECT conId, BLE_READ_REQUEST_CONTEXT requestContext, const Ble::ChipBleUUID * svcId,
                          const Ble::ChipBleUUID * charId) override;

    // ===== Members that implement virtual methods on BleApplicationDelegate.

    void NotifyChipConnectionClosed(BLE_CONNECTION_OBJECT conId) override;
    // ===== Members that implement virtual methods on BleConnectionDelegate.

    // ===== Members for internal use by the following friends.

    friend BLEManager & BLEMgr(void);
    friend BLEManagerImpl & BLEMgrImpl(void);

    static BLEManagerImpl sInstance;

    // ===== Private members reserved for use by this class only.

    enum class Flags : uint16_t
    {
        kAsyncInitCompleted         = 0x0001, /**< One-time asynchronous initialization actions have been performed. */
        kTalariaBLELayerInitialized = 0x0002, /**< The Talaria BLE layer has been initialized. */
        kAppRegistered              = 0x0004, /**< The CHIPoBLE application has been registered with the Talaria BLE layer. */
        kAttrsRegistered            = 0x0008, /**< The CHIPoBLE GATT attributes have been registered with the Talaria BLE layer. */
        kGATTServiceStarted         = 0x0010, /**< The CHIPoBLE GATT service has been started. */
        kAdvertisingConfigured      = 0x0020, /**< CHIPoBLE advertising has been configured in the Talaria BLE layer. */
        kAdvertising                = 0x0040, /**< The system is currently CHIPoBLE advertising. */
        kControlOpInProgress        = 0x0080, /**< An async control operation has been issued to the Talaria BLE layer. */
        kAdvertisingEnabled         = 0x0100, /**< The application has enabled CHIPoBLE advertising. */
        kFastAdvertisingEnabled     = 0x0200, /**< The application has enabled fast advertising. */
        kUseCustomDeviceName        = 0x0400, /**< The application has configured a custom BLE device name. */
        kAdvertisingRefreshNeeded   = 0x0800, /**< The advertising configuration/state in Talaria BLE layer needs to be updated. */
    };

    enum
    {
        kMaxConnections      = BLE_LAYER_NUM_BLE_ENDPOINTS,
        kMaxDeviceNameLength = 31
    };

    static uint16_t mNumGAPCons;
    // CHIPoBLEConState mCons[kMaxConnections];
    static CHIPoBLEServiceMode mServiceMode;
    static bool commissioning_completed;

    static BitFlags<Flags> mFlags;
    char mDeviceName[kMaxDeviceNameLength + 1];

    CHIP_ERROR MapBLEError(int bleErr);

    void DriveBLEState(void);
    static void DriveBLEState(intptr_t arg);
    CHIP_ERROR InitTalariaBleLayer(void);

    CHIP_ERROR ConfigureAdvertisingData(void);
    CHIP_ERROR StartAdvertising(void);

    static constexpr System::Clock::Timeout kFastAdvertiseTimeout =
        System::Clock::Milliseconds32(CHIP_DEVICE_CONFIG_BLE_ADVERTISING_INTERVAL_CHANGE_TIME);
    System::Clock::Timestamp mAdvertiseStartTime;

    static void HandleFastAdvertisementTimer(System::Layer * systemLayer, void * context);
    void HandleFastAdvertisementTimer();

    /* BLE address of this server */
    /* Pointer to our service */
    static uint8_t conn_id;
    static struct gatt_service * gatt_service;
    static struct gatt_char * chr_rx;
    static struct gatt_char * chr_tx;
    /* Pointer to link with client */
    static struct gatt_srv_link * gatt_link;
    static unsigned char subscribe_data[MAX_ADV_DATA_LEN];
    static int secured;
    static const gap_ops_t gap_ops;
    static const smp_ops_t smp_ops;
    static void prov_ble_connected_cb(bt_hci_event_t * hci_event);
    static void prov_ble_disconnected_cb(bt_hci_event_t * hci_event);
    static void passkey_output_cb(uint32_t passkey);
    static void passkey_input_cb(uint8_t handle);
    static void authenticate_cb(uint8_t handle, bt_smp_error_t result);
    static bt_att_error_t bt_handle_char_rx_cb(uint8_t bearer, bt_uuid_t * uudid, bt_gatt_fcn_t rw, uint8_t * length,
                                               uint16_t offset, uint8_t * data);
    static bt_att_error_t indication_callback(uint8_t bearer, bt_uuid_t * uuid, bt_gatt_fcn_t rw, uint8_t * length, uint16_t offset,
                                              uint8_t * data);
    static void indication_confirm_cb(uint8_t bearer);
};

/**
 * Returns a reference to the public interface of the BLEManager singleton object.
 *
 * Internal components should use this to access features of the BLEManager object
 * that are common to all platforms.
 */
inline BLEManager & BLEMgr(void)
{
    return BLEManagerImpl::sInstance;
}

/**
 * Returns the platform-specific implementation of the BLEManager singleton object.
 *
 * Internal components can use this to gain access to features of the BLEManager
 * that are specific to the Talaria platform.
 */
inline BLEManagerImpl & BLEMgrImpl(void)
{
    return BLEManagerImpl::sInstance;
}

inline ::chip::Ble::BleLayer * BLEManagerImpl::_GetBleLayer()
{
    return this;
}

inline bool BLEManagerImpl::_IsAdvertisingEnabled(void)
{
#if 1
    return mFlags.Has(Flags::kAdvertisingEnabled);
#else
    return true;
#endif
}

inline bool BLEManagerImpl::_IsAdvertising(void)
{
#if 1
    return mFlags.Has(Flags::kAdvertising);
#else
    return true;
#endif
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip

#endif // CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
