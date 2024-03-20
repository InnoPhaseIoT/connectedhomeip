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
#include <app/clusters/air-quality-server/air-quality-server.h>

/* Periodic timeout for AQS software timer in ms */
#define AQS_PERIODIC_TIME_OUT_MS 5000

using namespace chip;
using namespace chip::Platform;
using namespace chip::DeviceLayer;
using namespace chip::Credentials;
using namespace chip::app::Clusters;
using namespace chip::talaria;
using namespace chip::app::Clusters::AirQuality;

DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

/*-----------------------------------------------------------*/

void test_fn_1();
int TestBitMask();
int chip::RunRegisteredUnitTests();
void print_test_results(nlTestSuite * tSuite);
void test_suit_proc();
void app_test();

int main_TestInetLayer(int argc, char * argv[]);

int SoftwareTimer_Init(void);

static chip::BitMask<Feature, uint32_t> airQualityFeatures(Feature::kFair, Feature::kModerate, Feature::kVeryPoor,
		                                           Feature::kExtremelyPoor);
static Instance mAirQualityInstance = Instance(1, airQualityFeatures);
AirQualityEnum airQualityStatus;


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

int SoftwareTimer_Init()
{
    static TimerHandle_t xTimer = NULL;

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
    return 0;
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

    app_test();

    /* There is nothing to do in this task. Putting the task in suspend mode */
    vTaskSuspend(NULL);
    return 0;
}
