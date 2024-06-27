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
#include <kernel/pwm.h>

#if CONFIG_ENABLE_AWS_IOT_APP
#include "talaria_aws_iot/aws_iot.h"
#endif /* CONFIG_ENABLE_AWS_IOT_APP */

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
#include <app-common/zap-generated/attributes/Accessors.h>

#include <common/Utils.h>
#include <platform/ConnectivityManager.h>
#include <platform/talaria/TalariaUtils.h>
#include <math.h>
#include <app/clusters/identify-server/identify-server.h>

using namespace chip;
using namespace chip::Platform;
using namespace chip::DeviceLayer;
using namespace chip::Credentials;
using namespace chip::app::Clusters;
using namespace chip::talaria;
using namespace chip::app::Clusters::ColorControl;
using namespace chip::app::Clusters::LevelControl;
using namespace chip::app::Clusters::OccupancySensing;

DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

#define PWM_PIN 14
#define PWM_PERIOD 1000
#define OCCUPANCY_SENSOR_PERIODIC_TIME_OUT_MS 20000
#define IDENTIFY_TIMER_DELAY_MS  1000

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
#if CONFIG_ENABLE_AWS_IOT_APP
void execute_lighting_cmd(bool value);
#endif /* CONFIG_ENABLE_AWS_IOT_APP */

int main_TestInetLayer(int argc, char * argv[]);
static uint32_t identifyTimerCount = 0;
static struct pwm_output_cfg cfg;
static uint8_t flag_suspend_enable = 0;

/* Function Declarations */
static void CommonDeviceEventHandler(const chip::DeviceLayer::ChipDeviceEvent * event, intptr_t arg);

#ifdef __cplusplus
extern "C" {
#endif
struct wcm_handle * get_wcm_handle_for_additional_app(void)
{
    return chip::DeviceLayer::Internal::TalariaUtils::Get_wcm_handle();
}

#if CONFIG_ENABLE_AWS_IOT_APP
void execute_lighting_cmd_from_aws_server(bool value)
{
    DeviceLayer::PlatformMgr().ScheduleWork(execute_lighting_cmd, value);
}
#endif /* CONFIG_ENABLE_AWS_IOT_APP */

#ifdef __cplusplus
}
#endif

#if CONFIG_ENABLE_AWS_IOT_APP
static void post_lighting_status_to_aws_server(uint8_t value)
{
    DeviceLayer::PlatformMgr().ScheduleWork(post_lighting_status, value);
}

void execute_lighting_cmd(bool value)
{
    chip::app::Clusters::OnOff::Attributes::OnOff::Set(1, value);
}
#endif /* CONFIG_ENABLE_AWS_IOT_APP */

static void SetPWMdutyCycle(uint8_t dutycycle)
{
    cfg.duty_cycle = dutycycle;
    pwm_output_cfg_set(&cfg);
}

/* The Toggle API is used to control LED for Identify Cluster Commands. */
static void Toggle(EndpointId endpointId )
{
    bool value;
    chip::app::Clusters::OnOff::Attributes::OnOff::Get(endpointId , &value);
    if(value)
    {
        if (flag_suspend_enable)
            os_gpio_clr_pin(1 << PWM_PIN);
        else
            SetPWMdutyCycle(0);

        chip::app::Clusters::OnOff::Attributes::OnOff::Set(endpointId, false);
    }
    else
    {
        if (flag_suspend_enable)
            os_gpio_set_pin(1 << PWM_PIN);
        else
            SetPWMdutyCycle(100);

        chip::app::Clusters::OnOff::Attributes::OnOff::Set(endpointId, true);
    }
}

static void Hanlder(chip::System::Layer * systemLayer, EndpointId endpointId)
{
    if (identifyTimerCount > 0)
    {
        /* Decrement the timer count. */
        identifyTimerCount--;
        Toggle( endpointId );
        chip::app::Clusters::Identify::Attributes::IdentifyTime::Set(endpointId, identifyTimerCount );
    }
}

static void OnIdentifyPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    identifyTimerCount = (*value);
    DeviceLayer::SystemLayer().CancelTimer(Hanlder, endpointId);
    DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Milliseconds64(IDENTIFY_TIMER_DELAY_MS), Hanlder, endpointId);

exit:
    return;
}

static void ConfigureLED(void)
{
    int led_pin = 1 << PWM_PIN;
    os_gpio_request(led_pin);
    os_gpio_set_output(led_pin);
    os_gpio_clr_pin(led_pin);
}

static void ConfigurePWM(void)
{
    /* Set the default port to operate with 0% duty cycle. */
    cfg = { .port = 0, .duty_cycle = 0 };

    /* Set PWM_PIN as pwm */
    os_gpio_request(PWM_PIN);

    /* Set the operational mode of the pins to FUNCTION, as PWM will
     * operate them instead of the default GPIO block. */
    os_gpio_set_mode(PWM_PIN, GPIO_FUNCTION_MODE);

    /* Re-route the default pin to the selected pin. */
    os_gpio_mux_sel(GPIO_MUX_SEL_PWM_0, PWM_PIN);

    /* Create a 1000ns (1Mhz) long PWM signal */
    pwm_enable(PWM_PERIOD);

    /* Configure the channel to be enabled */
    if (pwm_channel_cfg_set(0, PWM_CTRL_ENABLE)) {
        ChipLogError(AppServer, "Failed to enable PWM channel 0!\n");
    }
}

