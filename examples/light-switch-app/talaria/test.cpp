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
#include <kernel/gpio.h>
#include <talaria_two.h>

#ifdef __cplusplus
}
#endif

#include <../third_party/nlunit-test/repo/src/nlunit-test.h>

#include <common/CHIPDeviceManager.h>
#include <lib/core/CHIPError.h>
#include <lib/support/CHIPMem.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/PlatformManager.h>
#include <lib/support/UnitTestRegistration.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/server/Server.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <platform/CommissionableDataProvider.h>
#include <platform/DeviceInstanceInfoProvider.h>
#include <platform/talaria/NetworkCommissioningDriver.h>
#include <platform/talaria/TalariaUtils.h>
#include <app/clusters/network-commissioning/network-commissioning.h>
#include "BindingHandler.h"
#include <app/server/OnboardingCodesUtil.h>
#include <common/DeviceCommissioningInterface.h>
#include <common/Utils.h>
#include <platform/talaria/FactoryDataProvider.h>
#include "LightSwitch_ProjecConfig.h"

#define os_avail_heap xPortGetFreeHeapSize

int SoftwareTimer_Init(void);

using namespace chip;
using namespace chip::Platform;
using namespace chip::DeviceLayer;
using namespace chip::Credentials;
using namespace chip::app::Clusters;
using namespace chip::DeviceManager;
using namespace chip::DeviceLayer;
using namespace chip::talaria;
using namespace chip::talaria::DeviceCommissioning;
#define USER_INTENDED_COMMISSIONING_TRIGGER_GPIO 3;

constexpr chip::EndpointId kNetworkCommissioningEndpointWiFi = 0;
chip::app::Clusters::NetworkCommissioning::Instance
    sWiFiNetworkCommissioningInstance(kNetworkCommissioningEndpointWiFi, &(chip::DeviceLayer::NetworkCommissioning::TalariaWiFiDriver::GetInstance()));
DeviceLayer::TalariaFactoryDataProvider sFactoryDataProvider;

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

void EventHandler(const DeviceLayer::ChipDeviceEvent * event, intptr_t arg)
{
    (void) arg;
    if (event->Type == DeviceLayer::DeviceEventType::kCHIPoBLEConnectionEstablished)
    {
        ChipLogProgress(DeviceLayer, "Receive kCHIPoBLEConnectionEstablished");
    }
#if ENABLE_DIMMER_SWITCH
    if (event->Type == DeviceLayer::DeviceEventType::kCommissioningComplete)
    {
        ChipLogProgress(DeviceLayer, "Receive kCommissioningComplete");
	if (SoftwareTimer_Init() != 0)
	{
	    ChipLogProgress(DeviceLayer, "SoftwareTimer Init failed");
	}
    }
#endif /* ENABLE_DIMMER_SWITCH */
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

    InitBindingHandler();
    os_printf("Initialization completed. os_free_heap(): %d\n", os_avail_heap());
    CommissioningInterface::EnableCommissioning();
    matterutils::MatterConfigLog();

#if ENABLE_DIMMER_SWITCH
    if (matterutils::IsNodeCommissioned() == true)
    {
	if (SoftwareTimer_Init() != 0)
	{
	    ChipLogProgress(DeviceLayer, "SoftwareTimer Init failed");
	}
    }
#endif /* ENABLE_DIMMER_SWITCH */
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
