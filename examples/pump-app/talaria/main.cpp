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
#include <app/clusters/identify-server/identify-server.h>

/* Periodic timeout for Flow Sensor software timer in ms */
#define FLOW_SENSOR_PERIODIC_TIME_OUT_MS 10000
#define IDENTIFY_TIMER_DELAY_MS  1000
#define LED_PIN 14
#define PUMP_APP_MIN_MAX_CONTROL_GPIO 18

using namespace chip;
using namespace chip::Platform;
using namespace chip::DeviceLayer;
using namespace chip::Credentials;
using namespace chip::app::Clusters;
using namespace chip::talaria;
using namespace chip::app::Clusters::OnOff;
using namespace chip::app::Clusters::LevelControl;
using namespace chip::app::Clusters::Identify;
using namespace chip::app::Clusters::FlowMeasurement;
using namespace chip::app::Clusters::PumpConfigurationAndControl;

DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

static void OnIdentifyStart(struct Identify *identify);
static void OnIdentifyStop(struct Identify * identify);
static void OnTriggerIdentifyEffect(struct Identify *identify);

chip::app::Clusters::Identify::EffectIdentifierEnum sIdentifyEffect = chip::app::Clusters::Identify::EffectIdentifierEnum::kStopEffect;
struct Identify gIdentify = {
    chip::EndpointId{ 1 },
    OnIdentifyStart,
    OnIdentifyStop,
    chip::app::Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator,
    OnTriggerIdentifyEffect,
};
/*-----------------------------------------------------------*/

void print_test_results(nlTestSuite * tSuite);
void app_test();

struct Flow_Sensor_Mesurement
{
    uint16_t MeasuredValue;
    uint16_t MinMeasuredValue;
    uint16_t MaxMeasuredValue;
    uint16_t Tolerance;
};

int SoftwareTimer_Init(void);
uint32_t led_pin;
static uint32_t identifyTimerCount = 0;
static uint32_t pump_control_gpio;

/* Function Declarations */

static void Update_FlowSensor_Mesurement_status_attributes(struct Flow_Sensor_Mesurement* flow_measurement_data)
{
    /* Update the Matter flow measurement sensor attribute values. */
    FlowMeasurement::Attributes::MeasuredValue::Set(1, flow_measurement_data->MeasuredValue);
    FlowMeasurement::Attributes::MinMeasuredValue::Set(1, flow_measurement_data->MinMeasuredValue);
    FlowMeasurement::Attributes::MaxMeasuredValue::Set(1, flow_measurement_data->MaxMeasuredValue);
    FlowMeasurement::Attributes::Tolerance::Set(1, flow_measurement_data->Tolerance);
}

static void Read_And_Update_FlowSensor_Measurements(intptr_t arg)
{
    struct Flow_Sensor_Mesurement flow_measurement_data;
    /* PlaceHolder: To call vendor-specific API to read and update
     * actual flow sensor measurement values instead of random
     * flow sensor measurement values.
     * The rand function generates random flow sensor measurement values
     * between 0 and 100 as actual flow measurement sensor is not available. */
    int state = (os_gpio_get_value(pump_control_gpio) > 0) ?  1 : 0;
    if ( state > 0)
    {
        flow_measurement_data.MeasuredValue = 65534;
        flow_measurement_data.MinMeasuredValue = 0;
        flow_measurement_data.MaxMeasuredValue = 65534;
        flow_measurement_data.Tolerance = rand() % (2048);
        Update_FlowSensor_Mesurement_status_attributes(&flow_measurement_data);
    }
    else
    {
        flow_measurement_data.MeasuredValue = 0 ;
        flow_measurement_data.MinMeasuredValue = 0;
        flow_measurement_data.MaxMeasuredValue = 65534;
        flow_measurement_data.Tolerance = rand() % (2048);
        Update_FlowSensor_Mesurement_status_attributes(&flow_measurement_data);
    }
}

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

static void OnOffPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    BitMask<PumpConfigurationAndControl::PumpStatusBitmap> pumpStatus;
    if (attributeId == OnOff::Attributes::OnOff::Id)
    {
	    if (*value)
	    {
		    /* PlaceHolder: To call vendor-specific API
		     * to handle pump-app Turn-ON Command. */
		    ChipLogProgress(AppServer, "[pump-app] ON Command Received");
		    pumpStatus.Set(PumpConfigurationAndControl::PumpStatusBitmap::kRunning);
	    }
	    else
	    {
		    /* PlaceHolder: To call vendor-specific API
		     * to handle pump-app Turn-OFF Command. */
		    ChipLogProgress(AppServer, "[pump-app] OFF Command Received");
		    pumpStatus.Clear(PumpConfigurationAndControl::PumpStatusBitmap::kRunning);
	    }
	    PumpConfigurationAndControl::Attributes::PumpStatus::Set(1, pumpStatus);
    }
}

