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

/* Periodic timeout for Thermostat software timer in ms */
#define THERMOSTAT_PERIODIC_TIME_OUT_MS 30000	//30 sec

#define IDENTIFY_TIMER_DELAY_MS  1000
#define LED_PIN 14

struct thermostat_read_temperature get_temp;
struct thermostat_read_temperature revd_temp;


using namespace chip;
using namespace chip::Platform;
using namespace chip::DeviceLayer;
using namespace chip::Credentials;
using namespace chip::app::Clusters;
using namespace chip::talaria;
using namespace chip::app::Clusters::Identify;
using namespace chip::app::Clusters::Thermostat;
using namespace chip::app::Clusters::Thermostat::Attributes;

DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;
/*-----------------------------------------------------------*/

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

void print_test_results(nlTestSuite * tSuite);
void app_test();
int Read_Update_Thermostat_Temperature(void);

uint32_t led_pin;
static uint32_t identifyTimerCount = 0;

constexpr EndpointId kThermostatEndpoint = 1;

SemaphoreHandle_t ServerInitDone;
SemaphoreHandle_t Getdata;
SemaphoreHandle_t GetTemperatureData;

/* Function Declarations */


static int8_t ConvertToPrintableTemp(int16_t temperature)
{
    /* PlaceHolder: To convert the temparature as per the actual thermostat. */
    constexpr uint8_t kRoundUpValue = 50;

    // Round up the temperature as we won't print decimals on LCD
    // Is it a negative temperature
    if (temperature < 0)
    {
        temperature -= kRoundUpValue;
    }
    else
    {
        temperature += kRoundUpValue;
    }
    return static_cast<int8_t>(temperature / 100);
}

static void OnThermostatPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    switch (attributeId)
    {
    case Thermostat::Attributes::LocalTemperature::Id: {
	int8_t Temp = ConvertToPrintableTemp(*((int16_t *) value));
        ChipLogProgress(AppServer, "[Thermostat] Local Temperature set to: %d°C", Temp);
    }
    break;
    case Thermostat::Attributes::OccupiedCoolingSetpoint::Id: {
        int16_t CoolingSetpoint = *((int16_t *) value);
	int8_t Temp = ConvertToPrintableTemp(CoolingSetpoint);
        ChipLogProgress(AppServer, "[Thermostat] Adjust cooling-setpoint command, value: %d°C", Temp);

	struct thermostat_set_data input_data;
	input_data.mode = 1;
	input_data.CoolingSetpoint = CoolingSetpoint;
	int payload = sizeof(struct thermostat_set_data);

	int ret = matter_notify(THERMOSTAT, THERMOSTAT_SETPOINT_RAISE_LOWER, payload, (struct thermostat_set_data *) &input_data);
	if(ret != 0)
	{
		ChipLogError(AppServer, "[Thermostat] Adjust cooling-setpoint command failed, error: %d", ret);
		return;
	}
	else
	{
		ChipLogProgress(AppServer, "[Thermostat] Successfully adjusted cooling-setpoint to : %d°C", Temp);
	}
    }
    break;
    case Thermostat::Attributes::OccupiedHeatingSetpoint::Id: {
        int16_t HeatingSetpoint  = *((int16_t *) value);
	int8_t Temp = ConvertToPrintableTemp(HeatingSetpoint);
        ChipLogProgress(AppServer, "[Thermostat] Adjust heating-setpoint command, value: %d°C", Temp);

	struct thermostat_set_data input_data;
        input_data.mode = 0;
        input_data.HeatingSetpoint = HeatingSetpoint;
        int payload = sizeof(struct thermostat_set_data);

        int ret = matter_notify(THERMOSTAT, THERMOSTAT_SETPOINT_RAISE_LOWER, payload, (struct thermostat_set_data *) &input_data);
        if(ret != 0)
        {
                ChipLogError(AppServer, "[Thermostat] Adjust heating-setpoint command failed, error: %d", ret);
                return;
        }
        else
        {
                ChipLogProgress(AppServer, "[Thermostat] Successfully adjusted heating-setpoint to : %d°C", Temp);
        }
    }
    break;
    default: {
        return;
    }
    }
}

