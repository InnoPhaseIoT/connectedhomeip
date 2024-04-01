

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <kernel/callout.h>
#include <kernel/gpio.h>
#include <talaria_two.h>

void print_ver(char * banner, int print_sdk_name, int print_emb_app_ver);
#ifdef __cplusplus
}
#endif

#include <app/server/Server.h>
#include <common/Utils.h>
#include <platform/CommissionableDataProvider.h>
#include <platform/DeviceInstanceInfoProvider.h>
#include <platform/PlatformManager.h>
#include <platform/talaria/Config.h>

namespace chip {
namespace talaria {
namespace talariautils {
void ApplicationInitLog(char * str)
{
    print_ver(str, 1, 1);
    os_printf("Talaria Matter build: %s %s\n", __DATE__, __TIME__);
    HeapAvailableLog();
}

void HeapAvailableLog()
{
    os_printf("Heap avaialble: %d\n", xPortGetFreeHeapSize());
}

void EnableSuspend()
{
    if (os_get_boot_arg_int("suspend", 0) == 1)
    {
        os_suspend_enable();
    }
}

void FactoryReset(int factory_reset_level)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    os_printf("\e[1;43;31;5m");
    os_printf("\n*******************************\n");
    os_printf("\nFACTORY RESET\n");
    os_printf("\n*******************************\n");
    err = Platform::MemoryInit();
    os_printf("\nMemoryInit err %d, %s", err.AsInteger(), err.AsString());
    err = DeviceLayer::PlatformMgr().InitChipStack();
    os_printf("\nInitChipStack err %d, %s", err.AsInteger(), err.AsString());
    err = DeviceLayer::PlatformMgr().StartEventLoopTask();
    os_printf("\nStartEventLoopTaks err %d, %s", err.AsInteger(), err.AsString());

    /* The Factory Reset is distributed into 2 levels,
       level 1: Reset to Factory Default values. e.g. counters and configs
       level 2: Full factory reset, which removes the kvs and counters and configs
    */
    if (factory_reset_level >= 1) {
        chip::DeviceLayer::Internal::TalariaConfig::FactoryDefaultConfigCotunters();
    }
    if (factory_reset_level == 2) {
        chip::Server::GetInstance().ScheduleFactoryReset();
    }

    os_printf("\n%s is completed ...........", (factory_reset_level == 1)? "Factory Default": "Full Factory Reset");
    os_printf("\nProgram T2 elf without FactoryReset to commission the device \n");
    os_printf("\n*******************************");
    os_printf("\e[0m");
}
} // namespace talariautils

namespace matterutils {
void MatterConfigLog()
{
    uint32_t Passcode        = 0;
    uint16_t Discriminator   = 0;
    uint16_t VendorId        = 0;
    uint16_t ProductId       = 0;
    uint16_t HardwareVersion = 0;

    chip::DeviceLayer::GetCommissionableDataProvider()->GetSetupPasscode(Passcode);
    chip::DeviceLayer::GetDeviceInstanceInfoProvider()->GetProductId(ProductId);
    chip::DeviceLayer::GetCommissionableDataProvider()->GetSetupDiscriminator(Discriminator);
    chip::DeviceLayer::GetDeviceInstanceInfoProvider()->GetVendorId(VendorId);
    chip::DeviceLayer::GetDeviceInstanceInfoProvider()->GetHardwareVersion(HardwareVersion);
    os_printf("\e[0;32m");
    os_printf("\nMatter config ********************* >>>");
    os_printf("\nVendorId:\t\t%d (0x%X) ", VendorId, VendorId);
    os_printf("\nProductId:\t\t%d (0x%X) ", ProductId, ProductId);
    os_printf("\nHardwareVersion:\t%d ", HardwareVersion);
    os_printf("\nSetupPasscode:\t\t%d ", Passcode);
    os_printf("\nDiscriminator:\t\t%d (0x%X) ", Discriminator, Discriminator);
    os_printf("\ncommissioned:\t\t%s [%d]", (IsNodeCommissioned() == true) ? "done" : "no", FabricCount());
    os_printf("\n************* ********************* >>>");
    os_printf("\e[0m");
    talariautils::HeapAvailableLog();
}

bool IsNodeCommissioned()
{
    bool RetVal = false;
    if (FabricCount() != 0)
    {
        RetVal = true;
    }
    return RetVal;
}

bool IsNodePrimaryCommissioned()
{
    bool RetVal = false;
    if (FabricCount() == 1)
    {
        RetVal = true;
    }
    return RetVal;
}

bool IsNodeSecondaryCommissioned()
{
    bool RetVal = false;
    if (FabricCount() >= 1)
    {
        RetVal = true;
    }
    return RetVal;
}

CommissionedStatus GetNodeCommissionedStatus()
{
    CommissionedStatus RetVal;
    uint8_t count = FabricCount();

    if (count == 0)
    {
        RetVal = NOT_COMMISSIONED;
    }
    else if (count == 1)
    {
        RetVal = PRIMARY_COMMISSIONED;
    }
    else if (count == 2)
    {
        RetVal = SECONDARY_COMMISSIONED;
    }
    else if (count > 2)
    {
        RetVal = MULTIPLE_COMMISSIONED;
    }

    return RetVal;
}

uint8_t FabricCount()
{
    return Server::GetInstance().GetFabricTable().FabricCount();
}

CommissioningFlowType GetCommissioningFlowType()
{
    CommissioningFlowType RetVal;
    int Type = os_get_boot_arg_int("matter.commissioning.flow_type", 0);

    switch (Type)
    {
    case 0:
        RetVal = COMMISSIONING_FLOW_TYPE_STANDARD;
        break;
    case 1:
        RetVal = COMMISSIONING_FLOW_TYPE_USER_INTENT;
        break;
    case 2:
        RetVal = COMMISSIONING_FLOW_TYPE_CUSTOM;
        break;
    case 3:
        RetVal = COMMISSIONING_FLOW_TYPE_INVALID;
        break;
    default:
        RetVal = COMMISSIONING_FLOW_TYPE_INVALID;
        break;
    }

    return RetVal;
}

} // namespace matterutils

} // namespace talaria
} // namespace chip

/*-----------------------------------------------------------*/
