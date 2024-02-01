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
// #include <stw_multi_proto/matter_hio.h>
#include "hio/matter.h"
#include "hio/matter_hio.h"
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
// #include <platform/Talaria/TalariaDeviceInfoProvider.h>
#include "custom_msg_exchange_group.h"
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/cluster-enums.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/clusters/door-lock-server/door-lock-server.h>
#include <app/server/Dnssd.h>
#include <app/server/Server.h>
#include <common/Utils.h>

using namespace chip;
using namespace chip::Platform;
using namespace chip::DeviceLayer;
using namespace chip::Credentials;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::DoorLock;
using namespace chip::talaria;

DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

#define LED_PIN 14
#define MAX_USERS 5

struct dl_set_get_user revd_user;
struct dl_set_get_credential revd_credential;

/*-----------------------------------------------------------*/
void test_fn_1();
int TestBitMask();
int chip::RunRegisteredUnitTests();
void print_test_results(nlTestSuite * tSuite);
void test_suit_proc();
void app_test();

void init_commissioning_flow();

SemaphoreHandle_t ServerInitDone;
SemaphoreHandle_t Getdata;
static struct dl_set_get_user userList[MAX_USERS];
static struct dl_set_get_credential credentialsList[MAX_USERS];
static CredentialStruct mCredentials[MAX_USERS][1];

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
        if (attributeId != OnOff::Attributes::OnOff::Id)
        {
            return;
        }
        VerifyOrExit(attributeId == OnOff::Attributes::OnOff::Id,
                     ChipLogError(AppServer, "Unhandled Attribute ID: '0x%" PRIx32 "'", attributeId));
        VerifyOrExit(endpointId == 1, ChipLogError(AppServer, "Unexpected EndPoint ID: `0x%02x'", endpointId));

        /* TODO: Use LED control api call to T2
        AppLED.Set(*value);
        */
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

void emberAfDoorLockClusterInitCallback(EndpointId endpoint)
{
    ChipLogDetail(AppServer, "emberAfDoorLockClusterInitCallback");
}

//bool emberAfPluginDoorLockOnDoorLockCommand(chip::EndpointId endpointId, const Optional<ByteSpan> & pinCode,
  //                                          OperationErrorEnum & err)
bool emberAfPluginDoorLockOnDoorLockCommand(chip::EndpointId endpointId, const Nullable<chip::FabricIndex> & fabricIdx, const Nullable<chip::NodeId> & nodeId, 
                                            const Optional<chip::ByteSpan> & pinCode,
                                            OperationErrorEnum & err)
{
    ChipLogProgress(Zcl, "Door Lock App: lock Command endpoint=%d", endpointId);
    struct dl_unlock_with_timeout time;
    time.unlock_time_out = 1000;
    int payload          = sizeof(struct dl_unlock_with_timeout);

    matter_doorlock_notify(DOOR_LOCK, LOCK_DOOR, payload, (struct dl_unlock_with_timeout *) &time);
    chip::app::Clusters::DoorLock::Attributes::LockState::Set(endpointId, chip::app::Clusters::DoorLock::DlLockState::kLocked);
    return true;
}

void emberAfPluginDoorLockOnAutoRelock(chip::EndpointId endpointId)
{
    ChipLogProgress(Zcl, "Door Lock App: lock Command endpoint=%d", endpointId);
    struct dl_unlock_with_timeout time;
    time.unlock_time_out = 1000;
    int payload          = sizeof(struct dl_unlock_with_timeout);

    matter_doorlock_notify(DOOR_LOCK, LOCK_DOOR, payload, (struct dl_unlock_with_timeout *) &time);
    chip::app::Clusters::DoorLock::Attributes::LockState::Set(endpointId, chip::app::Clusters::DoorLock::DlLockState::kLocked);
    return true;
}

