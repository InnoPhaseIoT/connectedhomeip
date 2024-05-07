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

// #define UNIT_TEST

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <kernel/gpio.h>
#include <talaria_two.h>
#include <hio/matter.h>
#include <hio/matter_hio.h>

void Smoke_co_alarm_update_status();
#ifdef __cplusplus
}
#endif

#define os_avail_heap xPortGetFreeHeapSize

#include <../third_party/nlunit-test/repo/src/nlunit-test.h>

#include <lib/core/CHIPError.h>
#include <lib/support/CHIPMem.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/PlatformManager.h>
#include <lib/support/UnitTestRegistration.h>
#include <platform/talaria/DeviceInfoProviderImpl.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/server/Server.h>

#include <common/Utils.h>
#include <platform/ConnectivityManager.h>
#include <platform/talaria/TalariaUtils.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/smoke-co-alarm-server/smoke-co-alarm-server.h>
#include <app/clusters/smoke-co-alarm-server/SmokeCOTestEventTriggerDelegate.h>

#define IDENTIFY_TIMER_DELAY_MS  1000
#define LED_PIN 14

/* Periodic timeout for Smoke Alaram software timer in ms */
#define SMOKE_ALARM_ENDPOINT_ID 1

struct smoke_co_alarm_get_data revd_smoke_co_alarm_data;
SemaphoreHandle_t ServerInitDone;
SemaphoreHandle_t Getdata;


using namespace chip;
using namespace chip::Platform;
using namespace chip::DeviceLayer;
using namespace chip::Credentials;
using namespace chip::app::Clusters;
using namespace chip::talaria;
using namespace chip::app::Clusters::SmokeCoAlarm;
using namespace chip::app::Clusters::SmokeCoAlarm::Attributes;

DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

/*-----------------------------------------------------------*/

void print_test_results(nlTestSuite * tSuite);
void app_test();

uint32_t led_pin;
static uint32_t identifyTimerCount = 0;

/* Function Declarations */

static SmokeCoAlarmServer mSmokealarmInstance = SmokeCoAlarmServer::Instance();
static std::array<ExpressedStateEnum, SmokeCoAlarmServer::kPriorityOrderLength> sPriorityOrder = {
    ExpressedStateEnum::kSmokeAlarm,     ExpressedStateEnum::kInterconnectSmoke, ExpressedStateEnum::kCOAlarm,
    ExpressedStateEnum::kInterconnectCO, ExpressedStateEnum::kHardwareFault,     ExpressedStateEnum::kTesting,
    ExpressedStateEnum::kEndOfService,   ExpressedStateEnum::kBatteryAlert
};

static void Update_SmokeCOAlarm_status_attributes(intptr_t arg)
{
    mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);

    SmokeCoAlarm::Attributes::ExpressedState::Set(SMOKE_ALARM_ENDPOINT_ID,
		    static_cast<ExpressedStateEnum>(revd_smoke_co_alarm_data.ExpressedState));

    SmokeCoAlarm::Attributes::SmokeState::Set(SMOKE_ALARM_ENDPOINT_ID,
		    static_cast<AlarmStateEnum>(revd_smoke_co_alarm_data.SmokeState));

    SmokeCoAlarm::Attributes::COState::Set(SMOKE_ALARM_ENDPOINT_ID,
		    static_cast<AlarmStateEnum>(revd_smoke_co_alarm_data.COState));

    SmokeCoAlarm::Attributes::BatteryAlert::Set(SMOKE_ALARM_ENDPOINT_ID,
		    static_cast<AlarmStateEnum>(revd_smoke_co_alarm_data.BatteryAlert));

    SmokeCoAlarm::Attributes::DeviceMuted::Set(SMOKE_ALARM_ENDPOINT_ID,
		    static_cast<MuteStateEnum>(revd_smoke_co_alarm_data.DeviceMuted));

    SmokeCoAlarm::Attributes::TestInProgress::Set(SMOKE_ALARM_ENDPOINT_ID,
		    static_cast<bool>(revd_smoke_co_alarm_data.TestInProgress));

    SmokeCoAlarm::Attributes::HardwareFaultAlert::Set(SMOKE_ALARM_ENDPOINT_ID,
		    static_cast<bool>(revd_smoke_co_alarm_data.HardwareFaultAlert));

    SmokeCoAlarm::Attributes::EndOfServiceAlert::Set(SMOKE_ALARM_ENDPOINT_ID,
		    static_cast<EndOfServiceEnum>(revd_smoke_co_alarm_data.EndOfServiceAlert));

    SmokeCoAlarm::Attributes::InterconnectSmokeAlarm::Set(SMOKE_ALARM_ENDPOINT_ID,
		    static_cast<AlarmStateEnum>(revd_smoke_co_alarm_data.InterconnectSmokeAlarm));

    SmokeCoAlarm::Attributes::InterconnectCOAlarm::Set(SMOKE_ALARM_ENDPOINT_ID,
		    static_cast<AlarmStateEnum>(revd_smoke_co_alarm_data.InterconnectCOAlarm));

    SmokeCoAlarm::Attributes::ContaminationState::Set(SMOKE_ALARM_ENDPOINT_ID,
		    static_cast<ContaminationStateEnum>(revd_smoke_co_alarm_data.ContaminationState));

    SmokeCoAlarm::Attributes::SmokeSensitivityLevel::Set(SMOKE_ALARM_ENDPOINT_ID,
		    static_cast<SensitivityEnum>(revd_smoke_co_alarm_data.SmokeSensitivityLevel));

    SmokeCoAlarm::Attributes::ExpiryDate::Set(SMOKE_ALARM_ENDPOINT_ID, revd_smoke_co_alarm_data.ExpiryDate);
}

