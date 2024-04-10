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
#include <app-common/zap-generated/ids/Clusters.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/cluster-objects.h>
#include "media-playback/MediaPlaybackManager.h"
#include "keypad-input/KeypadInputManager.h"

using namespace chip;
using namespace chip::Platform;
using namespace chip::DeviceLayer;
using namespace chip::Credentials;
using namespace chip::app::Clusters;
using namespace chip::talaria;
using namespace chip::app::Clusters::OnOff;
using namespace chip::app::Clusters::LevelControl;
using namespace chip::app::Clusters::Identify;
using namespace chip::app::Clusters::AudioOutput;
using namespace chip::app::Clusters::MediaInput;
using namespace chip::app::Clusters::MediaPlayback;

DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;
SemaphoreHandle_t ServerInitDone;
SemaphoreHandle_t Getdata;


namespace {
static MediaPlaybackManager mediaPlaybackManager;
static KeypadInputManager keypadInputManager;
}

/*-----------------------------------------------------------*/

void print_test_results(nlTestSuite * tSuite);
void app_test();

/* Function Declarations */


static void OnOffPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    if (attributeId == OnOff::Attributes::OnOff::Id)
    {
	    struct media_playback_set_data input_data;
	    int payload = sizeof(struct media_playback_set_data);
	    input_data.onoff = *value;

	    if (*value)
	    {
		    ChipLogProgress(AppServer, "[MediaPlayback] ON Command Received");
		    int ret = matter_notify(BASIC_VIDEO_PLAYER, MEDIA_PLAYBACK_ON, payload, (struct media_playback_set_data *) &input_data);
		    if(ret != 0)
		    {
			    ChipLogProgress(AppServer, "[MediaPlayback] ON Command Failed, error: %d", ret);
		    }
		    else
		    {
			    ChipLogProgress(AppServer, "[MediaPlayback] ON Command Successfull");
		    }
	    }
	    else
	    {
		    ChipLogProgress(AppServer, "[MediaPlayback] OFF Command Received");
		    int ret = matter_notify(BASIC_VIDEO_PLAYER, MEDIA_PLAYBACK_OFF, payload, (struct media_playback_set_data *) &input_data);
		    if(ret != 0)
		    {
			    ChipLogProgress(AppServer, "[MediaPlayback] OFF Command Failed, error: %d", ret);
		    }
		    else
		    {
			    ChipLogProgress(AppServer, "[MediaPlayback] OFF Command Successfull");
		    }
	    }
    }
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
        VerifyOrExit(endpointId == 1, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));
	OnOffPostAttributeChangeCallback(endpointId, attributeId, value);
	break;
    default:
	break;
    }

    os_printf("\nCluster control command addressed. os_free_heap(): %d", os_avail_heap());
exit:
    return;
}


void emberAfMediaPlaybackClusterInitCallback(EndpointId endpoint)
{
    ChipLogDetail(AppServer, "emberAfMediaPlaybackClusterInitCallback");
    MediaPlayback::SetDefaultDelegate(endpoint, &mediaPlaybackManager);
}

void emberAfKeypadInputClusterInitCallback(EndpointId endpoint)
{
    ChipLogDetail(AppServer, "emberAfKeypadInputClusterInitCallback");
    KeypadInput::SetDefaultDelegate(endpoint, &keypadInputManager);
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

BOOTARG_INT("disp_pkt_info", "display packet info", "1 to enable packet info print. 0(default) to disable");


int main(void)
{
    talariautils::ApplicationInitLog("matter basic video-player app");
    matter_hio_init();

    /* Delay is required before start doing the communication over hio,
       otherwise don't see any response*/
    vTaskDelay(pdMS_TO_TICKS(1000));

    int FactoryReset = os_get_boot_arg_int("matter.factory_reset", 0);
    if (FactoryReset == 1 || FactoryReset == 2)
    {
        talariautils::FactoryReset(FactoryReset);
        while (1)
            vTaskSuspend(NULL);
    }
#ifdef UNIT_TEST
    run_unit_test();
#endif

    talariautils::EnableSuspend();

    DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

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
