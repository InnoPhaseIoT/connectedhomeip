/*
 *
 *    Copyright (c) 2020-2021 Project CHIP Authors
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
/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>

#if CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE

#include <ble/BleLayer.h>
#include <ble/CHIPBleServiceData.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/CommissionableDataProvider.h>
#include <platform/internal/BLEManager.h>

#include <bt/bt_att.h>
#include <bt/bt_gap.h>
#include <bt/bt_gatt.h>
#include <bt/cmn.h>

#define MAX_ADV_DATA_LEN 251 /* Same definition is in BLEManagerImpl.h, make sure to modify there as well */
#define CHIP_ADV_DATA_TYPE_FLAGS 0x01
#define CHIP_ADV_DATA_FLAGS 0x06
#define CHIP_ADV_DATA_TYPE_SERVICE_DATA 0x16
#define CHIP_MAX_MTU_SIZE 256

using namespace ::chip;
using namespace ::chip::Ble;

namespace chip {
namespace DeviceLayer {
namespace Internal {

namespace {

const uint16_t CHIPoBLEAppId = 0x235A;

const uint128_t UUID_CHIPoBLEService = { { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xF6, 0xFF, 0x00,
                                           0x00 } };
const uint8_t ShortUUID_CHIPoBLEService[] = { 0xF6, 0xFF };
const uint128_t UUID_CHIPoBLEChar_RX = { { 0x11, 0x9D, 0x9F, 0x42, 0x9C, 0x4F, 0x9F, 0x95, 0x59, 0x45, 0x3D, 0x26, 0xF5, 0x2E, 0xEE,
                                           0x18 } };
const uint128_t UUID_CHIPoBLEChar_TX = { { 0x12, 0x9D, 0x9F, 0x42, 0x9C, 0x4F, 0x9F, 0x95, 0x59, 0x45, 0x3D, 0x26, 0xF5, 0x2E, 0xEE,
                                           0x18 } };
const ChipBleUUID ChipUUID_CHIPoBLEChar_RX = { { 0x18, 0xEE, 0x2E, 0xF5, 0x26, 0x3D, 0x45, 0x59, 0x95, 0x9F, 0x4F, 0x9C, 0x42, 0x9F,
                                                 0x9D, 0x11 } };
const ChipBleUUID ChipUUID_CHIPoBLEChar_TX = { { 0x18, 0xEE, 0x2E, 0xF5, 0x26, 0x3D, 0x45, 0x59, 0x95, 0x9F, 0x4F, 0x9C, 0x42, 0x9F,
                                                 0x9D, 0x12 } };

} // unnamed namespace

BLEManagerImpl BLEManagerImpl::sInstance;
BitFlags<BLEManagerImpl::Flags> BLEManagerImpl::mFlags;
struct gatt_service * BLEManagerImpl::gatt_service;
using CHIPoBLEServiceMode = ConnectivityManager::CHIPoBLEServiceMode;
CHIPoBLEServiceMode BLEManagerImpl::mServiceMode;
struct gatt_char * BLEManagerImpl::chr_rx;
struct gatt_char * BLEManagerImpl::chr_tx;
constexpr System::Clock::Timeout BLEManagerImpl::kFastAdvertiseTimeout;
uint8_t BLEManagerImpl::conn_id;
/* Pointer to link with client */
struct gatt_srv_link * BLEManagerImpl::gatt_link;
unsigned char BLEManagerImpl::subscribe_data[MAX_ADV_DATA_LEN];
int BLEManagerImpl::secured;
uint16_t BLEManagerImpl::mNumGAPCons;
static uint8_t indicate_count = 0;
SemaphoreHandle_t ble_mutex;
static bool BLEManagerImpl::commissioning_completed = false;

/* GAP option object to be passed to GAP functions */
const gap_ops_t BLEManagerImpl::gap_ops = {
    .connected_cb    = prov_ble_connected_cb,
    .disconnected_cb = prov_ble_disconnected_cb,
    .discovery_cb    = NULL,
};

/*SMP option object */
const smp_ops_t BLEManagerImpl::smp_ops = {
    .passkey_input_cb  = passkey_input_cb,
    .passkey_output_cb = passkey_output_cb,
    .authenticate_cb   = authenticate_cb,
};

CHIP_ERROR BLEManagerImpl::_Init()
{
    CHIP_ERROR err;
    ble_mutex = xSemaphoreCreateMutex();
    if (ConnectivityMgr().IsWiFiStationProvisioned())
    {
        ChipLogDetail(DeviceLayer, "WiFi station already provisioned, not initializing BLE");
        return CHIP_NO_ERROR;
    }

    // Initialize the Chip BleLayer.
    err = BleLayer::Init(this, this, &DeviceLayer::SystemLayer());
    SuccessOrExit(err);
    mNumGAPCons = 0;

    mFlags.ClearAll().Set(Flags::kFastAdvertisingEnabled, CHIP_DEVICE_CONFIG_CHIPOBLE_ENABLE_ADVERTISING_AUTOSTART);
    mFlags.Set(Flags::kFastAdvertisingEnabled, true);

    memset(mDeviceName, 0, sizeof(mDeviceName));
    mServiceMode = ConnectivityManager::kCHIPoBLEServiceMode_Enabled;
    PlatformMgr().ScheduleWork(DriveBLEState, 0);

exit:
    return err;
}

CHIP_ERROR BLEManagerImpl::_SetAdvertisingEnabled(bool val)
{

    CHIP_ERROR err = CHIP_NO_ERROR;
    VerifyOrExit(mServiceMode != ConnectivityManager::kCHIPoBLEServiceMode_NotSupported, err = CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE);

    if (val)
    {
        mAdvertiseStartTime = System::SystemClock().GetMonotonicTimestamp();
        ReturnErrorOnFailure(DeviceLayer::SystemLayer().StartTimer(kFastAdvertiseTimeout, HandleFastAdvertisementTimer, this));
    }
    mFlags.Set(Flags::kFastAdvertisingEnabled, val);
    mFlags.Set(Flags::kAdvertisingRefreshNeeded, 1);
    mFlags.Set(Flags::kAdvertisingEnabled, val);
    PlatformMgr().ScheduleWork(DriveBLEState, 0);
exit:
    return err;
}

void BLEManagerImpl::HandleFastAdvertisementTimer(System::Layer * systemLayer, void * context)
{
    static_cast<BLEManagerImpl *>(context)->HandleFastAdvertisementTimer();
}

void BLEManagerImpl::HandleFastAdvertisementTimer()
{
    System::Clock::Timestamp currentTimestamp = System::SystemClock().GetMonotonicTimestamp();

    if (currentTimestamp - mAdvertiseStartTime >= kFastAdvertiseTimeout)
    {
    }
}

CHIP_ERROR BLEManagerImpl::_SetAdvertisingMode(BLEAdvertisingMode mode)
{
    switch (mode)
    {
    case BLEAdvertisingMode::kFastAdvertising:
        mFlags.Set(Flags::kFastAdvertisingEnabled);
        break;
    case BLEAdvertisingMode::kSlowAdvertising:
        mFlags.Clear(Flags::kFastAdvertisingEnabled);
        break;
    default:
        return CHIP_ERROR_INVALID_ARGUMENT;
    }
    mFlags.Set(Flags::kAdvertisingRefreshNeeded);
    PlatformMgr().ScheduleWork(DriveBLEState, 0);
    return CHIP_NO_ERROR;
}

CHIP_ERROR BLEManagerImpl::_GetDeviceName(char * buf, size_t bufSize)
{
    if (strlen(mDeviceName) >= bufSize)
    {
        return CHIP_ERROR_BUFFER_TOO_SMALL;
    }
    strcpy(buf, mDeviceName);

    return CHIP_NO_ERROR;
}

CHIP_ERROR BLEManagerImpl::_SetDeviceName(const char * deviceName)
{
    if (mServiceMode == ConnectivityManager::kCHIPoBLEServiceMode_NotSupported)
    {
        return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
    }
    if (deviceName != NULL && deviceName[0] != 0)
    {
        if (strlen(deviceName) >= kMaxDeviceNameLength)
        {
            return CHIP_ERROR_INVALID_ARGUMENT;
        }
        strcpy(mDeviceName, deviceName);
        mFlags.Set(Flags::kUseCustomDeviceName);
    }
    else
    {
        mDeviceName[0] = 0;
        mFlags.Clear(Flags::kUseCustomDeviceName);
    }
    return CHIP_NO_ERROR;
}

void BLEManagerImpl::_OnPlatformEvent(const ChipDeviceEvent * event)
{
    switch (event->Type)
    {
    case DeviceEventType::kCHIPoBLESubscribe:
        HandleSubscribeReceived(event->CHIPoBLESubscribe.ConId, &CHIP_BLE_SVC_ID, &ChipUUID_CHIPoBLEChar_TX);
        {
            ChipDeviceEvent connectionEvent;
            connectionEvent.Type = DeviceEventType::kCHIPoBLEConnectionEstablished;
            PlatformMgr().PostEventOrDie(&connectionEvent);
        }
        break;

    case DeviceEventType::kCHIPoBLEUnsubscribe:
        HandleUnsubscribeReceived(event->CHIPoBLEUnsubscribe.ConId, &CHIP_BLE_SVC_ID, &ChipUUID_CHIPoBLEChar_TX);
        break;

    case DeviceEventType::kCHIPoBLEWriteReceived:
        HandleWriteReceived(event->CHIPoBLEWriteReceived.ConId, &CHIP_BLE_SVC_ID, &ChipUUID_CHIPoBLEChar_RX,
                            PacketBufferHandle::Adopt(event->CHIPoBLEWriteReceived.Data));
        break;

    case DeviceEventType::kCHIPoBLEIndicateConfirm:
        HandleIndicationConfirmation(event->CHIPoBLEIndicateConfirm.ConId, &CHIP_BLE_SVC_ID, &ChipUUID_CHIPoBLEChar_TX);
        indicate_count--;
        break;

    case DeviceEventType::kCHIPoBLEConnectionError:
        HandleConnectionError(event->CHIPoBLEConnectionError.ConId, event->CHIPoBLEConnectionError.Reason);
        break;

    case DeviceEventType::kServiceProvisioningChange:
    case DeviceEventType::kWiFiConnectivityChange:
        /* Nothing to do here */
        break;

    case DeviceEventType::kCommissioningComplete:
        commissioning_completed = true;
        break;

    default:
        break;
    }
}

bool BLEManagerImpl::SubscribeCharacteristic(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId)
{
    /* Not required for to be implemented */
    return true;
}

bool BLEManagerImpl::UnsubscribeCharacteristic(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId)
{
    /* Not required for to be implemented */
    return true;
}

bool BLEManagerImpl::CloseConnection(BLE_CONNECTION_OBJECT conId)
{
    ChipLogProgress(DeviceLayer, "Closing BLE GATT connection (con %u)", conId);

    // Signal the BLE layer to close the conntion.
    bt_gap_server_link_remove(gatt_link);

    return true;
}

uint16_t BLEManagerImpl::GetMTU(BLE_CONNECTION_OBJECT conId) const
{
    /* Not any specific api to get MTU in BLE. TODO: Check further for usage */
    return 251;
}

bool BLEManagerImpl::SendIndication(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId,
                                    PacketBufferHandle data)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    err = MapBLEError(bt_gatt_indication(gatt_link, chr_tx, data->DataLength(), data->Start(), indication_confirm_cb));
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "bt_gatt_indication() failed: %s", ErrorStr(err));
        ExitNow();
    }
    /* Maintain the count for indication and wait until all are completed */
    indicate_count++;

exit:
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "BLEManagerImpl::SendIndication() failed: %s", ErrorStr(err));
        return false;
    }
    return true;
}

