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
void print_ver(char * banner, int print_sdk_name, int print_emb_app_ver);

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


using namespace chip;
using namespace chip::Platform;
using namespace chip::DeviceLayer;
using namespace chip::Credentials;
using namespace chip::app::Clusters;
using namespace chip::talaria;

DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

#define CONTACT_PIN GPIO_PIN(18)

/*-----------------------------------------------------------*/
#ifdef UNIT_TEST
void print_test_results(nlTestSuite * tSuite);
void test_suit_proc();
#endif

void app_test();
static void SetBooleanState(bool value);
static int GpioValueAfterInterupt;
TimerHandle_t xTimer;

void emberAfBooleanStateClusterInitCallback(EndpointId endpoint)
{
    ChipLogProgress(Zcl, "emberAfBooleanStateClusterInitCallback");
    int state = (os_gpio_get_value(CONTACT_PIN)>0) ?  1 : 0;
    chip::DeviceLayer::PlatformMgr().ScheduleWork( SetBooleanState, state);

}

static void PostAttributeChangeCallback(EndpointId endpointId, ClusterId clusterId, AttributeId attributeId, uint8_t type,
                                        uint16_t size, uint8_t * value)
{
    ChipLogDetail(AppServer,
                  "PostAttributeChangeCallback - Cluster ID: '0x%" PRIx32 "', EndPoint ID: '0x%x', Attribute ID: '0x%" PRIx32 "'",
                  clusterId, endpointId, attributeId);

    if (clusterId == BooleanState::Id && attributeId == BooleanState::Attributes::StateValue::Id)
    {
        ChipLogProgress(Zcl, "Cluster BooleanState: attribute StateValue set to %u", *value);
    }

    os_printf("\nCluster control command addressed. os_free_heap(): %d", os_avail_heap());
exit:
    return;
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

 static void DebounceTimer(TimerHandle_t xTimer)
{
    /* Configuration to Send function pointer to xQueueSend*/
    int state;

    /*Reading the state of GPIO*/
    state = (os_gpio_get_value(CONTACT_PIN)>0) ?  1 : 0;

    if (state != GpioValueAfterInterupt)
    {
        os_gpio_enable_irq( CONTACT_PIN, gpio_event_1);
        return;
    }
    if (state)
    {
        os_gpio_set_irq_level_low(CONTACT_PIN);
    }
    else
    {
        os_gpio_set_irq_level_high(CONTACT_PIN);
    }
    chip::DeviceLayer::PlatformMgr().ScheduleWork(SetBooleanState, state);

    /*Enabling Interupt Again after ISR was served*/
    os_gpio_enable_irq( CONTACT_PIN, gpio_event_1);
}

static int __irq IRQHandler(uint32_t irqno , void * arg)
{
    BaseType_t xHighterPriority = pdFALSE;

    /*Disabling the Interupt so that we dont get ISR for same level again*/
    os_gpio_disable_irq(CONTACT_PIN);
    GpioValueAfterInterupt = (os_gpio_get_value(CONTACT_PIN)>0) ?  1 : 0;

    /* Starting the Timer */
    if(xTimerStartFromISR(xTimer, &xHighterPriority) != pdPASS)
    {
        ChipLogError(DeviceLayer,"Unable to Start Timer From ISR.");
        os_gpio_enable_irq( CONTACT_PIN, gpio_event_1);
    }

    return IRQ_HANDLED;
}

static void SetBooleanState(bool value)
{
    EmberAfStatus status;
    status = app::Clusters::BooleanState::Attributes::StateValue::Set(1 , (uint8_t)value );
    if(status != EMBER_ZCL_STATUS_SUCCESS)
    {
        ChipLogError(NotSpecified, "ERR: Updating Boolean Status Value %x", status);
        return;
    }
    ChipLogProgress(DeviceLayer,"Boolean State-value is being set to %d ........", (uint8_t)value );
}

static void configureContactSensorGPIO()
{
    os_gpio_request(CONTACT_PIN);
    os_gpio_set_input(CONTACT_PIN);
    os_gpio_set_irq_level_low(CONTACT_PIN);
    os_gpio_set_pull(CONTACT_PIN);
    os_gpio_attach_event(gpio_event_1, IRQHandler, NULL);
    xTimer = xTimerCreate( "DebounceTimer", pdMS_TO_TICKS(40), pdFALSE, (void *)0, DebounceTimer);
    os_gpio_enable_irq(CONTACT_PIN,gpio_event_1);
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

    talariautils::ApplicationInitLog("Matter Contact-sensor-app");
    talariautils::EnableSuspend();
    DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

    app_test();
    configureContactSensorGPIO();
    vTaskSuspend(NULL);

    return 0;
}
