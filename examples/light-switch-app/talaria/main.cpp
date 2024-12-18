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

#include "BindingHandler.h"
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
#include <app/clusters/switch-server/switch-server.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include "LightSwitch_ProjecConfig.h"
#include <app/clusters/identify-server/identify-server.h>

using namespace chip;
using namespace chip::Platform;
using namespace chip::DeviceLayer;
using namespace chip::Credentials;
using namespace chip::app::Clusters;
using namespace chip::talaria;
using namespace chip::app::Clusters::Switch;

DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;
/*-----------------------------------------------------------*/
void print_test_results(nlTestSuite * tSuite);
void app_test();
void startCommandLineInterface();
int SoftwareTimer_Init(void);

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

static enum Switch_Position Current_Switch_Pos = SWITCH_POS_0;
static SwitchServer mSwitchServer = SwitchServer::Instance();

static int GpioValueAfterInterupt;
TimerHandle_t xTimer;
uint32_t switch_pin;
uint32_t led_pin;
static uint32_t identifyTimerCount = 0;


/* 
 * The following ISR logic is mimicing the edge trigger for the switch gpio.
 * Known issue: With suspend enabled the edge interrupt isn't working,
 *              hence we are using level trigger interrupt.
 * Logic:
 * 	  On level low interrupt  -> Disable interrupt
 * 	  			  -> Configure the interrupt to level high
 * 	  			  -> Trigger Binding command to Off
 * 	  On level high interrupt -> Disable interrupt
 * 	  			  -> Configure the interrupt to level low
 * 	  			  -> Trigger Binding command to On
 */
static void __irq IRQHandler(void * arg)
{
    BaseType_t xHighterPriority = pdFALSE;

    /* Disabling the Interupt so that we dont get ISR for same level again */
    os_gpio_disable_irq(switch_pin);
    os_clear_event(EVENT_GPIO_3);
    GpioValueAfterInterupt = (os_gpio_get_value(switch_pin) > 0) ?  1 : 0;

    /* Starting Debounce Software Timer */
    if(xTimerStartFromISR(xTimer, &xHighterPriority) != pdPASS)
    {
        os_printf("\nUnable to Start Timer From ISR.");
        os_gpio_enable_irq(switch_pin, gpio_event_3);
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

    switch (clusterId)
    {
    case Switch::Id:
       VerifyOrExit(endpointId == 2, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));
       if (attributeId  == Switch::Attributes::CurrentPosition::Id)
       {
	  ChipLogProgress(AppServer, "Current switch position set to: %d", *value);
       }
       else if (attributeId  == Switch::Attributes::NumberOfPositions::Id)
       {
	  ChipLogProgress(AppServer, "Number of switch positions set to: %d", *value);
       }
       break;
    case app::Clusters::Identify::Id:
       VerifyOrExit(endpointId == 1, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));
       OnIdentifyPostAttributeChangeCallback(endpointId, attributeId, value);
       break;
    case OnOffSwitchConfiguration::Id:
       ChipLogProgress(AppServer, "OnOff Switch Configuration attribute ID: '0x%x', value: %d", attributeId, *value);
       break;
    default:
        break;
    }

    os_printf("\nCluster control command addressed. os_free_heap(): %d", os_avail_heap());
exit:
    return;
}

/** @brief Switch Cluster Init
 *
 * This function is called when a specific cluster is initialized. It gives the
 * application an opportunity to take care of cluster initialization procedures.
 * It is called exactly once for each endpoint where cluster is present.
 *
 * @param endpoint   Ver.: always
 *
 * emberAfSwitchClusterInitCallback happens before the stack initialize the cluster
 * attributes to the default value.
 *
 */