void Smoke_co_alarm_update_status(void)
{
    int FactoryReset = os_get_boot_arg_int("matter.factory_reset", 0);
    if ((FactoryReset != 0) || matterutils::IsNodeCommissioned() == false)
    {
	    /*  Ignore the Smoke-CO-Alarm data received from host if device is in factory reset OR not commissioned. */
	    return;
    }

    ChipLogProgress(Support, "[Smoke-CO-Alarm] Data received from host...");
    DeviceLayer::PlatformMgr().ScheduleWork(Update_SmokeCOAlarm_status_attributes, reinterpret_cast<intptr_t>(nullptr));

    ChipLogProgress(Support, "[Smoke-CO-Alarm] Status: ExpressedState: %d, SmokeState: %d, COState: %d, BatteryAlert: %d"
		    " DeviceMuted: %d, TestInProgress: %d, HardwareFaultAlert: %d, EndOfServiceAlert: %d, InterconnectSmokeAlarm: %d"
		    "InterconnectCOAlarm: %d, ContaminationState: %d, SmokeSensitivityLevel: %d, ExpiryDate: %d",
		    revd_smoke_co_alarm_data.ExpressedState, revd_smoke_co_alarm_data.ExpressedState, revd_smoke_co_alarm_data.COState,
		    revd_smoke_co_alarm_data.COState, revd_smoke_co_alarm_data.DeviceMuted, revd_smoke_co_alarm_data.TestInProgress,
		    revd_smoke_co_alarm_data.HardwareFaultAlert, revd_smoke_co_alarm_data.EndOfServiceAlert,
		    revd_smoke_co_alarm_data.InterconnectSmokeAlarm, revd_smoke_co_alarm_data.InterconnectCOAlarm,
		    revd_smoke_co_alarm_data.ContaminationState, revd_smoke_co_alarm_data.SmokeSensitivityLevel,
		    revd_smoke_co_alarm_data.ExpiryDate);
}

static void EndSelfTestingEventHandler(void)
{
    mSmokealarmInstance.SetTestInProgress(SMOKE_ALARM_ENDPOINT_ID, false);
    mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
}

