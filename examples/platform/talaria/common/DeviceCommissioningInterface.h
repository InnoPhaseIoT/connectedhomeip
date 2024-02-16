#pragma once

#include <lib/core/CHIPError.h>
#include <common/DeviceCommissioningManager.h>
#include <common/Utils.h>

namespace chip
{
namespace talaria
{
namespace DeviceCommissioning
{
    class CommissioningInterface
    {
    public:
        struct StdCommissioningInterfaceParam;
        struct UiCommissioningInterfaceParam;
        struct CustCommissioningInterfaceParam;
        union CommissioningTypedParam;
        struct CommissioningParam;

        static CHIP_ERROR Init(CommissioningParam &param);
        static CHIP_ERROR EnableCommissioning();

    private:
        CommissioningInterface() = delete;
        static CommissioningManager* mCommissioningMgr;

    protected:

    };

    struct CommissioningInterface::StdCommissioningInterfaceParam
    {

    };

    struct CommissioningInterface::UiCommissioningInterfaceParam
    {
        uint8_t TriggerGpio;
    };

    struct CommissioningInterface::CustCommissioningInterfaceParam
    {

    };

    union CommissioningInterface::CommissioningTypedParam
    {
        struct CommissioningInterface::StdCommissioningInterfaceParam Standard;
        struct CommissioningInterface::UiCommissioningInterfaceParam UserIntent;
        struct CommissioningInterface::CustCommissioningInterfaceParam Custom;
    };

    struct CommissioningInterface::CommissioningParam
    {
        chip::talaria::matterutils::CommissioningFlowType CommissioningFlowType;
        union CommissioningInterface::CommissioningTypedParam TypedParam;
    };

} // namespace DeviceCommissioning
} // namespace talaria
} // namespace CHIP

/*-----------------------------------------------------------*/
