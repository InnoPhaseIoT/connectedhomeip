
#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <talaria_two.h>
#include <kernel/gpio.h>
#include <kernel/callout.h>

#ifdef __cplusplus
}
#endif

#include <common/DeviceCommissioningInterface.h>

using namespace chip;
using namespace chip::talaria::talariautils;
using namespace chip::talaria::matterutils;

namespace chip
{
namespace talaria
{
namespace DeviceCommissioning
{
    TalariaDerivedAppDelegate DerivedAppDelegate;
    CommissioningManager *CommissioningInterface::mCommissioningMgr = nullptr;

    void EventHandler(const DeviceLayer::ChipDeviceEvent * event, intptr_t arg)
    {
        if (event->Type == DeviceLayer::DeviceEventType::kCommissioningComplete)
        {
            ChipLogProgress(DeviceLayer, "Receive kCommissioningComplete");
            CommissioningSessionInprogress =  false;
            chip::DeviceLayer::Internal::TalariaUtils::RestoreWcmPMConfig();
        }
    }

    CHIP_ERROR CommissioningInterface::Init(CommissioningParam &param)
    {
        DeviceLayer::PlatformMgrImpl().AddEventHandler(EventHandler, 0);

        if(mCommissioningMgr != nullptr)
            return CHIP_NO_ERROR;

        switch(param.CommissioningFlowType)
        {
            case COMMISSIONING_FLOW_TYPE_STANDARD:
            {
                struct StandardCommissioningManager::StdCommissioningParam StdParam;
                mCommissioningMgr = new StandardCommissioningManager(StdParam);
                break;
            }
            case COMMISSIONING_FLOW_TYPE_USER_INTENT:
            {
                struct UserIntentCommissioningManager::UiCommissioningParam UiParam;
                UiParam.TriggerGpio = param.TypedParam.UserIntent.TriggerGpio;
                mCommissioningMgr = new UserIntentCommissioningManager(UiParam);
                break;
            }
            case COMMISSIONING_FLOW_TYPE_CUSTOM:
            {
                os_printf("\n ((((((((((((((((( 222-A )))))))))))))))\n");
                struct CustomCommissioningManager::CustCommissioningParam CustParam;
                mCommissioningMgr = new CustomCommissioningManager(CustParam);
                os_printf("\n ((((((((((((((((( 222-B )))))))))))))))\n");
                break;
            }
            case COMMISSIONING_FLOW_TYPE_HOST:
            {
                break;
            }
            default:
            {
                mCommissioningMgr = nullptr;
                break;
            }
        }

        return CHIP_NO_ERROR;
    }

    CHIP_ERROR CommissioningInterface::EnableCommissioning()
    {
        AppDelegate* mAppDelegate = &DerivedAppDelegate;
        Server::GetInstance().GetCommissioningWindowManager().SetAppDelegate(mAppDelegate);

        if(mCommissioningMgr == nullptr)
        {
            ChipLogError(DeviceLayer, "Cannot commission, invalid commissioning type");
            return CHIP_ERROR_INVALID_ARGUMENT;
        }

        mCommissioningMgr->StartCommissioningFlow();
        return CHIP_NO_ERROR;
    }

} // namespace DeviceCommissioning
} // namespace talaria
} // namespace CHIP

/*-----------------------------------------------------------*/