static void SetBrightnessLevel(uint8_t value)
{
    /* Brightness level value received from user will be in rage of [0, 254] as per matter spec.
     * Converting [0, 254] range to [0, 100] as PWM duty cycle accepts between 0 to 100 percent. */

    float actual_brightness = value;
    uint8_t brightness = ceil(actual_brightness * 100 / 254);

    /* Setting Brightness Level of light using PWM dutycycle. */
    if (brightness >= 0 && brightness <= 100)
    {
	    ChipLogProgress(AppServer, "Setting Brightness Level to :%d", value);
	    SetPWMdutyCycle(brightness);
    }
}

static void SetColor(uint8_t hue, uint8_t saturation)
{
    ChipLogProgress(AppServer, "Color Control command received, hue: %d, saturation:%d", hue, saturation);

    /* PlaceHolder: To call vendor-specific APIs to handle color control commands. */

}

static void UpdateCurrentOccupancyStatus(intptr_t arg)
{
    bool OccupancyStatus = false;

    /* PlaceHolder: To call vendor-specific API to get the current occupency status
     * from occupancy sensor inplace of Random function.
     * Return status as true if occupied or false if not occupied. */

    OccupancyStatus = rand() % 2;
    if (OccupancyStatus)
	    OccupancySensing::Attributes::Occupancy::Set(1, OccupancyBitmap::kOccupied);
    else
	    OccupancySensing::Attributes::Occupancy::Set(1, 0);
}

static void OnLevelControlAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    bool onoff_value;
    chip::app::Clusters::OnOff::Attributes::OnOff::Get(endpointId , &onoff_value);
    if (attributeId == LevelControl::Attributes::CurrentLevel::Id && onoff_value == true)
    {
        if (flag_suspend_enable)
            os_gpio_set_pin(1 << PWM_PIN);
        else
            SetBrightnessLevel(*value);
    }
}

static void OnColorControlAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    uint8_t hue, saturation;

    if (attributeId == ColorControl::Attributes::CurrentHue::Id)
    {
	    hue = *value;
            ColorControl::Attributes::CurrentSaturation::Get(endpointId, &saturation);
            SetColor(hue, saturation);
    }
    if (attributeId == ColorControl::Attributes::CurrentSaturation::Id)
    {
	    saturation = *value;
            ColorControl::Attributes::CurrentHue::Get(endpointId, &hue);
            SetColor(hue, saturation);
    }
}

static void OnOccupancySensingAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    if (attributeId == OccupancySensing::Attributes::Occupancy::Id)
    {
	    if (*value)
	    {
		    ChipLogProgress(AppServer, "OccupancySensing status: Occupancy detected");
	    }
	    else
	    {
		    ChipLogProgress(AppServer, "OccupancySensing status: Occupancy not detected");
	    }
    }
}

/* occupancy sensing values are polling-based. Depending on the sensor type,
 * implementation needs to be changed to interrupt-based sensing.*/
static void vTimerCallback_Occupancy_Status(TimerHandle_t xTimer)
{
    DeviceLayer::PlatformMgr().ScheduleWork(UpdateCurrentOccupancyStatus, reinterpret_cast<intptr_t>(nullptr));
}

