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
#include <app/clusters/air-quality-server/air-quality-server.h>
#include <app/clusters/identify-server/identify-server.h>

/* Periodic timeout for AQS software timer in ms */
#define AQS_PERIODIC_TIME_OUT_MS 5000

#define IDENTIFY_TIMER_DELAY_MS  1000
#define LED_PIN 14
#define TEMP_HUMIDITY_APP_MIN_MAX_CONTROL_GPIO 18
#define TEMP_MIN_MEASURED_VALUE -27315
#define TEMP_MAX_MEASURED_VALUE 32767
#define HUMIDITY_MIN_MEASURED_VALUE 0
#define HUMIDITY_MAX_MEASURED_VALUE 65534

using namespace chip;
using namespace chip::Platform;
using namespace chip::DeviceLayer;
using namespace chip::Credentials;
using namespace chip::app::Clusters;
using namespace chip::talaria;
using namespace chip::app::Clusters::AirQuality;
using namespace chip::app::Clusters::Identify;

DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

/*-----------------------------------------------------------*/

void print_test_results(nlTestSuite * tSuite);
void app_test();

int SoftwareTimer_Init(void);

static chip::BitMask<Feature, uint32_t> airQualityFeatures(Feature::kFair, Feature::kModerate, Feature::kVeryPoor,
		                                           Feature::kExtremelyPoor);
static Instance mAirQualityInstance = Instance(1, airQualityFeatures);
AirQualityEnum airQualityStatus;

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

static uint32_t identifyTimerCount = 0;
uint32_t led_pin;
static uint32_t temp_humidity_control_gpio;

/* Function Declarations */

static AirQualityEnum Check_and_Calculate_AirQuality_Index(void)
{
    int index = 0;

    /* rand function generates random air quality index values between 0 and 500
     * and update the air quality status value as per the range. */
    index = rand() % 500 + 1;

    if (index >= 0 && index <= 50)
	    airQualityStatus = AirQualityEnum::kGood;
    else if (index > 50 && index <= 100)
	    airQualityStatus = AirQualityEnum::kFair;
    else if (index > 100 && index <= 150)
	    airQualityStatus = AirQualityEnum::kModerate;
    else if (index > 150 && index <= 200)
	    airQualityStatus = AirQualityEnum::kPoor;
    else if (index > 200 && index <= 300)
	    airQualityStatus = AirQualityEnum::kVeryPoor;
    else if (index > 300)
	    airQualityStatus = AirQualityEnum::kExtremelyPoor;
    else
	    airQualityStatus = AirQualityEnum::kUnknown;

    return airQualityStatus;
}

/* This API updates the Matter AirQuality attribute. */
static void Update_AirQuality_status(intptr_t arg)
{
    /* PlaceHolder to call vendor-specific API
     * instead of Check_and_Calculate_AirQuality_Index function to read
     * current air quality index value from air quality sensor and
     * matching it with appropriate AirQualityEnum and assign to
     * airQualityStatus variable.*/
    AirQualityEnum Status = Check_and_Calculate_AirQuality_Index();

    mAirQualityInstance.UpdateAirQuality(Status);
}

static void vTimerCallback_aqs(TimerHandle_t xTimer)
{
    DeviceLayer::PlatformMgr().ScheduleWork(Update_AirQuality_status, reinterpret_cast<intptr_t>(nullptr));
}

static void UpdateClusters(intptr_t arg)
{
    chip::app::Clusters::TemperatureMeasurement::Attributes::MinMeasuredValue::Set(1, TEMP_MIN_MEASURED_VALUE);
    chip::app::Clusters::TemperatureMeasurement::Attributes::MaxMeasuredValue::Set(1, TEMP_MAX_MEASURED_VALUE);

    chip::app::Clusters::RelativeHumidityMeasurement::Attributes::MinMeasuredValue::Set(1, HUMIDITY_MIN_MEASURED_VALUE);
    chip::app::Clusters::RelativeHumidityMeasurement::Attributes::MaxMeasuredValue::Set(1, HUMIDITY_MAX_MEASURED_VALUE);

    int state = (os_gpio_get_value(temp_humidity_control_gpio) > 0) ?  1 : 0;
    if (state > 0)
    {
        chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Set(1, TEMP_MAX_MEASURED_VALUE);
        chip::app::Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Set(1, HUMIDITY_MAX_MEASURED_VALUE);
    }
    else
    {
        chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Set(1, TEMP_MIN_MEASURED_VALUE);
        chip::app::Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Set(1, HUMIDITY_MIN_MEASURED_VALUE);
    }

}

static void UpdateTimer(TimerHandle_t xTimer)
{
    chip::DeviceLayer::PlatformMgr().ScheduleWork(UpdateClusters, reinterpret_cast<intptr_t>(nullptr));
}