static void OnLevelControlPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    if (attributeId == LevelControl::Attributes::CurrentLevel::Id)
    {
	    ChipLogProgress(AppServer, "[pump-app] LevelControl: attribute CurrentLevel set to %u", *value);
    }
}

static void OnIdentifyPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    identifyTimerCount = (*value);
    DeviceLayer::SystemLayer().CancelTimer(Hanlder, endpointId);
    DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Milliseconds64(IDENTIFY_TIMER_DELAY_MS), Hanlder, endpointId);
}

static void OnFlowMeasurementPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    if (attributeId == FlowMeasurement::Attributes::MeasuredValue::Id)
    {
	    ChipLogProgress(AppServer, "[FlowMeasurement] MeasuredValue set to %u", *value);
    }
    else if (attributeId == FlowMeasurement::Attributes::MinMeasuredValue::Id)
    {
	    ChipLogProgress(AppServer, "[FlowMeasurement] MinMeasuredValue set to %u", *value);
    }
    else if (attributeId == FlowMeasurement::Attributes::MaxMeasuredValue::Id)
    {
	    ChipLogProgress(AppServer, "[FlowMeasurement] MaxMeasuredValue set to %u", *value);
    }
    else if (attributeId == FlowMeasurement::Attributes::Tolerance::Id)
    {
	    ChipLogProgress(AppServer, "[FlowMeasurement] Tolerance set to %u", *value);
    }
}

static void OnPumpConfigurationAndControlPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    ChipLogProgress(AppServer, "PumpConfigurationAndControl Cluster: attribute '0x%x', value set to %u", attributeId, *value);
}

static void vTimerCallback_flowsensor(TimerHandle_t xTimer)
{
    DeviceLayer::PlatformMgr().ScheduleWork(Read_And_Update_FlowSensor_Measurements, reinterpret_cast<intptr_t>(nullptr));
}

int SoftwareTimer_Init()
{
    static TimerHandle_t xTimer = NULL;

    if (xTimer != NULL)
	    return 0;

    xTimer = xTimerCreate("flow_sensor", pdMS_TO_TICKS(FLOW_SENSOR_PERIODIC_TIME_OUT_MS), pdTRUE, (void *) 0, vTimerCallback_flowsensor);
    if (xTimer == NULL)
    {
	    os_printf("\nflow_sensor timer creation failed");
	    return -1;
    }
    if (xTimerStart(xTimer, 0) != pdPASS)
    {
	    os_printf("\nflow_sensor timer start failed");
	    return -1;
    }
    return 0;
}

static void PostAttributeChangeCallback(EndpointId endpointId, ClusterId clusterId, AttributeId attributeId, uint8_t type,
                                        uint16_t size, uint8_t * value)
{
    ChipLogDetail(AppServer,
                  "PostAttributeChangeCallback - Cluster ID: '0x%" PRIx32 "', EndPoint ID: '0x%x', Attribute ID: '0x%" PRIx32 "'",
                  clusterId, endpointId, attributeId);
    VerifyOrExit(endpointId == 1, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));

    switch (clusterId)
    {
    case OnOff::Id:
	OnOffPostAttributeChangeCallback(endpointId, attributeId, value);
	break;
    case LevelControl::Id:
	OnLevelControlPostAttributeChangeCallback(endpointId, attributeId, value);
	break;
    case Identify::Id:
	OnIdentifyPostAttributeChangeCallback(endpointId, attributeId, value);
	break;
    case FlowMeasurement::Id:
	OnFlowMeasurementPostAttributeChangeCallback(endpointId, attributeId, value);
	break;
    case PumpConfigurationAndControl::Id:
	OnPumpConfigurationAndControlPostAttributeChangeCallback(endpointId, attributeId, value);
	break;
    default:
	break;
    }

    os_printf("\nCluster control command addressed. os_free_heap(): %d", os_avail_heap());
exit:
    return;
}

/** @brief  FlowMesurement Cluster Init
 *
 * This function is called when a specific cluster is initialized. It gives the
 * application an opportunity to take care of cluster initialization procedures.
 * It is called exactly once for each endpoint where cluster is present.
 *
 * @param endpoint   Ver.: always
 *
 * emberAfFlowMeasurementClusterInitCallback happens before the stack initialize the cluster
 * attributes to the default value.
 *
 */