bool BLEManagerImpl::SendWriteRequest(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId,
                                      PacketBufferHandle pBuf)
{
    /* Implementation is not required */
    return true;
}

bool BLEManagerImpl::SendReadRequest(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId,
                                     PacketBufferHandle pBuf)
{
    ChipLogError(DeviceLayer, "BLEManagerImpl::SendReadRequest() not supported");
    return false;
}

bool BLEManagerImpl::SendReadResponse(BLE_CONNECTION_OBJECT conId, BLE_READ_REQUEST_CONTEXT requestContext,
                                      const ChipBleUUID * svcId, const ChipBleUUID * charId)
{
    ChipLogError(DeviceLayer, "BLEManagerImpl::SendReadResponse() not supported");
    return false;
}

void BLEManagerImpl::NotifyChipConnectionClosed(BLE_CONNECTION_OBJECT conId)
{
    ChipLogProgress(Ble, "Got notification regarding chip connection closure");
}

#if 1
CHIP_ERROR BLEManagerImpl::MapBLEError(int bleErr)
{
    switch (bleErr)
    {
    case GAP_ERROR_SUCCESS:
        return CHIP_NO_ERROR;
    case GAP_ERROR_INVALID_PARAMETER:
        return CHIP_ERROR_INVALID_ARGUMENT;
    case GAP_ERROR_BUSY:
        return CHIP_ERROR_BUSY;
    case GAP_ERROR_IMPOSSIBLE:
        return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
    default:
        return CHIP_ERROR(ChipError::Range::kPlatform, bleErr);
    }
}
#endif

