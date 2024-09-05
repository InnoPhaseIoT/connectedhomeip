/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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

#ifdef __cplusplus
    extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <talaria_two.h>
#if (CHIP_ENABLE_EXT_FLASH == true)
#include <matter_custom_ota.h>
#endif /* CHIP_ENABLE_EXT_FLASH */

void print_faults();
int
filesystem_util_mount_data_if(const char* path);

#ifdef __cplusplus
    }
#endif

#include <../third_party/nlunit-test/repo/src/nlunit-test.h>

#include <common/CHIPDeviceManager.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/PlatformManager.h>
#include <lib/core/CHIPError.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/UnitTestRegistration.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/server/Server.h>
#include <platform/CommissionableDataProvider.h>
#include <platform/DeviceInstanceInfoProvider.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <platform/talaria/NetworkCommissioningDriver.h>
#include <platform/talaria/TalariaUtils.h>
#include <platform/talaria/Config.h>
#include <app/clusters/network-commissioning/network-commissioning.h>
#include <app/server/OnboardingCodesUtil.h>
#include <common/DeviceCommissioningInterface.h>
#include <common/Utils.h>
#include <platform/talaria/FactoryDataProvider.h>

using namespace chip::DeviceLayer::Internal;

#define USER_INTENDED_COMMISSIONING_TRIGGER_GPIO 3
#if (CHIP_ENABLE_EXT_FLASH == true)
#define HOURS_TO_SECONDS(x) (x * 60 * 60)
#endif /* CHIP_ENABLE_EXT_FLASH */

using namespace chip;
using namespace chip::Platform;
using namespace chip::DeviceLayer;
using namespace chip::Credentials;
using namespace chip::app::Clusters;
using namespace chip::DeviceManager;
using namespace chip::DeviceLayer;
using namespace chip::talaria;
using namespace chip::talaria::DeviceCommissioning;

constexpr chip::EndpointId kNetworkCommissioningEndpointWiFi = 0;
chip::app::Clusters::NetworkCommissioning::Instance
    sWiFiNetworkCommissioningInstance(kNetworkCommissioningEndpointWiFi, &(chip::DeviceLayer::NetworkCommissioning::TalariaWiFiDriver::GetInstance()));
DeviceLayer::TalariaFactoryDataProvider sFactoryDataProvider;

#if (CHIP_ENABLE_EXT_FLASH == true)
static SemaphoreHandle_t matterCustomOtaCheck;
#endif /* CHIP_ENABLE_EXT_FLASH */

// static AppDeviceCallbacks EchoCallbacks;
static void InitServer(intptr_t context);

CommissioningInterface::CommissioningParam& GetCommissioningParam(CommissioningInterface::CommissioningParam &param)
{
    matterutils::CommissioningFlowType Type = matterutils::GetCommissioningFlowType();

    param.CommissioningFlowType = Type;

    if(Type == matterutils::COMMISSIONING_FLOW_TYPE_USER_INTENT)
    {
        param.TypedParam.UserIntent.TriggerGpio = USER_INTENDED_COMMISSIONING_TRIGGER_GPIO;
    }

    return param;
}

#if (CHIP_ENABLE_EXT_FLASH == true)
static void trigger_matter_custom_ota_check(chip::System::Layer * unused, void *arg)
{
    /* Give Semaphore to trigger the ota check */
    xSemaphoreGive(matterCustomOtaCheck);
}

static void schedule_next_ota_check(intptr_t context)
{
    int ota_check_interval_secs = os_get_boot_arg_int("matter.ota_check_interval_secs", HOURS_TO_SECONDS(24));

    /* Trigger check post 24 hours (default) or based on boot argument value */
    ChipLogProgress(AppServer, "Triggering the OTA check post %d seconds...", ota_check_interval_secs);
    DeviceLayer::SystemLayer().CancelTimer(trigger_matter_custom_ota_check, false);
    DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds32(ota_check_interval_secs), trigger_matter_custom_ota_check, NULL);
}