void emberAfFlowMeasurementClusterInitCallback(EndpointId endpoint)
{
    ChipLogDetail(AppServer, "emberAfFlowMeasurementClusterInitCallback");
}

void emberAfPressureMeasurementClusterInitCallback(EndpointId endpoint)
{
    ChipLogDetail(AppServer, "emberAfPressureMeasurementClusterInitCallback");
}

void emberAfOnOffClusterInitCallback(EndpointId endpoint)
{
    ChipLogDetail(AppServer, "emberAfOnOffClusterInitCallback");
}

void emberAfIdentifyClusterInitCallback(chip::EndpointId endpoint)
{
    Identify::Attributes::IdentifyType::Set(endpoint, Identify::IdentifyTypeEnum::kLightOutput);
    Identify::Attributes::IdentifyTime::Set(endpoint, 0);
    ChipLogDetail(AppServer, "emberAfIdentifyClusterInitCallback");
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

static void OnIdentifyStart(struct Identify *identify)
{
    /* For Identify Start setting some max number of seconds e.g. 10 */
    identifyTimerCount = 10;
    DeviceLayer::SystemLayer().CancelTimer(Hanlder, identify->mEndpoint);
    DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Milliseconds64(IDENTIFY_TIMER_DELAY_MS), Hanlder, identify->mEndpoint);
}

static void OnIdentifyStop(struct Identify * identify)
{
    /* On next timer handler call it will stop identify */
    identifyTimerCount = 0;
}

static void OnTriggerIdentifyEffectCompleted(chip::System::Layer * layer, void * appState)
{
    ChipLogProgress(AppServer, "Trigger Identify Complete");
    sIdentifyEffect = chip::app::Clusters::Identify::EffectIdentifierEnum::kStopEffect;

    OnIdentifyStop(NULL);
}

static void OnTriggerIdentifyEffect(struct Identify *identify)
{
    sIdentifyEffect = identify->mCurrentEffectIdentifier;

    if (identify->mEffectVariant != chip::app::Clusters::Identify::EffectVariantEnum::kDefault)
    {
        ChipLogDetail(AppServer, "Identify Effect Variant unsupported. Using default");
    }

    OnIdentifyStart(identify);

    switch (sIdentifyEffect)
    {
    case chip::app::Clusters::Identify::EffectIdentifierEnum::kBlink:
    case chip::app::Clusters::Identify::EffectIdentifierEnum::kOkay:
        (void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(5), OnTriggerIdentifyEffectCompleted,
                                                           identify);
        break;
    case chip::app::Clusters::Identify::EffectIdentifierEnum::kBreathe:
    case chip::app::Clusters::Identify::EffectIdentifierEnum::kChannelChange:
        (void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(10), OnTriggerIdentifyEffectCompleted,
                                                           identify);
        break;
    case chip::app::Clusters::Identify::EffectIdentifierEnum::kFinishEffect:
        (void) chip::DeviceLayer::SystemLayer().CancelTimer(OnTriggerIdentifyEffectCompleted, identify);
        (void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(1), OnTriggerIdentifyEffectCompleted,
                                                           identify);
        break;
    case chip::app::Clusters::Identify::EffectIdentifierEnum::kStopEffect:
        (void) chip::DeviceLayer::SystemLayer().CancelTimer(OnTriggerIdentifyEffectCompleted, identify);
        break;
    default:
        sIdentifyEffect = chip::app::Clusters::Identify::EffectIdentifierEnum::kStopEffect;
        ChipLogProgress(Zcl, "No identifier effect");
    }
}

#ifdef UNIT_TEST
void run_unit_test(void)
{
    os_printf("|*********** **************** ***********|");
    os_printf("|*********** TEST APPLICATION ***********V");

    os_printf("!!!!!!!!!! HEAP SIZE = %d\n", xPortGetFreeHeapSize());

    // test_suit_proc();
    // xTaskCreate( test_suit_proc, "unit_test_task", 12000, NULL, 5, NULL);

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

    talariautils::ApplicationInitLog("matter flow sensor app");
    talariautils::EnableSuspend();

    DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);
    led_pin = GPIO_PIN(LED_PIN);
    os_gpio_request(led_pin);
    os_gpio_set_output(led_pin);
    os_gpio_clr_pin(led_pin);
    pump_control_gpio = GPIO_PIN(PUMP_APP_MIN_MAX_CONTROL_GPIO);

    /* Requesting for Gpio pin */
    os_gpio_request(pump_control_gpio);

    /* Setting Direction as Input */
    os_gpio_set_input(pump_control_gpio);
    app_test();

    /* There is nothing to do in this task. Putting the task in suspend mode */
    vTaskSuspend(NULL);
    return 0;
}