void BLEManagerImpl::DriveBLEState(void)
{
    if ( xSemaphoreTake(ble_mutex, portMAX_DELAY) != pdTRUE)
    {
       ChipLogError(DeviceLayer, "Unbale to get Semaphore to Start BLE Advertisement")
    }
    CHIP_ERROR err = CHIP_NO_ERROR;

    // Perform any initialization actions that must occur after the Chip task is running.
    if (!mFlags.Has(Flags::kAsyncInitCompleted))
    {
        mFlags.Set(Flags::kAsyncInitCompleted);
    }

    // Initializes the Talaria BLE layer if needed.
    if (mServiceMode == ConnectivityManager::kCHIPoBLEServiceMode_Enabled && !mFlags.Has(Flags::kTalariaBLELayerInitialized))
    {
        err = InitTalariaBleLayer();
        SuccessOrExit(err);
    }

    // If the application has enabled CHIPoBLE and BLE advertising...
    if (mServiceMode == ConnectivityManager::kCHIPoBLEServiceMode_Enabled &&
        mFlags.Has(Flags::kAdvertisingEnabled)
#if CHIP_DEVICE_CONFIG_CHIPOBLE_SINGLE_CONNECTION
        // and no connections are active...
        && (_NumConnections() == 0)
#endif
    )
    {
        // Start/re-start advertising if not already advertising, or if the advertising state of the
        // Talaria BLE layer needs to be refreshed.
        if (!mFlags.Has(Flags::kAdvertising) && mFlags.Has(Flags::kAdvertisingRefreshNeeded))
        {
            // Configure advertising data if it hasn't been done yet.  This is an asynchronous step which
            // must complete before advertising can be started.  When that happens, this method will
            // be called again, and execution will proceed to the code below.
            if (!mFlags.Has(Flags::kAdvertisingConfigured))
            {
                err = ConfigureAdvertisingData();
                if (err != CHIP_NO_ERROR)
                {
                    ChipLogError(DeviceLayer, "ConfigureAdvertisingData Failed: 0x%x", err);
                    ExitNow();
                }
            }

            // Start advertising.  This is also an asynchronous step.
            err = StartAdvertising();
            ExitNow();
        }
        // Otherwise stop advertising if needed...
        else
        {
            if (mFlags.Has(Flags::kAdvertising) && mFlags.Has(Flags::kAdvertisingRefreshNeeded))
            {
                /* TODO: Evene after below call, advertisement is not getting stopped */
                err = MapBLEError(bt_gap_discoverable_mode(GAP_DISCOVERABLE_MODE_DISABLE, bt_hci_addr_type_random, 0, address_zero, &gap_ops));
                if (err != CHIP_NO_ERROR)
                {
                    ChipLogError(DeviceLayer, "bt_gap_discoverable_mode() disable failed: %s", ErrorStr(err));
                    ExitNow();
                }

                mFlags.Clear(Flags::kAdvertising);
                PlatformMgr().ScheduleWork(DriveBLEState, 0);
                ExitNow();
            }
        }
    } else {
        if (mFlags.Has(Flags::kAdvertising)) {
            err = MapBLEError(
            bt_gap_discoverable_mode(GAP_DISCOVERABLE_MODE_DISABLE, bt_hci_addr_type_random, 0, address_zero, &gap_ops));
            if (err != CHIP_NO_ERROR)
            {
                ChipLogError(DeviceLayer, "bt_gap_discoverable_mode() disable failed: %s", ErrorStr(err));
                ExitNow();
            }

            mFlags.Clear(Flags::kAdvertising);
            ExitNow();
        }
    }

    // Stop the CHIPoBLE GATT service if needed.
    if (mServiceMode != ConnectivityManager::kCHIPoBLEServiceMode_Enabled && mFlags.Has(Flags::kTalariaBLELayerInitialized))
    {
        // TODO: what to do about existing connections??
        common_server_destroy();
        bt_gap_destroy();
        ExitNow();
    }
exit:
    xSemaphoreGive(ble_mutex);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Disabling CHIPoBLE service due to error: %s", ErrorStr(err));
        mServiceMode = ConnectivityManager::kCHIPoBLEServiceMode_Disabled;
    }
}

