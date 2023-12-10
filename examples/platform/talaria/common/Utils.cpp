

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <talaria_two.h>
#include <kernel/gpio.h>
#include <kernel/callout.h>

void print_ver(char *banner, int print_sdk_name, int print_emb_app_ver);
#ifdef __cplusplus
}
#endif

#include <common/Utils.h>
#include <app/server/Server.h>
#include <platform/CommissionableDataProvider.h>
#include <platform/DeviceInstanceInfoProvider.h>

namespace chip
{
namespace talaria
{
namespace talariautils
{
    void ApplicationInitLog(char *str)
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
        if(os_get_boot_arg_int("suspend", 0) == 1)
        {
            os_suspend_enable();
        }
    }
} // namespace talariautils

namespace matterutils
{
    void MatterConfigLog()
    {
        uint32_t Passcode = 0;
        uint16_t Discriminator = 0;
        uint16_t VendorId = 0;
        uint16_t ProductId = 0;
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
        if(FabricCount() != 0)
        {
            RetVal = true;
        }
        return RetVal;
    }

    bool IsNodePrimaryCommissioned()
    {
        bool RetVal = false;
        if(FabricCount() == 1)
        {
            RetVal = true;
        }
        return RetVal;
    }

    bool IsNodeSecondaryCommissioned()
    {
        bool RetVal = false;
        if(FabricCount() >= 1)
        {
            RetVal = true;
        }
        return RetVal;
    }

    CommissionedStatus GetNodeCommissionedStatus()
    {
        CommissionedStatus RetVal;
        uint8_t count = FabricCount();

        if(count == 0)
        {
            RetVal = NOT_COMMISSIONED;
        }
        else if(count == 1)
        {
            RetVal = PRIMARY_COMMISSIONED;
        }
        else if(count == 2)
        {
            RetVal = SECONDARY_COMMISSIONED;
        }
        else if(count > 2)
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

        switch(Type)
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
} // namespace CHIP

/*-----------------------------------------------------------*/