int SoftwareTimer_Init()
{
    static TimerHandle_t xTimer = NULL;

    if (xTimer != NULL)
            return 0;

    xTimer = xTimerCreate("Occupancy_sensor_timer", pdMS_TO_TICKS(OCCUPANCY_SENSOR_PERIODIC_TIME_OUT_MS),
                          pdTRUE, (void *) 0, vTimerCallback_Occupancy_Status);
    if (xTimer == NULL)
    {
            ChipLogError(AppServer, "Occupancy_sensor_timer creation failed");
            return -1;
    }
    if (xTimerStart(xTimer, 0) != pdPASS)
    {
            ChipLogError(AppServer, "Occupancy_sensor_timer start failed");
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

    switch (clusterId)
    {
    case OnOff::Id:
        /* Return if the attribute ID not matches */
        if (attributeId != OnOff::Attributes::OnOff::Id)
        {
            return;
        }
        VerifyOrExit(attributeId == OnOff::Attributes::OnOff::Id,
                     ChipLogError(AppServer, "Unhandled Attribute ID: '0x%" PRIx32 "'", attributeId));
        VerifyOrExit(endpointId == 1, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));

        if (*value == 1)
        {
            if (flag_suspend_enable)
            {
                os_gpio_set_pin(1 << PWM_PIN);
            }
            else
            {
                /* Turn on the LED with the current brightness level */
                app::DataModel::Nullable<uint8_t> currentLevel;
                chip::app::Clusters::LevelControl::Attributes::CurrentLevel::Get(endpointId, currentLevel);
                SetBrightnessLevel(currentLevel.Value());
            }
	}
        else
        {
            if (flag_suspend_enable)
                os_gpio_clr_pin(1 << PWM_PIN);
            else
                SetPWMdutyCycle(0);
	}

#if CONFIG_ENABLE_AWS_IOT_APP
	post_lighting_status_to_aws_server(*value);
#endif /* CONFIG_ENABLE_AWS_IOT_APP */
	break;
    case app::Clusters::Identify::Id:
        OnIdentifyPostAttributeChangeCallback(endpointId, attributeId, value);
        break;
    case app::Clusters::LevelControl::Id:
        VerifyOrExit(endpointId == 1, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));
        OnLevelControlAttributeChangeCallback(endpointId, attributeId, value);
        break;
    case app::Clusters::ColorControl::Id:
        VerifyOrExit(endpointId == 1, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));
        OnColorControlAttributeChangeCallback(endpointId, attributeId, value);
        break;
    case app::Clusters::OccupancySensing::Id:
        VerifyOrExit(endpointId == 1, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));
        OnOccupancySensingAttributeChangeCallback(endpointId, attributeId, value);
        break;
    default:
        break;
    }

    os_printf("\nCluster control command addressed. os_free_heap(): %d", os_avail_heap());
exit:
    return;
}

/** @brief OnOff Cluster Init
 *
 * This function is called when a specific cluster is initialized. It gives the
 * application an opportunity to take care of cluster initialization procedures.
 * It is called exactly once for each endpoint where cluster is present.
 *
 * @param endpoint   Ver.: always
 *
 * emberAfOnOffClusterInitCallback happens before the stack initialize the cluster
 * attributes to the default value.
 * The logic here expects something similar to the deprecated Plugins callback
 * emberAfPluginOnOffClusterServerPostInitCallback.
 *
 */
void emberAfOnOffClusterInitCallback(EndpointId endpoint)
{
    ChipLogDetail(AppServer, "emberAfOnOffClusterInitCallback");
    /* OnOff value is stored in persistant storage hence we retrive,
       from it and setting the value of the LED in T2 Device.
    */
    bool value;
    chip::app::Clusters::OnOff::Attributes::OnOff::Get(endpoint, &value);
    if( value )
    {
        chip::app::Clusters::OnOff::Attributes::OnOff::Set(endpoint, true);
    }
    else
    {
        chip::app::Clusters::OnOff::Attributes::OnOff::Set(endpoint, false);
    }
}

void emberAfLevelControlClusterInitCallback(EndpointId endpoint)
{
    ChipLogDetail(AppServer, "emberAfLevelControlClusterInitCallback");
    /* Current-Level value is stored in persistant storage hence we retrive
       from it and setting the value of the current level in T2 Device.
    */
    app::DataModel::Nullable<uint8_t> currentLevel;
    chip::app::Clusters::LevelControl::Attributes::CurrentLevel::Get(endpoint, currentLevel);
    chip::app::Clusters::LevelControl::Attributes::CurrentLevel::Set(endpoint, currentLevel);
}

void emberAfIdentifyClusterInitCallback(chip::EndpointId endpoint)
{
    chip::app::Clusters::Identify::Attributes::IdentifyType::Set(endpoint, chip::app::Clusters::Identify::IdentifyTypeEnum::kLightOutput);
    chip::app::Clusters::Identify::Attributes::IdentifyTime::Set(endpoint, 0);
    ChipLogDetail(AppServer, "emberAfIdentifyClusterInitCallback");
}

void emberAfOccupancySensingClusterInitCallback(EndpointId endpoint)
{
    /* Initialize Occupancy Sensor type as PIR sensor. */
    OccupancySensing::Attributes::OccupancySensorTypeBitmap::Set(endpoint, OccupancySensorTypeBitmap::kPir);
    ChipLogDetail(AppServer, "emberAfOccupancySensingClusterInitCallback");
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
    flag_suspend_enable = os_get_boot_arg_int("suspend", 0);
#ifdef UNIT_TEST
    run_unit_test();
#endif

    talariautils::ApplicationInitLog("matter lighting app");
    talariautils::EnableSuspend();
    /* PWM is disabled in Suspend mode, so the brightness of the LED will not be changed by level control commands.
     * The LED will turn off when the brightness level is set to min-level.
     * The LED will turn on when the brightness level is set up to max-level. */
    if (flag_suspend_enable)
    {
        ChipLogProgress(AppServer, "PWM is disabled in Suspend mode, "
                                   "brightness of LED will not be changed by level control commands");
        ConfigureLED();
    }
    else
    {
        ConfigurePWM();
    }

    DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

    app_test();

    /* There is nothing to do in this task. Putting the task in suspend mode */
    vTaskSuspend(NULL);
    return 0;
}
