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
#include <app/clusters/window-covering-server/window-covering-server.h>
#include <app-common/zap-generated/attributes/Accessors.h>


using namespace chip;
using namespace chip::Platform;
using namespace chip::DeviceLayer;
using namespace chip::Credentials;
using namespace chip::app::Clusters;
using namespace chip::talaria;
using namespace chip::app::Clusters::WindowCovering;

DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

/*-----------------------------------------------------------*/
void test_fn_1();
int TestBitMask();
int chip::RunRegisteredUnitTests();
void print_test_results(nlTestSuite * tSuite);
void test_suit_proc();
void app_test();

int main_TestInetLayer(int argc, char * argv[]);

void DriveCurrentLiftPosition(EndpointId endpoint);
void DriveCurrentTiltPosition(EndpointId endpoint);
Percent100ths CalculateNextPosition(WindowCoveringType type, EndpointId endpointId);

/* Function Declarations */

Percent100ths CalculateNextPosition(WindowCoveringType type, EndpointId endpointId)
{
    EmberAfStatus status{};
    Percent100ths percent100ths{};
    NPercent100ths current{};
    WindowCovering::OperationalState opState{};

    if (type == WindowCoveringType::Lift)
    {
        status  = Attributes::CurrentPositionLiftPercent100ths::Get(endpointId, current);
        opState = OperationalStateGet(endpointId, OperationalStatus::kLift);
    }
    else if (type == WindowCoveringType::Tilt)
    {
        status  = Attributes::CurrentPositionTiltPercent100ths::Get(endpointId, current);
        opState = OperationalStateGet(endpointId, OperationalStatus::kTilt);
    }

    if ((status == EMBER_ZCL_STATUS_SUCCESS) && !current.IsNull())
    {
        static constexpr auto sPercentDelta{ WC_PERCENT100THS_MAX_CLOSED / 20 };
        percent100ths = ComputePercent100thsStep(opState, current.Value(), sPercentDelta);
    }
    else
    {
        ChipLogProgress(DeviceLayer, "Cannot read the current lift position. Error: %d", static_cast<uint8_t>(status));
    }

    return percent100ths;
}

void DriveCurrentLiftPosition(EndpointId endpointId)
{
    NPercent100ths current{};
    NPercent100ths target{};
    NPercent100ths positionToSet{};

    Attributes::CurrentPositionLiftPercent100ths::Get(endpointId, current);
    Attributes::TargetPositionLiftPercent100ths::Get(endpointId, target);

    WindowCovering::OperationalState state = ComputeOperationalState(target, current);
    OperationalStateSet(endpointId, OperationalStatus::kLift, state);

    chip::Percent100ths position = CalculateNextPosition(WindowCoveringType::Lift, endpointId);

    if (state == WindowCovering::OperationalState::MovingUpOrOpen)
    {
        positionToSet.SetNonNull(position > target.Value() ? position : target.Value());
    }
    else if (state == WindowCovering::OperationalState::MovingDownOrClose)
    {
        positionToSet.SetNonNull(position < target.Value() ? position : target.Value());
    }
    else
    {
        positionToSet.SetNonNull(current.Value());
    }


    /* PlaceHolder: Call vendor specific APIs to handle the Lift functionality for
     * up-or-open, down-or-close, go-to-lift-value, go-to-lift-percentage, stop-motion commands */

    LiftPositionSet(endpointId, positionToSet);
    state = ComputeOperationalState(target, current);
    OperationalStateSet(endpointId, OperationalStatus::kLift, state);
}

void DriveCurrentTiltPosition(EndpointId endpointId)
{
    NPercent100ths current{};
    NPercent100ths target{};
    NPercent100ths positionToSet{};

    Attributes::CurrentPositionTiltPercent100ths::Get(endpointId, current);
    Attributes::TargetPositionTiltPercent100ths::Get(endpointId, target);

    WindowCovering::OperationalState state = ComputeOperationalState(target, current);
    OperationalStateSet(endpointId, OperationalStatus::kTilt, state);

    chip::Percent100ths position = CalculateNextPosition(WindowCoveringType::Tilt, endpointId);

    if (state == WindowCovering::OperationalState::MovingUpOrOpen)
    {
        positionToSet.SetNonNull(position > target.Value() ? position : target.Value());
    }
    else if (state == WindowCovering::OperationalState::MovingDownOrClose)
    {
        positionToSet.SetNonNull(position < target.Value() ? position : target.Value());
    }
    else
    {
        positionToSet.SetNonNull(current.Value());
    }


    /* PlaceHolder: Call vendor specific APIs to handle the Tilt functionality for
     * up-or-open, down-or-close, go-to-tilt-value, go-to-tilt-percentage, stop-motion commands */

    TiltPositionSet(endpointId, positionToSet);
    state = ComputeOperationalState(target, current);
    OperationalStateSet(endpointId, OperationalStatus::kTilt, state);
}