bt_att_error_t BLEManagerImpl::bt_handle_char_rx_cb(uint8_t bearer, bt_uuid_t * uudid, bt_gatt_fcn_t rw, uint8_t * length,
                                                    uint16_t offset, uint8_t * data)
{
    CHIP_ERROR err    = CHIP_NO_ERROR;
    uint16_t data_len = 0;

    // Copy the data to a packet buffer.
    data_len = *length;

    PacketBufferHandle buf = System::PacketBufferHandle::New(data_len, 0);
    VerifyOrExit(!buf.IsNull(), err = CHIP_ERROR_NO_MEMORY);
    VerifyOrExit(buf->AvailableDataLength() >= data_len, err = CHIP_ERROR_BUFFER_TOO_SMALL);
    memcpy(buf->Start(), data, data_len);
    buf->SetDataLength(data_len);

    // Post an event to the Chip queue to deliver the data into the Chip stack.
    {
        ChipDeviceEvent event;
        event.Type                        = DeviceEventType::kCHIPoBLEWriteReceived;
        event.CHIPoBLEWriteReceived.ConId = &conn_id; /* TODO: To be changed */
        event.CHIPoBLEWriteReceived.Data  = std::move(buf).UnsafeRelease();
        err                               = PlatformMgr().PostEvent(&event);
    }

exit:
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "HandleRXCharWrite() failed: %s", ErrorStr(err));
    }
    return 0;
}