// bool emberAfPluginDoorLockOnDoorUnlockCommand(chip::EndpointId endpointId, const Optional<ByteSpan> & pinCode,
//                                               OperationErrorEnum & err)
bool emberAfPluginDoorLockOnDoorUnlockCommand(chip::EndpointId endpointId, const Nullable<chip::FabricIndex> & fabricIdx,
                                              const Nullable<chip::NodeId> & nodeId, const Optional<chip::ByteSpan> & pinCode,
                                              OperationErrorEnum & err)
{
    ChipLogProgress(Zcl, "Door Lock App: Unlock Command endpoint=%d", endpointId);
    struct dl_unlock_with_timeout time;
    time.unlock_time_out = 1000;
    int payload          = sizeof(struct dl_unlock_with_timeout);

    matter_doorlock_notify(DOOR_LOCK, UNLOCK_DOOR, payload, (struct dl_unlock_with_timeout *) &time);
    chip::app::Clusters::DoorLock::Attributes::LockState::Set(endpointId, chip::app::Clusters::DoorLock::DlLockState::kUnlocked);
    return true;
}

bool emberAfPluginDoorLockSetUser(chip::EndpointId endpointId, uint16_t userIndex, chip::FabricIndex creator,
                                  chip::FabricIndex modifier, const chip::CharSpan & userName, uint32_t uniqueId,
                                  UserStatusEnum userStatus, UserTypeEnum usertype, CredentialRuleEnum credentialRule,
                                  const CredentialStruct * credentials, size_t totalCredentials)
{
    int payload = 0;
    struct dl_set_get_user * setUser;
    uint32_t cmd = SET_USER;

    ChipLogProgress(Zcl,
                    "Door Lock App: SetUser "
                    "[endpoint=%d,userIndex=%d,creator=%d,modifier=%d,userName=%s,uniqueId=%ld "
                    "userStatus=%u,userType=%u,credentialRule=%u,credentials=%p,totalCredentials=%u]",
                    endpointId, userIndex, creator, modifier, userName.data(), uniqueId, to_underlying(userStatus),
                    to_underlying(usertype), to_underlying(credentialRule), credentials, totalCredentials);

    userIndex--;
    if (userIndex >= MAX_USERS || userIndex < 0)
    {
        ChipLogError(Zcl, "SetUser: Max supported User Index is 1 - %d. Got: %d", MAX_USERS, (userIndex + 1));
        return false;
    }
    if (totalCredentials > 1)
    {
        ChipLogError(Zcl, "SetUser: Max supported Credentials per user is 1. Got: %d", totalCredentials);
        return false;
    }
    setUser = &userList[userIndex];
    // memset(setUser, 0, sizeof(struct dl_set_get_user));
    if (userStatus == chip::app::Clusters::DoorLock::UserStatusEnum::kAvailable)
    {
        setUser->operationtype = (uint8_t) chip::app::Clusters::DoorLock::DataOperationTypeEnum::kClear;
        cmd                    = CLEAR_USER;
    }
    else if ((setUser->userstatus == (uint8_t) chip::app::Clusters::DoorLock::UserStatusEnum::kOccupiedEnabled ||
              setUser->userstatus == (uint8_t) chip::app::Clusters::DoorLock::UserStatusEnum::kOccupiedDisabled) &&
             userStatus != chip::app::Clusters::DoorLock::UserStatusEnum::kAvailable)
    {
        setUser->operationtype = (uint8_t) chip::app::Clusters::DoorLock::DataOperationTypeEnum::kAdd;
    }
    else
    {
        setUser->operationtype = (uint8_t) chip::app::Clusters::DoorLock::DataOperationTypeEnum::kAdd;
    }
    setUser->userindex      = userIndex + 1;
    setUser->userstatus     = (uint8_t) userStatus;
    setUser->usertype       = (uint8_t) usertype;
    setUser->useruniqueid   = uniqueId;
    setUser->credentialrule = (uint8_t) credentialRule;
    chip::Platform::CopyString(setUser->username, userName);
    for (size_t i = 0; i < totalCredentials; ++i)
    {
        mCredentials[userIndex][i] = credentials[i];
    }
    if (cmd == CLEAR_USER)
    {
        memset(&(mCredentials[userIndex][0].credentialIndex), 0xff, sizeof(uint16_t));
    }

    payload = sizeof(struct dl_set_get_user);
    matter_doorlock_notify(DOOR_LOCK, cmd, payload, (struct dl_set_get_user *) setUser);

    ChipLogProgress(Zcl, "Successfully set the user [mEndpointId=%d,index=%d] operationtype: %d", endpointId, userIndex,
                    setUser->operationtype);

    return true;
}