static void matter_custom_ota_check(void)
{
    int retval = FOTA_ERROR_NONE;
    fota_handle_t *f_handle = NULL;
    fota_init_param_t fota_init_param = { 0 };

#if (CHIP_ENABLE_SECUREBOOT == true)
    fota_init_param.cipher_key = (void *)TalariaConfig::GetAppCipherKey();
#endif /* CHIP_ENABLE_SECUREBOOT */

    while (true) {
        if (xSemaphoreTake(matterCustomOtaCheck, portMAX_DELAY) == pdFAIL) {
            /* Re-check for semaphore */
            continue;
        }
        ChipLogProgress(AppServer, "\nAvailable heap before initiating OTA: %d", xPortGetFreeHeapSize());

        /* Disable WCM power save for faster download */
        chip::DeviceLayer::Internal::TalariaUtils::DisableWcmPMConfig();
        /* Init custom matter ota */
        f_handle = matter_custom_ota_init(&fota_init_param);
        if (!f_handle) {
            ChipLogError(AppServer, "Error initialising matter custom ota");
            goto cleanup;
        }

        ChipLogProgress(AppServer, "Check for OTA upgrade available");
        retval = matter_custom_ota_perform(f_handle, FOTA_CHECK_FOR_UPDATE, 0);
        if (FOTA_ERROR_NONE != retval) {
            if (FOTA_ERROR_NO_NEW_UPDATE == retval) {
                ChipLogProgress(AppServer, "No new update available");
            }
            ChipLogError(AppServer, "FOTA perform failed. retval = %d", retval);
            goto cleanup;
        }
        /* fota commit.  This will reset the system */
        if (FOTA_ERROR_NONE != matter_custom_ota_commit(f_handle, 1)) {
            ChipLogError(AppServer, "FOTA commit failed");
            goto cleanup;
        }
        ChipLogProgress(AppServer, "FOTA commit successful");

cleanup:
        /* Restore WCM power config */
        chip::DeviceLayer::Internal::TalariaUtils::RestoreWcmPMConfig();
        /* Cleanup for next OTA run */
        if (f_handle) {
            matter_custom_ota_deinit(f_handle);
            f_handle = NULL;
        }

        chip::DeviceLayer::PlatformMgr().ScheduleWork(schedule_next_ota_check, reinterpret_cast<intptr_t>(nullptr));
        ChipLogProgress(AppServer, "\nAvailable heap after OTA: %d", xPortGetFreeHeapSize());
    }

    /* Cleanup: Following flow shall never get executed */
    vSemaphoreDelete(matterCustomOtaCheck);
    vTaskDelete(NULL);
}

static int matter_custom_ota_task_create(void)
{
    BaseType_t xReturned;
    static TaskHandle_t xHandle = NULL;
    int retval = 0;

    /* This API can be called in case of 2 events kCommissioningComplete and kServerReady.
     * In single run these events can happen multiple times, hence following check will
     * make sure that custom OTA task is created only once
     */
    if (xHandle == NULL)
    {
        /* Create Semaphore for triggerring the OTA check */
        matterCustomOtaCheck = xSemaphoreCreateCounting(1, 0);
        /* Create the task to load the persistent data from hsot */
        xReturned = xTaskCreate(matter_custom_ota_check,   /* Function that implements the task. */
                        "matter_custom_ota_check", /* Text name for the task. */
                        1024,     /* Stack size in words, not bytes. */
                        (void *) NULL,                /* Parameter passed into the task. */
                        tskIDLE_PRIORITY + 2,         /* Priority at which the task is created. */
                        &xHandle);                    /* Used to pass out the created task's handle. */

        if (xReturned == pdPASS)
        {
            /* The task was created.  Use the task's handle to delete the task. */
            ChipLogProgress(AppServer, "Custom OTA check task created...");

            /* Give initial semaphore to check OTA */
            xSemaphoreGive(matterCustomOtaCheck);
            retval = 0;
        }
        else
        {
            ChipLogError(AppServer, "Failed to create Custom OTA check task... errocode: %d", xReturned);
            retval = -1;
        }
    }
    return retval;
}
#endif /* CHIP_ENABLE_EXT_FLASH */