#if TALARIA_TEST_EVENT_TRIGGER_ENABLED
static bool Talaria_SmokeCO_emberAfHandleEventTrigger(uint64_t eventTrigger)
{
    SmokeCOTrigger trigger = static_cast<SmokeCOTrigger>(eventTrigger);

    switch (trigger)
    {
    case SmokeCOTrigger::kForceSmokeCritical:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Force smoke (critical)");
        VerifyOrReturnValue(mSmokealarmInstance.SetSmokeState(SMOKE_ALARM_ENDPOINT_ID, AlarmStateEnum::kCritical), true);
        mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
        break;
    case SmokeCOTrigger::kForceSmokeWarning:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Force smoke (warning)");
        VerifyOrReturnValue(mSmokealarmInstance.SetSmokeState(SMOKE_ALARM_ENDPOINT_ID, AlarmStateEnum::kWarning), true);
        mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
        break;
    case SmokeCOTrigger::kForceSmokeInterconnect:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Force smoke interconnect (warning)");
        VerifyOrReturnValue(mSmokealarmInstance.SetInterconnectSmokeAlarm(SMOKE_ALARM_ENDPOINT_ID, AlarmStateEnum::kWarning), true);
        mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
        break;
    case SmokeCOTrigger::kForceCOCritical:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Force CO (critical)");
        VerifyOrReturnValue(mSmokealarmInstance.SetCOState(SMOKE_ALARM_ENDPOINT_ID, AlarmStateEnum::kCritical), true);
        mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
        break;
    case SmokeCOTrigger::kForceCOWarning:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Force CO (warning)");
        VerifyOrReturnValue(mSmokealarmInstance.SetCOState(SMOKE_ALARM_ENDPOINT_ID, AlarmStateEnum::kWarning), true);
        mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
        break;
    case SmokeCOTrigger::kForceCOInterconnect:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Force CO (warning)");
        VerifyOrReturnValue(mSmokealarmInstance.SetInterconnectCOAlarm(SMOKE_ALARM_ENDPOINT_ID, AlarmStateEnum::kWarning), true);
        mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
        break;
    case SmokeCOTrigger::kForceSmokeContaminationHigh:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Force smoke contamination (critical)");
        mSmokealarmInstance.SetContaminationState(SMOKE_ALARM_ENDPOINT_ID, ContaminationStateEnum::kCritical);
        break;
    case SmokeCOTrigger::kForceSmokeContaminationLow:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Force smoke contamination (warning)");
        mSmokealarmInstance.SetContaminationState(SMOKE_ALARM_ENDPOINT_ID, ContaminationStateEnum::kLow);
        break;
    case SmokeCOTrigger::kForceSmokeSensitivityHigh:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Force smoke sensistivity (high)");
        mSmokealarmInstance.SetSmokeSensitivityLevel(SMOKE_ALARM_ENDPOINT_ID, SensitivityEnum::kHigh);
        break;
    case SmokeCOTrigger::kForceSmokeSensitivityLow:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Force smoke sensitivity (low)");
        mSmokealarmInstance.SetSmokeSensitivityLevel(SMOKE_ALARM_ENDPOINT_ID, SensitivityEnum::kLow);
        break;
    case SmokeCOTrigger::kForceMalfunction:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Force malfunction");
        VerifyOrReturnValue(mSmokealarmInstance.SetHardwareFaultAlert(SMOKE_ALARM_ENDPOINT_ID, true), true);
        mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
        break;
    case SmokeCOTrigger::kForceLowBatteryWarning:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Force low battery (warning)");
        VerifyOrReturnValue(mSmokealarmInstance.SetBatteryAlert(SMOKE_ALARM_ENDPOINT_ID, AlarmStateEnum::kWarning), true);
        mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
        break;
    case SmokeCOTrigger::kForceLowBatteryCritical:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Force low battery (critical)");
        VerifyOrReturnValue(mSmokealarmInstance.SetBatteryAlert(SMOKE_ALARM_ENDPOINT_ID, AlarmStateEnum::kCritical), true);
        mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
        break;
    case SmokeCOTrigger::kForceEndOfLife:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Force end-of-life");
        VerifyOrReturnValue(mSmokealarmInstance.SetEndOfServiceAlert(SMOKE_ALARM_ENDPOINT_ID, EndOfServiceEnum::kExpired), true);
        mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
        break;
    case SmokeCOTrigger::kForceSilence:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Force silence");
        mSmokealarmInstance.SetDeviceMuted(SMOKE_ALARM_ENDPOINT_ID, MuteStateEnum::kMuted);
        break;
    case SmokeCOTrigger::kClearSmoke:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Clear smoke");
        VerifyOrReturnValue(mSmokealarmInstance.SetSmokeState(SMOKE_ALARM_ENDPOINT_ID, AlarmStateEnum::kNormal), true);
        mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
        break;
    case SmokeCOTrigger::kClearCO:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Clear CO");
        VerifyOrReturnValue(mSmokealarmInstance.SetCOState(SMOKE_ALARM_ENDPOINT_ID, AlarmStateEnum::kNormal), true);
        mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
        break;
    case SmokeCOTrigger::kClearSmokeInterconnect:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Clear smoke interconnect");
        VerifyOrReturnValue(mSmokealarmInstance.SetInterconnectSmokeAlarm(SMOKE_ALARM_ENDPOINT_ID, AlarmStateEnum::kNormal), true);
        mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
        break;
    case SmokeCOTrigger::kClearCOInterconnect:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Clear CO interconnect");
        VerifyOrReturnValue(mSmokealarmInstance.SetInterconnectCOAlarm(SMOKE_ALARM_ENDPOINT_ID, AlarmStateEnum::kNormal), true);
        mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
        break;
    case SmokeCOTrigger::kClearMalfunction:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Clear malfunction");
        VerifyOrReturnValue(mSmokealarmInstance.SetHardwareFaultAlert(SMOKE_ALARM_ENDPOINT_ID, false), true);
        mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
        break;
    case SmokeCOTrigger::kClearEndOfLife:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Clear end-of-life");
        VerifyOrReturnValue(mSmokealarmInstance.SetEndOfServiceAlert(SMOKE_ALARM_ENDPOINT_ID, EndOfServiceEnum::kNormal), true);
        mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
        break;
    case SmokeCOTrigger::kClearSilence:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Clear silence");
        mSmokealarmInstance.SetDeviceMuted(SMOKE_ALARM_ENDPOINT_ID, MuteStateEnum::kNotMuted);
        break;
    case SmokeCOTrigger::kClearBatteryLevelLow:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Clear low battery");
        VerifyOrReturnValue(mSmokealarmInstance.SetBatteryAlert(SMOKE_ALARM_ENDPOINT_ID, AlarmStateEnum::kNormal), true);
        mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
        break;
    case SmokeCOTrigger::kClearContamination:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Force SmokeContamination (warning)");
        mSmokealarmInstance.SetContaminationState(SMOKE_ALARM_ENDPOINT_ID, ContaminationStateEnum::kNormal);
        break;
    case SmokeCOTrigger::kClearSensitivity:
        ChipLogProgress(Support, "[Smoke-CO-Alarm-Test-Event] => Clear Smoke Sensitivity");
        mSmokealarmInstance.SetSmokeSensitivityLevel(SMOKE_ALARM_ENDPOINT_ID, SensitivityEnum::kStandard);
        break;
    default:
        ChipLogProgress(Support, "Not a Smoke-CO-Alarm-Test-Event");
        return false;
    }

    return true;
}