bool emberAfPluginDoorLockGetUser(chip::EndpointId endpointId, uint16_t userIndex, EmberAfPluginDoorLockUserInfo & user)
{
    userIndex--;
    if (userIndex >= MAX_USERS || userIndex < 0)
    {
        ChipLogError(Zcl, "GetUser: Max supported User Index is 1 - %d. Got: %d", MAX_USERS, (userIndex + 1));
        return false;
    }
    struct dl_set_get_user * getUser;
    getUser = &userList[userIndex];

    user.userName       = chip::CharSpan(getUser->username, sizeof(getUser->username));
    user.userUniqueId   = getUser->useruniqueid;
    user.userType       = (chip::app::Clusters::DoorLock::UserTypeEnum) getUser->usertype;
    user.credentialRule = (chip::app::Clusters::DoorLock::CredentialRuleEnum) getUser->credentialrule;
    user.userStatus     = (chip::app::Clusters::DoorLock::UserStatusEnum) getUser->userstatus;
    if (credentialsList[userIndex].userstatus != (uint8_t) DlCredentialStatus::kAvailable)
    {
        user.credentials = chip::Span<const CredentialStruct>(mCredentials[userIndex], 1);
    }
    else
    {
        user.credentials = chip::Span<const CredentialStruct>(mCredentials[userIndex], 0);
    }

    ChipLogError(Zcl,
                 "Found occupied user "
                 "[endpoint=%d,name=\"%.*s\",credentialsCount=%u,uniqueId=%lx,type=%u,credentialRule=%u,"
                 "createdBy=%d,lastModifiedBy=%d]",
                 endpointId, static_cast<int>(user.userName.size()), user.userName.data(), user.credentials.size(),
                 user.userUniqueId, to_underlying(user.userType), to_underlying(user.credentialRule), user.createdBy,
                 user.lastModifiedBy);
    return true;
}

bool emberAfPluginDoorLockSetCredential(chip::EndpointId endpointId, uint16_t credentialIndex, chip::FabricIndex creator,
                                        chip::FabricIndex modifier, DlCredentialStatus credentialStatus,
                                        CredentialTypeEnum credentialType, const chip::ByteSpan & credentialData)
{
    uint32_t cmd = SET_CREDENTIAL;
    ChipLogProgress(Zcl,
                    "Door Lock App: SetCredential "
                    "[endpoint=%d,credentialIndex=%d,creator=%d,modifier=%d "
                    "credentialStatus=%u,credentialType=%u,credentialData=%s]",
                    endpointId, credentialIndex, creator, modifier, to_underlying(credentialStatus), to_underlying(credentialType),
                    credentialData.data());

    credentialIndex--;
    if (credentialIndex < 0 || credentialIndex >= MAX_USERS)
    {
        ChipLogError(Zcl, "GetCredential: Max supported Credential Index is 1 - %d. Got: %d", MAX_USERS, (credentialIndex + 1));
        return false;
    }
    struct dl_set_get_credential * setCredential;
    setCredential = &credentialsList[credentialIndex];
    int payload   = sizeof(struct dl_set_get_credential);
    // memset(setCredential, 0, sizeof(struct dl_set_get_credential));

    if (credentialStatus == DlCredentialStatus::kAvailable)
    {
        cmd                          = CLEAR_CREDENTIAL;
        setCredential->operationtype = (uint8_t) chip::app::Clusters::DoorLock::DataOperationTypeEnum::kClear;
        memset(&(mCredentials[credentialIndex][0].credentialIndex), 0xff, sizeof(uint16_t));
    }
    else
    {
        setCredential->operationtype = (uint8_t) chip::app::Clusters::DoorLock::DataOperationTypeEnum::kAdd;
    }

    setCredential->userindex  = credentialIndex + 1;
    setCredential->userstatus = (uint8_t) credentialStatus;
    setCredential->usertype   = (uint8_t) credentialType;
    memcpy(setCredential->credentialdata, credentialData.data(), min(credentialData.size(), sizeof(setCredential->credentialdata)));

    matter_doorlock_notify(DOOR_LOCK, cmd, payload, (struct dl_set_get_credential *) setCredential);
    ChipLogProgress(Zcl, "Successfully set the Credential [mEndpointId=%d,index=%d] operationType: %d", endpointId, credentialIndex,
                    setCredential->operationtype);
    return true;
}