bt_att_error_t bt_handle_char_tx_cb(uint8_t bearer, bt_uuid_t * uudid, bt_gatt_fcn_t rw, uint8_t * length, uint16_t offset,
                                    uint8_t * data)
{
    /* Not supported */
    ChipLogError(DeviceLayer, "Tx operation not supported");
    return 0;
}

bt_att_error_t cb_unused(uint8_t bearer, bt_uuid_t * uuid, bt_gatt_fcn_t rw, uint8_t * length, uint16_t offset, uint8_t * data)
{
    /* Nothing to do here */
    return BT_ATT_ERROR_SUCCESS;
}

static void BLEManagerImpl::indication_confirm_cb(uint8_t bearer)
{
    ChipDeviceEvent eventx;
    eventx.Type                          = DeviceEventType::kCHIPoBLEIndicateConfirm;
    eventx.CHIPoBLEIndicateConfirm.ConId = &conn_id;
    PlatformMgr().PostEventOrDie(&eventx);
}

static bt_att_error_t BLEManagerImpl::indication_callback(uint8_t bearer, bt_uuid_t * uuid, bt_gatt_fcn_t rw, uint8_t * length,
                                                          uint16_t offset, uint8_t * data)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    bool indicationsEnabled;

    // Determine if the client is enabling or disabling indications.
    indicationsEnabled = data[0] & GATT_CCCD_INDICATION_BIT;
    ChipLogDetail(DeviceLayer, "indicationsEnabled %d ", indicationsEnabled);

    // Post an event to the Chip queue to process either a CHIPoBLE Subscribe or Unsubscribe based on
    // whether the client is enabling or disabling indications.
    {
        ChipDeviceEvent event;
        event.Type = (indicationsEnabled) ? DeviceEventType::kCHIPoBLESubscribe : DeviceEventType::kCHIPoBLEUnsubscribe;
        event.CHIPoBLESubscribe.ConId = &conn_id;
        err                           = PlatformMgr().PostEvent(&event);
    }

    ChipLogProgress(DeviceLayer, "CHIPoBLE %s received", (indicationsEnabled) ? "subscribe" : "unsubscribe");
    return BT_ATT_ERROR_SUCCESS;
}