bool TalariaTestEventTriggerDelegate::DoesEnableKeyMatch(const ByteSpan & enableKey) const
{
    return !mEnableKey.empty() && mEnableKey.data_equal(enableKey);
}

CHIP_ERROR TalariaTestEventTriggerDelegate::HandleEventTrigger(uint64_t eventTrigger)
{
    bool success = Talaria_SmokeCO_emberAfHandleEventTrigger(eventTrigger);
    return success ? CHIP_NO_ERROR : CHIP_ERROR_INVALID_ARGUMENT;
}
#endif /* TALARIA_TEST_EVENT_TRIGGER_ENABLED */

static void Hanlder(chip::System::Layer * systemLayer, EndpointId endpointId)
{
    if (identifyTimerCount > 0)
    {
        /* Decrement the timer count. */
        identifyTimerCount--;

        /* Toggle LED. */
        os_gpio_toggle_pin(led_pin);
        chip::app::Clusters::Identify::Attributes::IdentifyTime::Set(endpointId, identifyTimerCount);
    }
}

static void OnIdentifyPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    identifyTimerCount = (*value);
    DeviceLayer::SystemLayer().CancelTimer(Hanlder, endpointId);
    DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Milliseconds64(IDENTIFY_TIMER_DELAY_MS), Hanlder, endpointId);
}

