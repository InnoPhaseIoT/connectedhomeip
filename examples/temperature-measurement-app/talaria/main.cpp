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
#define TEMPERATURE_PERIODIC_TIMER 5000
#ifdef UNIT_TEST
void print_test_results(nlTestSuite * tSuite);
void test_suit_proc();
#endif

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

void app_test();

static void PostAttributeChangeCallback(EndpointId endpointId, ClusterId clusterId, AttributeId attributeId, uint8_t type,
                                        uint16_t size, uint8_t * value)
{
    ChipLogDetail(AppServer,
                  "PostAttributeChangeCallback - Cluster ID: '0x%" PRIx32 "', EndPoint ID: '0x%x', Attribute ID: '0x%" PRIx32 "'",
                  clusterId, endpointId, attributeId);

    if (clusterId == TemperatureMeasurement::Id && attributeId == TemperatureMeasurement::Attributes::MeasuredValue::Id)
    {
        ChipLogDetail(Zcl, "Cluster TemperatureMeasurement: attribute MeasuredValue set to %u", *value);
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

static void TemperatureUpdate(int16_t TemperatureValue)
{
    static int MaxTemperatureValue = 0, MinimalTemperatureValue = 100;

    if ( TemperatureValue < MinimalTemperatureValue)
    {
        chip::app::Clusters::TemperatureMeasurement::Attributes::MinMeasuredValue::Set( 1, TemperatureValue);
        MinimalTemperatureValue = TemperatureValue;
    }
    if ( TemperatureValue > MaxTemperatureValue)
    {
        chip::app::Clusters::TemperatureMeasurement::Attributes::MaxMeasuredValue::Set( 1, TemperatureValue);
        MaxTemperatureValue = TemperatureValue;
    }
    chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Set( 1, TemperatureValue);
}

static void TemperatureUpdateTimer(TimerHandle_t xTimer)    
{   
    int16_t PresentTemperatureValue = rand() % 58;
    /* Read the value from Temperature Senosor and fetch the data 
       For present application we are using rand() function to generate randon values*/

    chip::DeviceLayer::PlatformMgr().ScheduleWork(TemperatureUpdate, PresentTemperatureValue);
}

void SoftwareTimerStart()
{
    static TimerHandle_t xTimer;
    xTimer  = xTimerCreate("TemperatureUpdateTimer", pdMS_TO_TICKS(TEMPERATURE_PERIODIC_TIMER), pdTRUE, (void*)0, TemperatureUpdateTimer);
    if ( xTimer != NULL )
    {
        if (xTimerStart( xTimer, 0) != pdPASS)
        {
            ChipLogError(DeviceLayer,"Uneable to Start the Timer");
        }
    }
}

int main(void)
{

    int FactoryReset = os_get_boot_arg_int("matter.factory_reset", 0);
    if (FactoryReset == 1)
    {
        talariautils::FactoryReset();
        vTaskSuspend(NULL);
    }
#ifdef UNIT_TEST
    run_unit_test();
#endif

    talariautils::ApplicationInitLog("Temperature-Measurement-app");
    talariautils::EnableSuspend();
    DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

    app_test();
    SoftwareTimerStart();

    vTaskSuspend(NULL);

    return 0;
}