CHIP_ERROR BLEManagerImpl::InitTalariaBleLayer(void)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    uint16_t discriminator;

    VerifyOrExit(!mFlags.Has(Flags::kTalariaBLELayerInitialized), /* */);

    // If a custom device name has not been specified, generate a CHIP-standard name based on the
    // discriminator value
    SuccessOrExit(err = GetCommissionableDataProvider()->GetSetupDiscriminator(discriminator));

    if (!mFlags.Has(Flags::kUseCustomDeviceName))
    {
        /* TODO: define CHIP_DEVICE_CONFIG_BLE_DEVICE_NAME_PREFIX in config
        macro else default ("MATTER-") will be used */
        ChipLogProgress(Ble, mDeviceName, sizeof(mDeviceName), "%s%04u", CHIP_DEVICE_CONFIG_BLE_DEVICE_NAME_PREFIX, discriminator);
        mDeviceName[kMaxDeviceNameLength] = 0;
    }

    err = MapBLEError(bt_gap_init());
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "bt_gap_init() failed: %s", ErrorStr(err));
        ExitNow();
    }

    common_server_create(mDeviceName, 0 /*TODO: read appearance value*/, "InnoPhase");

    /* Add Service for Tx and Rx UUIDs */
    gatt_service = bt_gatt_create_service_16(0xFFF6);
    chr_rx = bt_gatt_add_char_128(gatt_service, UUID_CHIPoBLEChar_RX, bt_handle_char_rx_cb, GATT_PERM_WRITE, GATT_CHAR_PROP_W);
    chr_tx = bt_gatt_add_char_128(gatt_service, UUID_CHIPoBLEChar_TX, bt_handle_char_tx_cb, GATT_PERM_READ, GATT_CHAR_PROP_I);
    bt_gatt_add_desc_16(chr_tx, 0x2902, indication_callback, GATT_PERM_RW, GATT_CHAR_PROP_RW);
    bt_gatt_add_desc_16(chr_rx, 0x2902, cb_unused, GATT_PERM_RW, GATT_CHAR_PROP_RW);

    bt_gatt_add_service(gatt_service);

    mFlags.Set(Flags::kTalariaBLELayerInitialized);
    mFlags.Set(Flags::kAdvertisingRefreshNeeded);
exit:
    return err;
}

CHIP_ERROR BLEManagerImpl::ConfigureAdvertisingData(void)
{
    CHIP_ERROR err;
    uint8_t advData[MAX_ADV_DATA_LEN];
    uint8_t index = 0;

    memset(advData, 0, sizeof(advData));
    advData[index++] = 0x02;                         // length
    advData[index++] = 0x01;                         // AD type : flags
    advData[index++] = 0x06;                         // AD value
    advData[index++] = 0x0B;                         // length
    advData[index++] = 0x16;                         // AD type: (Service Data - 16-bit UUID)
    advData[index++] = ShortUUID_CHIPoBLEService[0]; // AD value
    advData[index++] = ShortUUID_CHIPoBLEService[1]; // AD value

    chip::Ble::ChipBLEDeviceIdentificationInfo deviceIdInfo;
    err = ConfigurationMgr().GetBLEDeviceIdentificationInfo(deviceIdInfo);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "GetBLEDeviceIdentificationInfo(): %s", ErrorStr(err));
        ExitNow();
    }

    VerifyOrExit(index + sizeof(deviceIdInfo) <= sizeof(advData), err = CHIP_ERROR_OUTBOUND_MESSAGE_TOO_BIG);
    memcpy(&advData[index], &deviceIdInfo, sizeof(deviceIdInfo));
    index = static_cast<uint8_t>(index + sizeof(deviceIdInfo));

    // Construct the Chip BLE Service Data to be sent in the scan response packet.
    err = MapBLEError(bt_gap_set_adv_data(sizeof(advData), advData));
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "bt_gap_set_adv_data failed: %s", ErrorStr(err));
        ExitNow();
    }

    mFlags.Set(Flags::kAdvertisingConfigured);

exit:
    return err;
}

/* BLE SMP callback functions */
void BLEManagerImpl::passkey_input_cb(uint8_t handle)
{
    uint8_t passkey[16];
    /* Either 20-bits passkey or 128-bits oob */
    os_printf("Enter 20-bits passkey or 128-bits oob: ...\n");
    /* FIXME */
    bt_smp_passkey_set(handle, passkey);
}

void BLEManagerImpl::passkey_output_cb(uint32_t passkey)
{
    os_printf("Passkey (to be entered on remote device): %06d\n", passkey);
}

void BLEManagerImpl::authenticate_cb(uint8_t handle, bt_smp_error_t result)
{
    if (result == BT_SMP_ERROR_SUCCESS)
    {
        os_printf("Authentication succeeded.\n");
    }
    else if (result != BT_SMP_ERROR_INTERNAL_REMOTE_LOST_BOND)
    {
        os_printf("Authentication failed (0x%x).\n", result);
        bt_gap_terminate_connection(handle, bt_err_authentication_failure);
    }
}