bool emberAfPluginDoorLockGetCredential(chip::EndpointId endpointId, uint16_t credentialIndex, CredentialTypeEnum credentialType,
                                        EmberAfPluginDoorLockCredentialInfo & credential)
{
    struct dl_set_get_credential * getCredential;
    credentialIndex--;
    if (credentialIndex < 0 || credentialIndex >= MAX_USERS)
    {
        ChipLogError(Zcl, "GetCredential: Max supported Credential Index is 1 - %d. Got: %d", MAX_USERS, (credentialIndex + 1));
        return false;
    }

    getCredential             = &credentialsList[credentialIndex];
    credential.status         = (DlCredentialStatus) getCredential->userstatus;
    credential.credentialType = (chip::app::Clusters::DoorLock::CredentialTypeEnum) getCredential->usertype;
    if (getCredential->userstatus != (uint8_t) DlCredentialStatus::kAvailable)
    {
        credential.credentialData =
            chip::ByteSpan((uint8_t *) &getCredential->credentialdata[0], strlen(getCredential->credentialdata));
    }

    ChipLogProgress(Zcl,
                    "Door Lock App: GetCredential "
                    "[endpoint=%d,credentialIndex=%d,creator=%d,modifier=%d "
                    "credentialStatus=%d,credentialType=%u,credentialData=%s]",
                    endpointId, credentialIndex + 1, 0, 0, credential.status, 0, credential.credentialData.data());
    return true;
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

void load_stored_info_from_host()
{
    int payload = 0;
    struct dl_set_get_user * getUser;
    struct dl_set_get_credential * getCredential;

    /* Load USer info and Credentials information from the host */
    Getdata = xSemaphoreCreateCounting(1, 0);

    for (int i = 0; i < MAX_USERS; i++)
    {
        getUser = &userList[i];
        memset(getUser, 0, sizeof(struct dl_set_get_user));
        getUser->userindex = i + 1;
        payload            = sizeof(struct dl_set_get_user);
        matter_doorlock_notify(DOOR_LOCK, GET_USER, payload, (struct dl_set_get_user *) getUser);
        if (xSemaphoreTake(Getdata, portMAX_DELAY) == pdFAIL)
        {
            os_printf("Unable to wait on semaphore...!!\n");
            continue;
        }
        memcpy(getUser, &revd_user, sizeof(struct dl_set_get_user));
    }
    for (int i = 0; i < MAX_USERS; i++)
    {
        getCredential = &credentialsList[i];
        memset(getCredential, 0, sizeof(struct dl_set_get_credential));
        getCredential->userindex = i + 1;
        payload                  = sizeof(struct dl_set_get_credential);
        matter_doorlock_notify(DOOR_LOCK, GET_CREDENTIAL_STATUS, payload, (struct dl_set_get_credential *) getCredential);
        if (xSemaphoreTake(Getdata, portMAX_DELAY) == pdFAIL)
        {
            os_printf("Unable to wait on semaphore...!!\n");
            continue;
        }
        memcpy(getCredential, &revd_credential, sizeof(struct dl_set_get_credential));
        mCredentials[i][0].credentialType  = (chip::app::Clusters::DoorLock::CredentialTypeEnum) getCredential->usertype;
        mCredentials[i][0].credentialIndex = getCredential->userindex;
        // os_printf("\r\n credIndex: %d, credType: %d \r\n", getCredential->userindex, getCredential->usertype);
    }
    os_printf("\nReading User Lock data from host is completed...!!\n");
    vSemaphoreDelete(Getdata);
    vTaskDelete(NULL);
}

BOOTARG_INT("disp_pkt_info", "display packet info", "1 to enable packet info print. 0(default) to disable");

__irq void show_packet_info(char * name, struct packet * pkt, int count)
{
    const struct hio_msghdr * hdr;
    hdr            = packet_data(pkt);
    uint32_t group = hdr->group, msgid = hdr->msgid, trxid = hdr->trxid;
    os_printf("hio: %s-hook: count=%04d,group=0x%02x,msgid=0x%02x,trxid=%04d\n", name, count, group, msgid, trxid);
}

__irq void hio_input_pkt_hook(struct packet * pkt)
{
    static int tot_in = 0;
    show_packet_info(" input", pkt, tot_in++);
}

__irq void hio_output_pkt_hook(struct packet * pkt)
{
    static int tot_out = 0;
    show_packet_info("output", pkt, tot_out++);
}

void register_hio_packet_hook()
{
    int ret;
    void hio_packet_hook_unregister(void);
    hio_packet_hook_unregister();
    os_printf("calling hio_packet_hook_register\n");
    ret = hio_packet_hook_register(hio_input_pkt_hook, hio_output_pkt_hook);
    os_printf("register = %d\n", ret);
}

void chk_and_register_pkt_hook()
{
    if (os_get_boot_arg_int("disp_pkt_info", 0) != 0)
    {
        /* Register packet hook.
         * Hook will print the msg_id and group_id of every packets sent and received
         */
        register_hio_packet_hook();
    }
}

int main(void)
{
#ifdef UNIT_TEST
    run_unit_test();
#endif

    talariautils::ApplicationInitLog("matter lock app");

    matter_hio_init();

    chk_and_register_pkt_hook();
    /* Delay is required before start doing the communication over hio,
       otherwise don't see any response*/
    vTaskDelay(pdMS_TO_TICKS(1000));

    int FactoryReset = os_get_boot_arg_int("matter.factory_reset", 0);
    if (FactoryReset == 1)
    {
        talariautils::FactoryReset();

        /* Factory Reset the host side data as well */
        struct dl_set_get_user setUser;
        struct dl_set_get_credential setCredential;
        memset(&setUser, 0, sizeof(setUser));
        memset(&setCredential, 0, sizeof(setCredential));
        os_printf("\n Clearing User Data from host....");
        for (int i = 1; i <= MAX_USERS; i++)
        {
            setUser.userindex       = i;
            setCredential.userindex = i;
            matter_doorlock_notify(DOOR_LOCK, CLEAR_USER, sizeof(setUser), &setUser);
            os_printf(".");
            matter_doorlock_notify(DOOR_LOCK, CLEAR_CREDENTIAL, sizeof(setCredential), &setCredential);
            os_printf(".");
        }

        os_printf("\n Cleared User Data from host Completed successfully \n");
        os_printf("\nFACTORY RESET is completed ...........");

        while (1)
            vTaskDelay(100000);
    }

    BaseType_t xReturned;
    TaskHandle_t xHandle = NULL;

    /* Create the task to load the persistent data from hsot */
    xReturned = xTaskCreate(load_stored_info_from_host,   /* Function that implements the task. */
                            "load_stored_info_from_host", /* Text name for the task. */
                            configMINIMAL_STACK_SIZE,     /* Stack size in words, not bytes. */
                            (void *) NULL,                /* Parameter passed into the task. */
                            tskIDLE_PRIORITY + 2,         /* Priority at which the task is created. */
                            &xHandle);                    /* Used to pass out the created task's handle. */

    if (xReturned == pdPASS)
    {
        /* The task was created.  Use the task's handle to delete the task. */
        os_printf("Loading user data from host task created...\n");
    }

    DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);
    ServerInitDone = xSemaphoreCreateCounting(1, 0);

    app_test();
    if (xSemaphoreTake(ServerInitDone, portMAX_DELAY) == pdFAIL)
    {
        os_printf("Unable to wait on semaphore...!!\n");
    }

    os_printf("After Server initialization completed. os_free_heap(): %d\n", os_avail_heap());

    os_printf("\n");
    while (1)
    {
        vTaskDelay(10000);
        os_printf(".");
    }

    return 0;
}