void emberAfSwitchClusterInitCallback(EndpointId endpoint)
{
    ChipLogDetail(AppServer, "emberAfSwitchClusterInitCallback");

    /* Initialize Type and Number of Positions attributes of Switch */
    Switch::Attributes::NumberOfPositions::Set(SWITCH_ENDPOINT_ID, SWITCH_MAX_POSITIONS);
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

void PrepareBindingCommand(uint8_t state)
{
    BindingCommandData * data = Platform::New<BindingCommandData>();
    data->clusterId = chip::app::Clusters::OnOff::Id;

    if (state == 0)
	    data->commandId = chip::app::Clusters::OnOff::Commands::Off::Id;
    else
	    data->commandId = chip::app::Clusters::OnOff::Commands::On::Id;

    SwitchWorkerFunction(data);
}

void Retry_PrepareBindingCommand(void)
{
    uint8_t state;

    /* Reading the state of GPIO */
    state = (os_gpio_get_value(switch_pin) > 0) ?  1 : 0;
    PrepareBindingCommand(state);
}

static void DebounceTimer(TimerHandle_t xTimer)
{
    /* Configuration to Send function pointer to xQueueSend */
    uint8_t state;

    /* Reading the state of GPIO */
    state = (os_gpio_get_value(switch_pin) > 0) ?  1 : 0;

    if (state != GpioValueAfterInterupt)
    {
        os_gpio_enable_irq(switch_pin, gpio_event_3);
        return;
    }
    if (state)
    {
        os_gpio_set_irq_level_low(switch_pin);
    }
    else
    {
        os_gpio_set_irq_level_high(switch_pin);
    }
    chip::DeviceLayer::PlatformMgr().ScheduleWork(PrepareBindingCommand, state);

    /* Enabling Interupt Again after ISR was served */
    os_gpio_enable_irq(switch_pin, gpio_event_3);
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

void print_test_results(nlTestSuite *tSuite)
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

static void configureSwitchGPIO()
{
    switch_pin = GPIO_PIN(SWITCH_PIN);

    /* Requesting for Gpio pin */
    os_gpio_request(switch_pin);

    /* Setting Direction as Input */
    os_gpio_set_input(switch_pin);

    /* Setting IRQ level from low to high */
    /* Here os_gpio_set_edge_both() is not used since in suspend mode irq attched event is not triggered */
    os_gpio_set_irq_level_low(switch_pin);

    os_gpio_set_pull(switch_pin);

    /* Attaching Event handler to IRQ */
    os_gpio_attach_event(gpio_event_3, IRQHandler, NULL);

    /* Create Debounce Software Timer */
    xTimer = xTimerCreate( "DebounceTimer", pdMS_TO_TICKS(40), pdFALSE, (void *)0, DebounceTimer);

    /* Enabling IRQ */
    os_gpio_enable_irq(switch_pin, gpio_event_3);
}

#if ENABLE_LEVEL_CONTROL
static void PrepareBindingCommand_Level_Control(uint8_t current_pos)
{
    /* PlaceHolder: Map each switch position to the corresponding level control cluster command
     * to be sent to the other device to perform the operation.
     * Prepare and send Binding command accordingly. */

    BindingCommandData * data = Platform::New<BindingCommandData>();

    /* Command arguments for MoveToLevelWithOnOff:
     * data->args[0] - Current Level, data->args[1] - Transition Time
     * data->args[2] - Options Mask, data->args[3] - Options Override
     * Update the data commandId and arguments as per requirement. */

    data->clusterId = app::Clusters::LevelControl::Id;
    data->commandId = app::Clusters::LevelControl::Commands::MoveToLevelWithOnOff::Id;
    data->attributeId = app::Clusters::LevelControl::Attributes::CurrentLevel::Id;
    data->args[1] = 0;
    data->args[2] = 1;
    data->args[3] = 1;

    switch (current_pos)
    {
    case SWITCH_POS_0:
       data->args[0] = 0;
       break;
    case SWITCH_POS_1:
       data->args[0] = 64;
       break;
    case SWITCH_POS_2:
       data->args[0] = 128;
       break;
    case SWITCH_POS_3:
       data->args[0] = 192;
       break;
    case SWITCH_POS_4:
       data->args[0] = 254;
       break;
    default:
       ChipLogError(AppServer, "Invalid Switch Position: %d", current_pos);
       return;
    }

    SwitchWorkerFunction(data);
}

void Retry_PrepareBindingCommand_Level_Control(void)
{
    uint8_t switch_pos;
    Switch::Attributes::CurrentPosition::Get(SWITCH_ENDPOINT_ID, &switch_pos);
    PrepareBindingCommand_Level_Control(switch_pos);
}
#endif /* ENABLE_LEVEL_CONTROL */

#if ENABLE_COLOUR_CONTROL
static void PrepareBindingCommand_Colour_Control(uint8_t current_pos)
{
    /* PlaceHolder: Map each switch position to the corresponding colour control cluster command
     * to be sent to the other device to perform the operation.
     * Prepare and send Binding command accordingly. */

    BindingCommandData * data = Platform::New<BindingCommandData>();

    /* Command arguments for MoveToHue:
     * data->args[0] - Hue, data->args[1] - direction
     * data->args[2] - Transition Time, data->args[3] - Options Mask
     * data->args[4] - Options Override
     *
     * Command arguments for MoveToSaturation:
     * data->args[0] - Saturation, data->args[1] - Transition Time
     * data->args[2] - Options Mask, data->args[3] - Options Override
     * Update the data commandID and arguments as per requirement. */

    data->clusterId = app::Clusters::ColorControl::Id;
    data->commandId = app::Clusters::ColorControl::Commands::MoveToHue::Id;
    data->args[1] = 1;
    data->args[2] = 1;
    data->args[3] = 1;
    data->args[4] = 1;

    switch (current_pos)
    {
    case SWITCH_POS_0:
       data->args[0] = 0;
       break;
    case SWITCH_POS_1:
       data->args[0] = 64;
       break;
    case SWITCH_POS_2:
       data->args[0] = 128;
       break;
    case SWITCH_POS_3:
       data->args[0] = 192;
       break;
    case SWITCH_POS_4:
       data->args[0] = 254;
       break;
    default:
       ChipLogError(AppServer, "Invalid Switch Position: %d", current_pos);
       return;
    }

    SwitchWorkerFunction(data);
}

void Retry_PrepareBindingCommand_Colour_Control(void)
{
    uint8_t switch_pos;
    Switch::Attributes::CurrentPosition::Get(SWITCH_ENDPOINT_ID, &switch_pos);
    PrepareBindingCommand_Colour_Control(switch_pos);
}
#endif /* ENABLE_COLOUR_CONTROL */

static void Update_SwitchPosition_Attribute_Status(uint8_t current_pos)
{
    /* Update Current Switch Position attribute value */
    Switch::Attributes::CurrentPosition::Set(SWITCH_ENDPOINT_ID, current_pos);

    /* Update Current Switch Position event */
    mSwitchServer.OnSwitchLatch(SWITCH_ENDPOINT_ID, current_pos);
}

static void OnSwitchPositionChanged(uint8_t Switch_Pos)
{
    /* PlaceHolder: To call vendor-specific API to handle the Switch command,
     * OR send a binding command to another Matter device.
     * Update_SwitchPosition_Attribute_Status API will be called post handling
     * of Switch command to update the switch position attribute. */
    static uint8_t previous_pos;

    Switch::Attributes::CurrentPosition::Get(SWITCH_ENDPOINT_ID, &previous_pos);
    if (previous_pos == Switch_Pos)
	    return;

#if ENABLE_LEVEL_CONTROL
    PrepareBindingCommand_Level_Control(Switch_Pos);
#endif /* ENABLE_LEVEL_CONTROL */

#if ENABLE_COLOUR_CONTROL
    PrepareBindingCommand_Colour_Control(Switch_Pos);
#endif /* ENABLE_COLOUR_CONTROL */

    Update_SwitchPosition_Attribute_Status(Switch_Pos);
}

#if SWITCH_USING_ADC
static void OnSwitchPositionChangedHandler_Adc(intptr_t arg)
{
    uint32_t adc_val = os_adc();

    if (adc_val > ADC_VALUE_0_25_V_MIN && adc_val <= ADC_VALUE_0_25_V_MAX)
	    Current_Switch_Pos = SWITCH_POS_1;
    else if (adc_val > ADC_VALUE_0_25_V_MAX && adc_val <= ADC_VALUE_0_5_V_MAX)
	    Current_Switch_Pos = SWITCH_POS_2;
    else if (adc_val > ADC_VALUE_0_5_V_MAX && adc_val <= ADC_VALUE_0_75_V_MAX)
	    Current_Switch_Pos = SWITCH_POS_3;
    else if (adc_val > ADC_VALUE_0_75_V_MAX && adc_val <= ADC_VALUE_1_V_MAX)
	    Current_Switch_Pos = SWITCH_POS_4;
    else
	    Current_Switch_Pos = SWITCH_POS_0;

    OnSwitchPositionChanged(Current_Switch_Pos);
}
#else
static void OnSwitchPositionChangedHandler_RandomFunction(intptr_t arg)
{
    Current_Switch_Pos = rand() % (SWITCH_MAX_POSITIONS + 1);

    OnSwitchPositionChanged(Current_Switch_Pos);
}
#endif  /* SWITCH_USING_ADC */

#if ENABLE_DIMMER_SWITCH
static void vTimerCallback_switch_position_change(TimerHandle_t xTimer)
{
#if SWITCH_USING_ADC
    DeviceLayer::PlatformMgr().ScheduleWork(OnSwitchPositionChangedHandler_Adc, reinterpret_cast<intptr_t>(nullptr));
#else
    int enable_cli = os_get_boot_arg_int("matter.enable_cli", 0);
    if (enable_cli != 1)
    {
        DeviceLayer::PlatformMgr().ScheduleWork(OnSwitchPositionChangedHandler_RandomFunction, reinterpret_cast<intptr_t>(nullptr));
    }
#endif /* SWITCH_USING_ADC */
}
#endif /* ENABLE_DIMMER_SWITCH */

#if ENABLE_DIMMER_SWITCH
int SoftwareTimer_Init()
{
    static TimerHandle_t xTimer = NULL;

    if (xTimer != NULL)
	    return 0;

    int switch_pos_periodic_timeout_ms = os_get_boot_arg_int("matter.config_app_timer_interval_ms", SWITCH_POS_PERIODIC_TIME_OUT_MS);
    os_printf("\nswitch_pos_periodic_timeout set to :%d ms", switch_pos_periodic_timeout_ms);
    xTimer = xTimerCreate("switch_position", pdMS_TO_TICKS(switch_pos_periodic_timeout_ms),
		          pdTRUE, (void *) 0, vTimerCallback_switch_position_change);
    if (xTimer == NULL)
    {
	    ChipLogError(AppServer, "switch_position creation failed");
	    return -1;
    }
    if (xTimerStart(xTimer, 0) != pdPASS)
    {
	    ChipLogError(AppServer, "switch_position start failed");
	    return -1;
    }
    return 0;
}
#endif /* ENABLE_DIMMER_SWITCH */

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

    talariautils::ApplicationInitLog("matter light-switch app");
    talariautils::EnableSuspend();
    DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);
    led_pin = GPIO_PIN(LED_PIN);
    os_gpio_request(led_pin);
    os_gpio_set_output(led_pin);
    os_gpio_clr_pin(led_pin);
    configureSwitchGPIO();

    app_test();
    int enable_cli = os_get_boot_arg_int("matter.enable_cli", 0);
    if (enable_cli == 1)
    {
        startCommandLineInterface();
    }

    vTaskSuspend(NULL);

    return 0;
}