void EventHandler(const DeviceLayer::ChipDeviceEvent * event, intptr_t arg)
{
    (void) arg;
    int retval = 0;
    if (event->Type == DeviceLayer::DeviceEventType::kCHIPoBLEConnectionEstablished)
    {
        ChipLogProgress(DeviceLayer, "Receive kCHIPoBLEConnectionEstablished");
    }
    else if (event->Type == DeviceLayer::DeviceEventType::kCommissioningComplete)
    {
        ChipLogProgress(DeviceLayer, "Receive kCommissioningComplete");
#if (CHIP_ENABLE_EXT_FLASH == true)
        /* Trigger custom OTA check */
        retval = matter_custom_ota_task_create();
#endif /* CHIP_ENABLE_EXT_FLASH */
    }
    else if (event->Type == DeviceLayer::DeviceEventType::kServerReady)
    {
        ChipLogProgress(DeviceLayer, "Receive kServerReady");
#if (CHIP_ENABLE_EXT_FLASH == true)
        if (chip::DeviceLayer::Internal::TalariaUtils::IsStationProvisioned() == true) {
            /* Trigger custom OTA check only if the wifi station is provisioned */
            retval = matter_custom_ota_task_create();
        }
#endif /* CHIP_ENABLE_EXT_FLASH */
    }
}

void InitServer(intptr_t context)
{
    chip::PayloadContents payload;
    CHIP_ERROR err = GetPayloadContents(payload, chip::RendezvousInformationFlag::kBLE);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(AppServer, "GetPayloadContents() failed: %" CHIP_ERROR_FORMAT, err.Format());
    }
    int flow_type =os_get_boot_arg_int("matter.commissioning.flow_type", 0);
    if (flow_type == 1)
    {
        payload.commissioningFlow = CommissioningFlow::kUserActionRequired;
    }
    else if (flow_type == 2)
    {
        payload.commissioningFlow = CommissioningFlow::kCustom;
    }
    else{
        payload.commissioningFlow = CommissioningFlow::kStandard;
    }
    PrintOnboardingCodes(payload);
    static chip::CommonCaseDeviceServerInitParams initParams;
    (void) initParams.InitializeStaticResourcesBeforeServerInit();
    chip::Server::GetInstance().Init(initParams);

    sWiFiNetworkCommissioningInstance.Init();
    CommissioningInterface::EnableCommissioning();
    matterutils::MatterConfigLog();
}

chip::Credentials::DeviceAttestationCredentialsProvider * get_dac_provider(void)
{
    int enable_factory_data_provider = os_get_boot_arg_int("matter.enable_factory_data_provider", 0);
    if (enable_factory_data_provider == 1) {
        return &sFactoryDataProvider;
    } else {
        return chip::Credentials::Examples::GetExampleDACProvider();
    }
}

void app_test()
{
    CHIP_ERROR err;

    err = Platform::MemoryInit();
    os_printf("\nMemoryInit err %d, %s", err.AsInteger(), err.AsString());

    err = DeviceLayer::PlatformMgr().InitChipStack();
    os_printf("\nInitChipStack err %d, %s", err.AsInteger(), err.AsString());


    //ConnectivityMgr().SetBLEAdvertisingEnabled(true);
    // PlatformMgr().AddEventHandler(CHIPDeviceManager::CommonDeviceEventHandler, reinterpret_cast<intptr_t>(nullptr));

    err = PlatformMgr().StartEventLoopTask();
    os_printf("\nStartEventLoopTaks err %d, %s", err.AsInteger(), err.AsString());

    PlatformMgr().StartBackgroundEventLoopTask();
    // os_printf("\n RunBackgroundEventLoop err %d, %s\n", err.AsInteger(), err.AsString());

    err = DeviceLayer::PlatformMgrImpl().AddEventHandler(EventHandler, 0);
    os_printf("\nPlatformMgrImpl::AddEventHandler err %d, %s", err.AsInteger(), err.AsString());

    CommissioningInterface::CommissioningParam param;
    CommissioningInterface::Init(GetCommissioningParam(param));

    Credentials::SetDeviceAttestationCredentialsProvider(get_dac_provider());

    int enable_factory_data_provider = os_get_boot_arg_int("matter.enable_factory_data_provider", 0);
    int enable_device_instance_info_provider = os_get_boot_arg_int("matter.enable_device_instance_info_provider", 0);
    if (enable_factory_data_provider == 1) {
        SetCommissionableDataProvider(&sFactoryDataProvider);
        if (enable_device_instance_info_provider == 1) {
            SetDeviceInstanceInfoProvider(&sFactoryDataProvider);
        }
    }

    err = chip::DeviceLayer::PlatformMgr().ScheduleWork(InitServer, reinterpret_cast<intptr_t>(nullptr));
    os_printf("\nPlatformMgrImpl::ScheduleWork err %d, %s", err.AsInteger(), err.AsString());
}

/*-----------------------------------------------------------*/