static void Update_Thermostat_Temperature_attribute_status(intptr_t arg)
{
    app::Clusters::Thermostat::Attributes::LocalTemperature::Set(1, revd_temp.LocalTemperature);
}

static void read_temperature_from_host(void)
{
    struct thermostat_read_temperature *get_temperature;
    GetTemperatureData = xSemaphoreCreateCounting(1, 0);

    while(1)
    {
	    get_temperature = &get_temp;
	    memset(get_temperature, 0, sizeof(struct thermostat_read_temperature));
	    int payload = sizeof(struct thermostat_read_temperature);
	    int ret = matter_notify(THERMOSTAT, THERMOSTAT_READ_TEMPERATURE, payload, (struct thermostat_read_temperature *) get_temperature);
	    if(ret != 0)
		    return;

	    if (xSemaphoreTake(GetTemperatureData, portMAX_DELAY) == pdFAIL)
	    {
		    os_printf("Unable to wait on semaphore...!!\n");
	    }
	    memcpy(get_temperature, &revd_temp, sizeof(struct thermostat_read_temperature));

	    DeviceLayer::PlatformMgr().ScheduleWork(Update_Thermostat_Temperature_attribute_status, reinterpret_cast<intptr_t>(nullptr));
	    vTaskDelay(pdMS_TO_TICKS(THERMOSTAT_PERIODIC_TIME_OUT_MS));
    }
}

int Read_Update_Thermostat_Temperature(void)
{
    static BaseType_t xReturned;
    TaskHandle_t xHandle = NULL;

    if (xReturned != pdPASS)
    {
	    /* Create the task to load the persistent data from hsot */
	    xReturned = xTaskCreate(read_temperature_from_host,   /* Function that implements the task. */
			            "read_temperature_from_host", /* Text name for the task. */
			            configMINIMAL_STACK_SIZE,     /* Stack size in words, not bytes. */
                                    (void *) NULL,                /* Parameter passed into the task. */
                                    tskIDLE_PRIORITY + 2,         /* Priority at which the task is created. */
                                    &xHandle);                    /* Used to pass out the created task's handle. */

	    if (xReturned == pdPASS)
	    {
		    /* The task was created.  Use the task's handle to delete the task. */
		    os_printf("\nLoading Thermostat Temperature from host task created...\n");
		    return 0;
	    }
	    else
	    {
		    os_printf("\nFailed to created Thermostat Temperature host task...\n");
		    return -1;
	    }
    }
    return 0;
}

static void Hanlder(chip::System::Layer * systemLayer, EndpointId endpointId)
{
    if (identifyTimerCount > 0)
    {
        /* Decrement the timer count. */
        identifyTimerCount--;

	/* PlaceHolder: To add vendor-specific implementation for Identify cluster command. */
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
    VerifyOrExit(endpointId == 1, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));

    switch (clusterId)
    {
    case Identify::Id:
	OnIdentifyPostAttributeChangeCallback(endpointId, attributeId, value);
	break;
    case Thermostat::Id:
	OnThermostatPostAttributeChangeCallback(endpointId, attributeId, value);
	break;
    default:
	break;
    }

    os_printf("\nCluster control command addressed. os_free_heap(): %d", os_avail_heap());
exit:
    return;
}

void emberAfThermostatClusterInitCallback(chip::EndpointId endpoint)
{
    ChipLogDetail(AppServer, "emberAfThermostatClusterInitCallback");
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


BOOTARG_INT("disp_pkt_info", "display packet info", "1 to enable packet info print. 0(default) to disable");


int main(void)
{
#ifdef UNIT_TEST
    run_unit_test();
#endif

    talariautils::ApplicationInitLog("matter thermostat app");

    matter_hio_init();
    /* Delay is required before start doing the communication over hio,
       otherwise don't see any response*/
    vTaskDelay(pdMS_TO_TICKS(2000));

    int FactoryReset = os_get_boot_arg_int("matter.factory_reset", 0);
    if (FactoryReset == 1 || FactoryReset == 2)
    {
        talariautils::FactoryReset(FactoryReset);
	vTaskSuspend(NULL);
    }

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
