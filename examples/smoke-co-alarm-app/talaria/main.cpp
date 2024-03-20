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

/* Periodic timeout for Smoke Alaram software timer in ms */
#define SMOKE_ALARM_PERIODIC_TIME_OUT_MS 5000
#define SMOKE_ALARM_ENDPOINT_ID 1

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

/* Function Declarations */

static SmokeCoAlarmServer mSmokealarmInstance = SmokeCoAlarmServer::Instance();
static std::array<ExpressedStateEnum, SmokeCoAlarmServer::kPriorityOrderLength> sPriorityOrder = {
    ExpressedStateEnum::kSmokeAlarm,     ExpressedStateEnum::kInterconnectSmoke, ExpressedStateEnum::kCOAlarm,
    ExpressedStateEnum::kInterconnectCO, ExpressedStateEnum::kHardwareFault,     ExpressedStateEnum::kTesting,
    ExpressedStateEnum::kEndOfService,   ExpressedStateEnum::kBatteryAlert
};
struct  SmokeCO_alarm_data
{
    AlarmStateEnum newSmokeState;
    AlarmStateEnum newCOState;
    AlarmStateEnum newBatteryAlert;
    MuteStateEnum newDeviceMuted;
    bool newTestInProgress;
    bool newHardwareFaultAlert;
    EndOfServiceEnum newEndOfServiceAlert;
    AlarmStateEnum newInterconnectSmokeAlarm;
    AlarmStateEnum newInterconnectCOAlarm;
    ContaminationStateEnum newContaminationState;
    SensitivityEnum newSmokeSensitivityLevel;
    uint32_t expiryDate;
}SmokeCO_alarm_status;

static uint8_t generate_random_values(uint8_t range)
{
    return rand() % range + 1;
}

static void Update_SmokeCOAlarm_status_values(void)
{
    mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
    SmokeCoAlarm::Attributes::SmokeState::Set(SMOKE_ALARM_ENDPOINT_ID, SmokeCO_alarm_status.newSmokeState);
    SmokeCoAlarm::Attributes::COState::Set(SMOKE_ALARM_ENDPOINT_ID, SmokeCO_alarm_status.newCOState);
    SmokeCoAlarm::Attributes::BatteryAlert::Set(SMOKE_ALARM_ENDPOINT_ID, SmokeCO_alarm_status.newBatteryAlert);
    SmokeCoAlarm::Attributes::DeviceMuted::Set(SMOKE_ALARM_ENDPOINT_ID, SmokeCO_alarm_status.newDeviceMuted);
    SmokeCoAlarm::Attributes::TestInProgress::Set(SMOKE_ALARM_ENDPOINT_ID, SmokeCO_alarm_status.newTestInProgress);
    SmokeCoAlarm::Attributes::HardwareFaultAlert::Set(SMOKE_ALARM_ENDPOINT_ID, SmokeCO_alarm_status.newHardwareFaultAlert);
    SmokeCoAlarm::Attributes::EndOfServiceAlert::Set(SMOKE_ALARM_ENDPOINT_ID, SmokeCO_alarm_status.newEndOfServiceAlert);
    SmokeCoAlarm::Attributes::InterconnectSmokeAlarm::Set(SMOKE_ALARM_ENDPOINT_ID, SmokeCO_alarm_status.newInterconnectSmokeAlarm);
    SmokeCoAlarm::Attributes::InterconnectCOAlarm::Set(SMOKE_ALARM_ENDPOINT_ID, SmokeCO_alarm_status.newInterconnectCOAlarm);
    SmokeCoAlarm::Attributes::ContaminationState::Set(SMOKE_ALARM_ENDPOINT_ID, SmokeCO_alarm_status.newContaminationState);
    SmokeCoAlarm::Attributes::SmokeSensitivityLevel::Set(SMOKE_ALARM_ENDPOINT_ID, SmokeCO_alarm_status.newSmokeSensitivityLevel);
    SmokeCoAlarm::Attributes::ExpiryDate::Set(SMOKE_ALARM_ENDPOINT_ID, SmokeCO_alarm_status.expiryDate);
}