static void PostAttributeChangeCallback(EndpointId endpointId, ClusterId clusterId, AttributeId attributeId, uint8_t type,
                                        uint16_t size, uint8_t * value)
{
    ChipLogDetail(AppServer,
                  "PostAttributeChangeCallback - Cluster ID: '0x%" PRIx32 "', EndPoint ID: '0x%x', Attribute ID: '0x%" PRIx32 "'",
                  clusterId, endpointId, attributeId);

    if (clusterId == SmokeCoAlarm::Id)
    {
	VerifyOrExit(endpointId == 1, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));
	switch(attributeId)
	{
	case SmokeCoAlarm::Attributes::ExpressedState::Id:
	    ChipLogProgress(AppServer, "[Smoke CO Alarm] ExpressedState attribute set to %d", *value);
	    break;
	case SmokeCoAlarm::Attributes::SmokeState::Id:
	    ChipLogProgress(AppServer, "[Smoke CO Alarm] SmokeState attribute set to %d", *value);
	    break;
	case SmokeCoAlarm::Attributes::COState::Id:
	    ChipLogProgress(AppServer, "[Smoke CO Alarm] COState attribute set to %d", *value);
	    break;
	case SmokeCoAlarm::Attributes::BatteryAlert::Id:
	    ChipLogProgress(AppServer, "[Smoke CO Alarm] BatteryAlert attribute set to %d", *value);
	    break;
	case SmokeCoAlarm::Attributes::DeviceMuted::Id:
	    ChipLogProgress(AppServer, "[Smoke CO Alarm] DeviceMuted attribute set to %d", *value);
	    break;
	case SmokeCoAlarm::Attributes::TestInProgress::Id:
	    ChipLogProgress(AppServer, "[Smoke CO Alarm] TestInProgress attribute set to %d", *value);
	    break;
	case SmokeCoAlarm::Attributes::HardwareFaultAlert::Id:
	    ChipLogProgress(AppServer, "[Smoke CO Alarm] HardwareFaultAlert attribute set to %d", *value);
	    break;
	case SmokeCoAlarm::Attributes::EndOfServiceAlert::Id:
	    ChipLogProgress(AppServer, "[Smoke CO Alarm] EndOfServiceAlert attribute set to %d", *value);
	    break;
	case SmokeCoAlarm::Attributes::InterconnectSmokeAlarm::Id:
	    ChipLogProgress(AppServer, "[Smoke CO Alarm] InterconnectSmokeAlarm attribute set to %d", *value);
	    break;
	case SmokeCoAlarm::Attributes::InterconnectCOAlarm::Id:
	    ChipLogProgress(AppServer, "[Smoke CO Alarm] InterconnectCOAlarm attribute set to %d", *value);
	    break;
	case SmokeCoAlarm::Attributes::ContaminationState::Id:
	    ChipLogProgress(AppServer, "[Smoke CO Alarm] ContaminationState attribute set to %d", *value);
	    break;
	case SmokeCoAlarm::Attributes::SmokeSensitivityLevel::Id:
	    ChipLogProgress(AppServer, "[Smoke CO Alarm] SmokeSensitivityLevel attribute set to %d", *value);
	    break;
	case SmokeCoAlarm::Attributes::ExpiryDate::Id:
	    ChipLogProgress(AppServer, "[Smoke CO Alarm] ExpiryDate attribute set to %d", *value);
	    break;
	 default:
	    return;
	}
    }
    else if (clusterId == Identify::Id)
    {
        OnIdentifyPostAttributeChangeCallback(endpointId, attributeId, value);
    }

    os_printf("\nCluster control command addressed. os_free_heap(): %d", os_avail_heap());
exit:
    return;
}

/** @brief  Smoke CO Alarm Cluster Init
 *
 * This function is called when a specific cluster is initialized. It gives the
 * application an opportunity to take care of cluster initialization procedures.
 * It is called exactly once for each endpoint where cluster is present.
 *
 * @param endpoint   Ver.: always
 *
 * emberAfSmokeCoAlarmClusterInitCallback happens before the stack initialize the cluster
 * attributes to the default value.
 *
 */
void emberAfSmokeCoAlarmClusterInitCallback(EndpointId endpoint)
{
    ChipLogDetail(AppServer, "emberAfSmokeCoAlarmClusterInitCallback");
}