void subscribe_callback(uint8_t bearer, uint16_t handle, uint8_t length, uint8_t * data)
{
    for (int i = 0; i < length; i++)
    {
        os_printf("0x%2x ", data[i]);
        if (i % 80 == 0)
            os_printf("\n");
    }
}

/* Callback called when the client connects */
void BLEManagerImpl::prov_ble_connected_cb(bt_hci_event_t * hci_event)
{
    const bt_hci_evt_le_conn_cmpl_t * param = (bt_hci_evt_le_conn_cmpl_t *) &hci_event->parameter;

    os_printf("\n[PROV]BLE connection success");

    /*Add link for the connection*/
    conn_id   = param->handle;
    gatt_link = bt_gap_server_link_add(param->handle);

    /* smp authenticate */
    if (secured)
        bt_gap_authenticate(param->handle, 0 /*oob*/, 1 /*bondable*/, 1 /*mitm*/, 0 /*sc*/, 1 /*key128*/);

    mNumGAPCons++;
    ChipLogProgress(DeviceLayer, "BLE GAP connection established (con %u)", conn_id);
}

/* Callback called when the client disconnects */
void BLEManagerImpl::prov_ble_disconnected_cb(bt_hci_event_t * hci_event)
{
    os_printf("\n[PROV]BLE client disconnected");

    mNumGAPCons--;
    if (gatt_link)
    {
        bt_gap_server_link_remove(gatt_link);
        gatt_link = NULL;
    }

    /* Make server connectable again (will re-enable advertisement) */
    if (mFlags.Has(Flags::kAdvertisingEnabled))
    {
        bt_gap_connectable_mode(GAP_CONNECTABLE_MODE_UNDIRECT, bt_hci_addr_type_random, 0, address_zero, &gap_ops);
    }

    /* If the commissioning is completed, we don't need the BLE anymore, so freeing up the memory */
    if (commissioning_completed == true &&
        !mFlags.Has(Flags::kAdvertisingEnabled) &&
        mServiceMode == ConnectivityManager::kCHIPoBLEServiceMode_Enabled) {
        mServiceMode = ConnectivityManager::kCHIPoBLEServiceMode_Disabled;
        PlatformMgr().ScheduleWork(DriveBLEState, 0);
    }
}

CHIP_ERROR BLEManagerImpl::StartAdvertising(void)
{
    CHIP_ERROR err;

    /* Configure advertisement */
    bt_gap_cfg_adv_t bt_adv_handle;
    bt_smp_cfg_t bt_smp_handle;
    bt_adv_handle.fast_period   = 30000;  /* Configure fast period as 30sec as per Matter spec  */
    bt_adv_handle.slow_period   = 0;
    bt_adv_handle.fast_interval = 40;     /* Configure fast interval as 40ms as per Matter spec range(20ms-60ms) */
    bt_adv_handle.slow_interval = 480;
    bt_adv_handle.tx_power      = 0;
    bt_adv_handle.channel_map   = BT_HCI_ADV_CHANNEL_ALL;
    bt_gap_cfg_adv_set(&bt_adv_handle);


    /* Assumed the value as 0, as the value is specified 0 in the
       prov application's documentation */
    bt_smp_handle.io_cap = bt_smp_io_no_input_no_output;
    bt_smp_handle.ops          = &smp_ops;
    bt_smp_handle.oob          = 0;
    bt_smp_handle.bondable     = 1;
    bt_smp_handle.mitm         = 0;
    bt_smp_handle.sc           = 1;
    bt_smp_handle.keypress     = 0;
    bt_smp_handle.key_size_min = 16;
    bt_smp_handle.encrypt      = 1;
    bt_smp_cfg_set(&bt_smp_handle);

    err = MapBLEError(bt_gap_discoverable_mode(GAP_DISCOVERABLE_MODE_GENERAL, bt_hci_addr_type_random, 0, address_zero, &gap_ops));
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "bt_gap_discoverable_mode() start failed: %s", ErrorStr(err));
        ExitNow();
    }

    mFlags.Clear(Flags::kAdvertisingRefreshNeeded);
    mFlags.Set(Flags::kAdvertising);

exit:
    return err;
}

uint16_t BLEManagerImpl::_NumConnections(void)
{
    return mNumGAPCons;
}

void BLEManagerImpl::DriveBLEState(intptr_t arg)
{
    sInstance.DriveBLEState();
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip

#endif // CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
