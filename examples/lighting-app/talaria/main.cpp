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
// #include <lib/support/CodeUtils.h>
#include <lib/support/UnitTestRegistration.h>
#include <platform/talaria/DeviceInfoProviderImpl.h>
// #include <platform/talaria/TalariaDeviceInfoProvider.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/server/Server.h>
#include <app-common/zap-generated/attributes/Accessors.h>

#include <common/Utils.h>
#include <platform/ConnectivityManager.h>
#include <platform/talaria/TalariaUtils.h>

using namespace chip;
using namespace chip::Platform;
using namespace chip::DeviceLayer;
using namespace chip::Credentials;
using namespace chip::app::Clusters;
using namespace chip::talaria;

DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

#define LED_PIN 14
#define IDENTIFY_TIMER_DELAY_MS  1000

/*-----------------------------------------------------------*/
void test_fn_1();
int TestBitMask();
int chip::RunRegisteredUnitTests();
void print_test_results(nlTestSuite * tSuite);
void test_suit_proc();
void app_test();

int main_TestInetLayer(int argc, char * argv[]);
static uint32_t identifyTimerCount = 0;

/* Function Declarations */
static void CommonDeviceEventHandler(const chip::DeviceLayer::ChipDeviceEvent * event, intptr_t arg);


static void Toggle(EndpointId endpointId )
{
    bool value;
    chip::app::Clusters::OnOff::Attributes::OnOff::Get(endpointId , &value);
    if(value)
    {
        os_gpio_clr_pin(1 << LED_PIN);
        chip::app::Clusters::OnOff::Attributes::OnOff::Set(endpointId, false);
    }
    else
    {
        os_gpio_set_pin(1 << LED_PIN);
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
            os_gpio_set_pin(1 << LED_PIN);
        }
        else
        {
            os_gpio_clr_pin(1 << LED_PIN);
        }
        break;
    case app::Clusters::Identify::Id:
        OnIdentifyPostAttributeChangeCallback(endpointId, attributeId, value);
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

void emberAfIdentifyClusterInitCallback(chip::EndpointId endpoint)
{
    chip::app::Clusters::Identify::Attributes::IdentifyType::Set(endpoint, chip::app::Clusters::Identify::IdentifyTypeEnum::kLightOutput);
    chip::app::Clusters::Identify::Attributes::IdentifyTime::Set(endpoint, 0);
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

    talariautils::ApplicationInitLog("matter lighting app");
    talariautils::EnableSuspend();
    int led_pin = 1 << LED_PIN;
    os_gpio_request(led_pin);
    os_gpio_set_output(led_pin);
    os_gpio_clr_pin(led_pin);

    DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

    app_test();

    /* There is nothing to do in this task. Putting the task in suspend mode */
    vTaskSuspend(NULL);
    return 0;
}