void emberAfIdentifyClusterInitCallback(chip::EndpointId endpoint)
{
    Identify::Attributes::IdentifyType::Set(endpoint, Identify::IdentifyTypeEnum::kLightOutput);
    Identify::Attributes::IdentifyTime::Set(endpoint, 0);
    ChipLogDetail(AppServer, "emberAfIdentifyClusterInitCallback");
}

static void SelfTesting_EventHandler(void)
{
    ChipLogProgress(AppServer, "[Smoke-CO-Alarm] SelfTesting command received");
    struct  smoke_co_alarm_set_data input_data;
    int payload = sizeof(struct smoke_co_alarm_set_data);

    int ret = matter_notify(SMOKE_CO_ALARM, SMOKE_CO_ALARM_SELF_TEST_REQUEST, payload, (struct smoke_co_alarm_set_data *) &input_data);
    if(ret != 0)
    {
            ChipLogError(AppServer, "[Smoke-CO-Alarm] SelfTesting command failed, error: %d", ret);
    }
    else
    {
	    EndSelfTestingEventHandler();
	    ChipLogProgress(AppServer, "[Smoke-CO-Alarm] SelfTesting command successfull");
    }
}

void emberAfPluginSmokeCoAlarmSelfTestRequestCommand(EndpointId endpointId)
{
    SelfTesting_EventHandler();
}

void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath & path, uint8_t type, uint16_t size, uint8_t * value)
{
    TaskHandle_t task = xTaskGetCurrentTaskHandle();
    const char * name = pcTaskGetName(task);
    if (strcmp(name, "CHIP"))
    {
        ChipLogError(AppServer, "application, Attribute changed on non-Matter task '%s'\n", name);
    }

    PostAttributeChangeCallback(path.mEndpointId, path.mClusterId, path.mAttributeId, type, size, value);
}

#ifdef UNIT_TEST
void run_unit_test(void)
{
    os_printf("|*********** **************** ***********|");
    os_printf("|*********** TEST APPLICATION ***********V");

    os_printf("!!!!!!!!!! HEAP SIZE = %d\n", xPortGetFreeHeapSize());


    vTaskDelay(1000);
    os_printf("!!!!!!!!!! HEAP SIZE = %d\n", xPortGetFreeHeapSize());
    os_printf("");
    os_printf("|********** END OF APPLICATION *********|");
    os_printf("|********** ****************** *********|");

    os_printf("\n");
    while (1)
    {
        // never return from unit test
        vTaskDelay(10000);
    }
}

void print_test_results(nlTestSuite * tSuite)
{
    os_printf("\n\n\
            ******************************\n \
            TEST RESULTS : %s\n\
            \t total tests      : %d \n\
            \t failed           : %d\n\
            \t Assertions       : %d\n\
            \t failed           : %d\n\
            ****************************** %s \n\n",
              tSuite->name, tSuite->runTests, tSuite->failedTests, tSuite->performedAssertions, tSuite->failedAssertions,
              tSuite->flagError ? "FAIL" : "PASS");
}
#endif

int main(void)
{
    talariautils::ApplicationInitLog("matter Smoke CO Alarm app");

    matter_hio_init();
    /* Delay is required before start doing the communication over hio,
       otherwise don't see any response*/
    vTaskDelay(pdMS_TO_TICKS(1000));

    int FactoryReset = os_get_boot_arg_int("matter.factory_reset", 0);
    if (FactoryReset == 1 || FactoryReset == 2)
    {
        talariautils::FactoryReset(FactoryReset);
        while (1)
            vTaskSuspend(NULL);
    }
#ifdef UNIT_TEST
    run_unit_test();
#endif
    talariautils::EnableSuspend();

    DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);
    led_pin = GPIO_PIN(LED_PIN);
    os_gpio_request(led_pin);
    os_gpio_set_output(led_pin);
    os_gpio_clr_pin(led_pin);

    ServerInitDone = xSemaphoreCreateCounting(1, 0);
    app_test();

    if (xSemaphoreTake(ServerInitDone, portMAX_DELAY) == pdFAIL)
    {
        os_printf("Unable to wait on semaphore...!!\n");
    }

    os_printf("After Server initialization completed. os_free_heap(): %d\n", os_avail_heap());
    os_printf("\n");


    /* There is nothing to do in this task. Putting the task in suspend mode */
    vTaskSuspend(NULL);
    return 0;
}