static void Read_SmokeCOAlarm_status_values(void)
{
    /* Set SmokeState/COState/InterconnectSmokeAlarm/InterconnectCOAlarm
     * status to 0(normal) value, TestInProgress to false value,
     * EndOfServiceAlert to 0 (normal) value to make
     * self test command work else self test command gives
     * Busy response error as SmokeCO alarm is not in normal mode. */

    /* update SmokeState status value.*/
    SmokeCO_alarm_status.newSmokeState = static_cast<AlarmStateEnum>(0);

    /* update COState status value. */
    SmokeCO_alarm_status.newCOState = static_cast<AlarmStateEnum>(0);

    /* update BatteryAlert status value. */
    SmokeCO_alarm_status.newBatteryAlert = static_cast<AlarmStateEnum>(generate_random_values(2));

    /* update DeviceMuted status value. */
    SmokeCO_alarm_status.newDeviceMuted = static_cast<MuteStateEnum>(generate_random_values(1));

    /* update TestInProgress status value. */
    SmokeCO_alarm_status.newTestInProgress = false;

    /* update HardwareFaultAlert status value. */
    SmokeCO_alarm_status.newHardwareFaultAlert = static_cast<bool>(generate_random_values(1));

    /* update EndOfServiceAlert status value. */
    SmokeCO_alarm_status.newEndOfServiceAlert = static_cast<EndOfServiceEnum>(0);

    /* update InterconnectSmokeAlarm status value. */
    SmokeCO_alarm_status.newInterconnectSmokeAlarm = static_cast<AlarmStateEnum>(0);

    /* update InterconnectCOAlarm status value. */
    SmokeCO_alarm_status.newInterconnectCOAlarm = static_cast<AlarmStateEnum>(0);

    /* update ContaminationState status value. */
    SmokeCO_alarm_status.newContaminationState = static_cast<ContaminationStateEnum>(generate_random_values(3));

    /* update SmokeSensitivityLevel status value. */
    SmokeCO_alarm_status.newSmokeSensitivityLevel = static_cast<SensitivityEnum>(generate_random_values(2));

    /* update expiryDate status value in months. */
    SmokeCO_alarm_status.expiryDate = static_cast<uint32_t>(generate_random_values(12));
}

static void EndSelfTestingEventHandler(void)
{
    mSmokealarmInstance.SetTestInProgress(SMOKE_ALARM_ENDPOINT_ID, false);
    mSmokealarmInstance.SetExpressedStateByPriority(SMOKE_ALARM_ENDPOINT_ID, sPriorityOrder);
}

static void Read_and_update_SmokeCOAlarm_status(intptr_t arg)
{
    /* PlaceHolder: To call vendor-specific APIs to read the Smoke CO Alarm attribute values */
    Read_SmokeCOAlarm_status_values();

    /* Update the Smoke CO Alarm attribute values */
    Update_SmokeCOAlarm_status_values();
}

static void vTimerCallback_smoke_co_alarm(TimerHandle_t xTimer)
{
    DeviceLayer::PlatformMgr().ScheduleWork(Read_and_update_SmokeCOAlarm_status, reinterpret_cast<intptr_t>(nullptr));
}

int SoftwareTimer_Init()
{
    static TimerHandle_t xTimer = NULL;

    if (xTimer != NULL)
	    return 0;

    xTimer = xTimerCreate("smoke_co_alarm_timer", pdMS_TO_TICKS(SMOKE_ALARM_PERIODIC_TIME_OUT_MS),
		          pdTRUE, (void *) 0, vTimerCallback_smoke_co_alarm);
    if (xTimer == NULL)
    {
	    ChipLogError(AppServer, "smoke_co_alarm_timer creation failed");
	    return -1;
    }
    if (xTimerStart(xTimer, 0) != pdPASS)
    {
	    ChipLogError(AppServer, "smoke_co_alarm_timer start failed");
	    return -1;
    }
    return 0;
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

static void PostAttributeChangeCallback(EndpointId endpointId, ClusterId clusterId, AttributeId attributeId, uint8_t type,
                                        uint16_t size, uint8_t * value)
{
    ChipLogDetail(AppServer,
                  "PostAttributeChangeCallback - Cluster ID: '0x%" PRIx32 "', EndPoint ID: '0x%x', Attribute ID: '0x%" PRIx32 "'",
                  clusterId, endpointId, attributeId);

    if (clusterId == SmokeCoAlarm::Id &&  attributeId == SmokeCoAlarm::Attributes::ExpressedState::Id)
    {
	VerifyOrExit(endpointId == 1, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));
	SmokeCoAlarm::ExpressedStateEnum expressedState = *(reinterpret_cast<SmokeCoAlarm::ExpressedStateEnum *>(value));
	ChipLogProgress(AppServer, "Smoke CO Alarm cluster: attribute state set to %d", to_underlying(expressedState));
    }
    else
    {
        return;
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
    ChipLogError(AppServer, "emberAfSmokeCoAlarmClusterInitCallback");
}

static void SelfTesting_EventHandler(void)
{
    /* PlaceHolder: To call vendor-specific API to handle the SmokeCoAlarm SelfTest Request command.
     * EndSelfTest event handler will be called post handling of SelfTest Request command. */

    EndSelfTestingEventHandler();
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
    int FactoryReset = os_get_boot_arg_int("matter.factory_reset", 0);
    if (FactoryReset == 1 || FactoryReset == 2)
    {
        talariautils::FactoryReset(FactoryReset);
        while (1)
            vTaskDelay(100000);
    }
#ifdef UNIT_TEST
    run_unit_test();
#endif

    talariautils::ApplicationInitLog("matter Smoke CO Alarm app");
    talariautils::EnableSuspend();

    DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

    app_test();

    /* There is nothing to do in this task. Putting the task in suspend mode */
    vTaskSuspend(NULL);
    return 0;
}