static void PostAttributeChangeCallback(EndpointId endpointId, ClusterId clusterId, AttributeId attributeId, uint8_t type,
                                        uint16_t size, uint8_t * value)
{
    ChipLogDetail(AppServer,
                  "PostAttributeChangeCallback - Cluster ID: '0x%" PRIx32 "', EndPoint ID: '0x%x', Attribute ID: '0x%" PRIx32 "'",
                  clusterId, endpointId, attributeId);

    switch (clusterId)
    {
    case WindowCovering::Id:
	VerifyOrExit(endpointId == 1, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));
	switch(attributeId)
	{
	case WindowCovering::Attributes::TargetPositionLiftPercent100ths::Id:
	   DriveCurrentLiftPosition(endpointId);
	   break;
	case WindowCovering::Attributes::TargetPositionTiltPercent100ths::Id:
	   DriveCurrentTiltPosition(endpointId);
	   break;
	default:
	   return;
	}
	break;
    default:
        break;
    }

    os_printf("\nCluster control command addressed. os_free_heap(): %d", os_avail_heap());
exit:
    return;
}

/** @brief Window-cover Cluster Init
 *
 * This function is called when a specific cluster is initialized. It gives the
 * application an opportunity to take care of cluster initialization procedures.
 * It is called exactly once for each endpoint where cluster is present.
 *
 * @param endpoint   Ver.: always
 *
 * emberAfWindowCoveringClusterInitCallback happens before the stack initialize the cluster
 * attributes to the default value.
 * The logic here expects something similar to the deprecated Plugins callback
 * MatterWindowCoveringPluginServerInitCallback.
 *
 */
void emberAfWindowCoveringClusterInitCallback(EndpointId endpoint)
{
    ChipLogError(AppServer, "emberAfWindowCoveringClusterInitCallback");

    app::DataModel::Nullable<chip::Percent100ths> currentPercent100ths;
    app::DataModel::Nullable<chip::Percent100ths> targetPercent100ths;
    app::DataModel::Nullable<chip::Percent> currentPercentage;
    EmberAfStatus status;

    /* Initialize the Window-cover attributes */

    status = Attributes::CurrentPositionLiftPercentage::Get(endpoint, currentPercentage);
    if (currentPercentage.IsNull())
    {
        Attributes::CurrentPositionLiftPercentage::Set(endpoint, 0);
    }

    status = Attributes::CurrentPositionTiltPercentage::Get(endpoint, currentPercentage);
    if (currentPercentage.IsNull())
    {
        Attributes::CurrentPositionTiltPercentage::Set(endpoint, 0);
    }

    status = Attributes::CurrentPositionLiftPercent100ths::Get(endpoint, currentPercent100ths);
    if (currentPercent100ths.IsNull())
    {
        currentPercent100ths.SetNonNull(0);
        Attributes::CurrentPositionLiftPercent100ths::Set(endpoint, 0);
    }

    status = Attributes::TargetPositionLiftPercent100ths::Get(endpoint, targetPercent100ths);
    if (targetPercent100ths.IsNull())
    {
        Attributes::TargetPositionLiftPercent100ths::Set(endpoint, currentPercent100ths);
    }

    status = Attributes::CurrentPositionTiltPercent100ths::Get(endpoint, currentPercent100ths);
    if (currentPercent100ths.IsNull())
    {
        currentPercent100ths.SetNonNull(0);
        Attributes::CurrentPositionTiltPercent100ths::Set(endpoint, 0);
    }

    status = Attributes::TargetPositionTiltPercent100ths::Get(endpoint, targetPercent100ths);
    if (targetPercent100ths.IsNull())
    {
        Attributes::TargetPositionTiltPercent100ths::Set(endpoint, currentPercent100ths);
    }

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
    if (FactoryReset == 1)
    {
        talariautils::FactoryReset();
        while (1)
            vTaskDelay(100000);
    }
#ifdef UNIT_TEST
    run_unit_test();
#endif

    talariautils::ApplicationInitLog("matter window-cover app");
    talariautils::EnableSuspend();

    ConnectivityManager::SEDIntervalsConfig intervalsConfig;
    uint32_t active_interval         = os_get_boot_arg_int("matter.sed.active_interval", 3);
    uint32_t idle_interval           = os_get_boot_arg_int("matter.sed.idle_interval", 3);
    intervalsConfig.ActiveIntervalMS = static_cast<System::Clock::Milliseconds32>(active_interval);
    intervalsConfig.IdleIntervalMS   = static_cast<System::Clock::Milliseconds32>(idle_interval);

    // ConnectivityMgr().SetSEDIntervalsConfig(intervalsConfig);

    DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

    app_test();

    /* There is nothing to do in this task. Putting the task in suspend mode */
    vTaskSuspend(NULL);
    return 0;
}
