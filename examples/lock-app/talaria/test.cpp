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
#include "semphr.h"
#include "task.h"
#include <talaria_two.h>

void print_faults();
int filesystem_util_mount_data_if(const char * path);
void matter_hio_init(void);
void openCommissionWindow();

#ifdef __cplusplus
}
#endif

#include "custom_msg_exchange_group.h"
#include <../third_party/nlunit-test/repo/src/nlunit-test.h>

#include <common/CHIPDeviceManager.h>
#include <lib/core/CHIPError.h>
#include <lib/support/CHIPMem.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/PlatformManager.h>
#include <lib/support/UnitTestRegistration.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/server/Server.h>
#include <platform/CommissionableDataProvider.h>
#include <platform/DeviceInstanceInfoProvider.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <platform/talaria/NetworkCommissioningDriver.h>
#include <platform/talaria/TalariaUtils.h>
#include <app/clusters/network-commissioning/network-commissioning.h>
#include <app/server/OnboardingCodesUtil.h>
#include <common/DeviceCommissioningInterface.h>
#include <common/Utils.h>
#include <platform/talaria/FactoryDataProvider.h>
#include <app/clusters/door-lock-server/door-lock-server.h>
#include <system/SystemEvent.h>


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

// static AppDeviceCallbacks EchoCallbacks;
static void InitServer(intptr_t context);
extern SemaphoreHandle_t ServerInitDone;
bool CommissioningFlowTypeHost = false;

/*fota code start*/
#include "hio/matter.h"
#include "hio/matter_hio.h"


#if CHIP_ENABLE_OTA_STORAGE_ON_HOST
#include <app/clusters/ota-requestor/BDXDownloader.h>
#include <app/clusters/ota-requestor/DefaultOTARequestor.h>
#include <app/clusters/ota-requestor/DefaultOTARequestorStorage.h>
#include <app/clusters/ota-requestor/ExtendedOTARequestorDriver.h>
#include <platform/talaria/OTAImageProcessorImpl.h>
#include <app/clusters/ota-requestor/DefaultOTARequestorUserConsent.h>
static void InitOTARequestor();

constexpr uint16_t requestedOtaBlockSize = 4096;
char *app_name = "matter_lock_app";

namespace chip {
namespace Shell {
class OTARequestorCommands
{
public:
    // delete the copy constructor
    OTARequestorCommands(const OTARequestorCommands &) = delete;
    // delete the move constructor
    OTARequestorCommands(OTARequestorCommands &&) = delete;
    // delete the assignment operator
    OTARequestorCommands & operator=(const OTARequestorCommands &) = delete;

    static OTARequestorCommands & GetInstance()
    {
        static OTARequestorCommands instance;
        return instance;
    }

    // Register the OTA requestor commands
    void Register();

private:
    OTARequestorCommands() {}
};
}
}


class CustomOTARequestorDriver : public DeviceLayer::ExtendedOTARequestorDriver
{
public:
    bool CanConsent() override;
};

namespace {
DefaultOTARequestor gRequestorCore;
DefaultOTARequestorStorage gRequestorStorage;
CustomOTARequestorDriver gRequestorUser;
BDXDownloader gDownloader;
chip::Optional<bool> gRequestorCanConsent;
OTAImageProcessorImpl gImageProcessor;
chip::ota::DefaultOTARequestorUserConsent gUserConsentProvider;

} // namespace

bool CustomOTARequestorDriver::CanConsent()
{
    return gRequestorCanConsent.ValueOr(DeviceLayer::ExtendedOTARequestorDriver::CanConsent());
}

void InitOTARequestor(void)
{
    if (!GetRequestorInstance())
    {
        SetRequestorInstance(&gRequestorCore);
        gRequestorStorage.Init(Server::GetInstance().GetPersistentStorage());
        gRequestorCore.Init(Server::GetInstance(), gRequestorStorage, gRequestorUser, gDownloader);
        gRequestorUser.SetMaxDownloadBlockSize(requestedOtaBlockSize);
        gRequestorUser.SetSendNotifyUpdateApplied(true);
        gImageProcessor.SetOTADownloader(&gDownloader);
        gImageProcessor.ota_get_firmware_name(app_name);
        gDownloader.SetImageProcessorDelegate(&gImageProcessor);
        gRequestorUser.Init(&gRequestorCore, &gImageProcessor);
    }
}
/*fota code end*/
#endif /* CHIP_ENABLE_OTA_STORAGE_ON_HOST  */


static void OpenUserIntentCommissioningWindow(intptr_t arg)
{

    chip::Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow();
}

void openCommissionWindow()
{
    if(Server::GetInstance().GetFabricTable().FabricCount() != 0)
    {
        ChipLogError(DeviceLayer,"Device is already Commissioned hence not Opening Commissioning Window");
        /* Send event to support the Door Lock Alarm */
        ChipLogError(DeviceLayer, "Sending Door Lock Jammed Alarm Event");
        DoorLockServer::Instance().SendLockAlarmEvent(1, AlarmCodeEnum::kLockJammed);

        return;
    }
    if(CommissioningFlowTypeHost == true)
    {
        ChipLogProgress(DeviceLayer, "Opening Commissioning Window");
        chip::DeviceLayer::PlatformMgr().ScheduleBackgroundWork( OpenUserIntentCommissioningWindow , reinterpret_cast<intptr_t>(nullptr));
    }
}

CommissioningInterface::CommissioningParam& GetCommissioningParam(CommissioningInterface::CommissioningParam &param)
{
    param.CommissioningFlowType = matterutils::GetCommissioningFlowType();

    if(param.CommissioningFlowType == matterutils::COMMISSIONING_FLOW_TYPE_STANDARD || param.CommissioningFlowType == matterutils::COMMISSIONING_FLOW_TYPE_USER_INTENT )
    {
        CommissioningFlowTypeHost = true;
        ChipLogError(DeviceLayer, "\nDoor Lock application can not use Standard commissioning flow, Using UserIntentent Commissioning\n");
        param.CommissioningFlowType = matterutils::COMMISSIONING_FLOW_TYPE_HOST;
    }

    return param;
}


void EventHandler(const DeviceLayer::ChipDeviceEvent * event, intptr_t arg)
{
    (void) arg;
    if (event->Type == DeviceLayer::DeviceEventType::kCHIPoBLEConnectionEstablished)
    {
        ChipLogProgress(DeviceLayer, "Receive kCHIPoBLEConnectionEstablished");
    }
}
//void InitServer(intptr_t context);/*MJ-merge*/

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
        payload.commissioningFlow = CommissioningFlow::kUserActionRequired;
    }
    PrintOnboardingCodes(payload);
    static chip::CommonCaseDeviceServerInitParams initParams;
    (void) initParams.InitializeStaticResourcesBeforeServerInit();
    chip::Server::GetInstance().Init(initParams);

    sWiFiNetworkCommissioningInstance.Init();

    xSemaphoreGive(ServerInitDone);
    // matter_hio_init();
    // custom_msg_exchange_api_init();
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

#if CHIP_ENABLE_OTA_STORAGE_ON_HOST
    err = chip::DeviceLayer::PlatformMgrImpl().AddEventHandler(InitOTARequestor, reinterpret_cast<intptr_t>(nullptr));
    os_printf("\nOTA::ScheduleWork err %d, %s", err.AsInteger(), err.AsString());
#endif /* CHIP_ENABLE_OTA_STORAGE_ON_HOST  */

}

/*-----------------------------------------------------------*/