int SoftwareTimer_Init()
{
    static TimerHandle_t xTimer = NULL;
    static TimerHandle_t xTimerUpdate = NULL;

    if (xTimer != NULL)
	    return 0;

    xTimer = xTimerCreate("aqs_timer", pdMS_TO_TICKS(AQS_PERIODIC_TIME_OUT_MS), pdTRUE, (void *) 0, vTimerCallback_aqs);
    if (xTimer == NULL)
    {
	    os_printf("\naqs_timer creation failed");
	    return -1;
    }
    if (xTimerStart(xTimer, 0) != pdPASS)
    {
	    os_printf("\naqs_timer start failed");
	    return -1;
    }
    xTimerUpdate = xTimerCreate("aqs_timer", pdMS_TO_TICKS(AQS_PERIODIC_TIME_OUT_MS), pdTRUE, (void *) 0, UpdateTimer);
    if (xTimerUpdate == NULL)
    {
	    os_printf("\naqs_timer creation failed");
	    return -1;
    }
    if (xTimerStart(xTimerUpdate, 0) != pdPASS)
    {
	    os_printf("\naqs_timer start failed");
	    return -1;
    }
    return 0;
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

    if (clusterId == AirQuality::Id &&  attributeId == AirQuality::Attributes::AirQuality::Id)
    {
	VerifyOrExit(endpointId == 1, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));
	ChipLogProgress(AppServer, "AirQuality: attribute status value set to %u", airQualityStatus);
    }
    else if (clusterId == Identify::Id)
    {
	VerifyOrExit(endpointId == 1, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));
	OnIdentifyPostAttributeChangeCallback(endpointId, attributeId, value);
    }
    else if (clusterId == TemperatureMeasurement::Id)
    {
        VerifyOrExit(endpointId == 1, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));
        if (attributeId == TemperatureMeasurement::Attributes::MeasuredValue::Id)
        {
            ChipLogProgress(AppServer, "[TemperatureMeasurement]: Measured-Value: %d", *(int16_t*)value);
        }
    }
    else if (clusterId == RelativeHumidityMeasurement::Id)
    {
        VerifyOrExit(endpointId == 1, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));
        if (attributeId == RelativeHumidityMeasurement::Attributes::MeasuredValue::Id)
        {
	    ChipLogProgress(AppServer, "[RelativeHumidityMeasurement]: Measured-Value: %d", *(uint16_t*)value);
        }
    }

    os_printf("\nCluster control command addressed. os_free_heap(): %d", os_avail_heap());
exit:
    return;
}

/** @brief  AirQuality Cluster Init
 *
 * This function is called when a specific cluster is initialized. It gives the
 * application an opportunity to take care of cluster initialization procedures.
 * It is called exactly once for each endpoint where cluster is present.
 *
 * @param endpoint   Ver.: always
 *
 * emberAfAirQualityClusterInitCallback happens before the stack initialize the cluster
 * attributes to the default value.
 * The logic here expects something similar to the emberAfAirQualityClusterInitCallback.
 *
 */
void emberAfAirQualityClusterInitCallback(EndpointId endpoint)
{
    ChipLogError(AppServer, "emberAfAirQualityClusterInitCallback");

    /* Initialize the AirQuality Cluster */
    mAirQualityInstance.Init();
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
    /* Identify Start */
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

    switch (sIdentifyEffect)
    {
    case chip::app::Clusters::Identify::EffectIdentifierEnum::kBlink:
    case chip::app::Clusters::Identify::EffectIdentifierEnum::kOkay:
        identifyTimerCount = 5;
        (void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(5), OnTriggerIdentifyEffectCompleted,
                                                           identify);
        OnIdentifyStart(identify);
        break;
    case chip::app::Clusters::Identify::EffectIdentifierEnum::kBreathe:
    case chip::app::Clusters::Identify::EffectIdentifierEnum::kChannelChange:
        identifyTimerCount = 10;
        (void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(10), OnTriggerIdentifyEffectCompleted,
                                                           identify);
        OnIdentifyStart(identify);
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
        break;
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

    talariautils::ApplicationInitLog("matter air quality sensor app");
    talariautils::EnableSuspend();

    DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);
    led_pin = GPIO_PIN(LED_PIN);
    os_gpio_request(led_pin);
    os_gpio_set_output(led_pin);
    os_gpio_clr_pin(led_pin);
    temp_humidity_control_gpio = GPIO_PIN(TEMP_HUMIDITY_APP_MIN_MAX_CONTROL_GPIO);

    /* Requesting for Gpio pin */
    os_gpio_request(temp_humidity_control_gpio);

    /* Setting Direction as Input */
    os_gpio_set_input(temp_humidity_control_gpio);

    app_test();

    /* There is nothing to do in this task. Putting the task in suspend mode */
    vTaskSuspend(NULL);
    return 0;
}
