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

void print_faults();
int filesystem_util_mount_data_if(const char * path);
void print_ver(char *banner, int print_sdk_name, int print_emb_app_ver);

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
// #include <lib/support/CodeUtils.h>
#include <lib/support/UnitTestRegistration.h>
#include <platform/talaria/DeviceInfoProviderImpl.h>
// #include <platform/Talaria/TalariaDeviceInfoProvider.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/server/Server.h>
#include <common/Utils.h>

using namespace chip;
using namespace chip::Platform;
using namespace chip::DeviceLayer;
using namespace chip::Credentials;
using namespace chip::app::Clusters;
using namespace chip::talaria;

DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

#define LED_PIN 14
#define SWITCH_PIN 18

/*-----------------------------------------------------------*/
void test_fn_1();
int TestBitMask();
int chip::RunRegisteredUnitTests();
void print_test_results(nlTestSuite * tSuite);
void test_suit_proc();
void app_test();

/*Semaphour for switch functionality  */
int switch_interrupt;
uint32_t state;
uint32_t switch_pin;
static bool SwitchGpioInteruptEnable = false;

void configureSwitchGpioIsrLevel(int level)
{
    if(level == 0)
    {
        os_gpio_set_irq_level_low(switch_pin);
    }
    else {
        os_gpio_set_irq_level_high(switch_pin);
    }
    os_gpio_enable_irq(switch_pin, 3);
}

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
int __irq gpio_changed(uint32_t irqno, void * arg)
{
    os_gpio_disable_irq(switch_pin);
    os_clear_event(EVENT_GPIO_3);
    /* Reading the switch_pin GPIO */
    state = (os_gpio_get_value(switch_pin) > 0 ) ? 1 : 0;
    switch_interrupt++;

    if(state)
    {
        /* This will change the irq level from high to low */
        configureSwitchGpioIsrLevel(0);
    }
    else {
        /* This will change the irq level from low to high */
        configureSwitchGpioIsrLevel(1);
    }

    return IRQ_HANDLED;
}


int main_TestInetLayer(int argc, char * argv[]);

/* Function Declarations */
static void CommonDeviceEventHandler(const chip::DeviceLayer::ChipDeviceEvent * event, intptr_t arg);
CHIP_ERROR InitWifiStack();

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
	if (attributeId != OnOff::Attributes::OnOff::Id) {
		return;
	}
        VerifyOrExit(attributeId == OnOff::Attributes::OnOff::Id,
                     ChipLogError(AppServer, "Unhandled Attribute ID: '0x%" PRIx32 "'", attributeId));
        VerifyOrExit(endpointId == 1, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));

        /* TODO: Use LED control api call to T2
        AppLED.Set(*value);
        */
        if (*value == 1)
        {
            os_gpio_set_pin(1 << LED_PIN);
        }
        else
        {
            os_gpio_clr_pin(1 << LED_PIN);
        }
        break;

    default:
        // ChipLogDetail(AppServer, "Unhandled cluster ID: %" PRIu32, clusterId);
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

void ConfigurationAfterCommissioning()
{
    state = 0;
    /* Setting Gpio pin SWITCH_PIN*/
    switch_pin = 1 << SWITCH_PIN;
    /*Requesting for Gpio pin*/
    os_gpio_request(switch_pin);
    /*Setting Direction as Input */
    os_gpio_set_input(switch_pin);
    /*Setting IRQ level from low to high*/
    /*Here os_gpio_set_edge_both() is not used since in suspend mode irq attched event is not triggered */
    os_gpio_set_irq_level_low(switch_pin);
    /*Enabling IRQ*/
    os_gpio_enable_irq(switch_pin, 3);
    /*Attaching Event handler to IRQ*/
    os_attach_event(EVENT_GPIO_3, gpio_changed, NULL);
    os_gpio_set_pull(GPIO_PIN(switch_pin));
    SwitchGpioInteruptEnable = true;
}

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

    int led_pin = 1 << LED_PIN;
    os_gpio_request(led_pin);
    os_gpio_set_output(led_pin);
    os_gpio_clr_pin(led_pin);

    DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);
    app_test();
    while (1)
    {
        if( matterutils::IsNodeCommissioned() == true && SwitchGpioInteruptEnable == false)
        {
            ConfigurationAfterCommissioning();
        }
        vTaskDelay(1000);
        os_printf(".");

        if (switch_interrupt)
        {
            switch_interrupt = 0;
        }
        else
        {
            continue;
        }
        if (state == 0)
        {
            BindingCommandData * data = Platform::New<BindingCommandData>();
            data->commandId           = chip::app::Clusters::OnOff::Commands::Off::Id;
            data->clusterId           = chip::app::Clusters::OnOff::Id;
            DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, reinterpret_cast<intptr_t>(data));
        }
        else if (state == 1)
        {

            BindingCommandData * data = Platform::New<BindingCommandData>();
            data->commandId           = chip::app::Clusters::OnOff::Commands::On::Id;
            data->clusterId           = chip::app::Clusters::OnOff::Id;
            DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, reinterpret_cast<intptr_t>(data));
        }
    }

    return 0;
}
