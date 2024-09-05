#pragma once

#include <lib/core/CHIPError.h>
#include <common/DeviceCommissioningManager.h>
#include <common/Utils.h>
#include <platform/talaria/TalariaUtils.h>

namespace chip
{
namespace talaria
{
namespace DeviceCommissioning
{
    static bool CommissioningSessionInprogress =  false;
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

    class TalariaDerivedAppDelegate : public AppDelegate
    {
    public :
        ~TalariaDerivedAppDelegate() override {}
        /**
        * This is called when the PBKDFParamRequest is received and indicates the start of the session establishment process
        */
        void OnCommissioningSessionEstablishmentStarted() override {
            CommissioningSessionInprogress = true;
        }

        /**
        * This is called when the commissioning session has been established
        */
        void OnCommissioningSessionStarted() override {}

        /**
        * This is called when the PASE establishment failed (such as, when an invalid passcode is provided) or PASE was established
        * fine but then the fail-safe expired (including being expired by the commissioner)
        *
        * @param err CHIP_ERROR indicating the error that occurred during session establishment or the error accompanying the fail-safe
        * timeout.
        */
        void OnCommissioningSessionEstablishmentError(CHIP_ERROR err) override {
            CommissioningSessionInprogress = false;
        }

        /**
        * This is called when the PASE establishment failed or PASE was established fine but then the fail-safe expired (including
        * being expired by the commissioner) AND the commissioning window is closed. The window may be closed because the commissioning
        * attempts limit was reached or advertising/listening for PASE failed.
        */
        void OnCommissioningSessionStopped() override {
            CommissioningSessionInprogress = false;
        }

        /*
            * This is called any time a basic or enhanced commissioning window is opened.
            *
            * The type of the window can be retrieved by calling
            * CommissioningWindowManager::CommissioningWindowStatusForCluster(), but
            * being careful about how to handle the None status when a window is in
            * fact open.
            */
        void OnCommissioningWindowOpened() override {
            chip::DeviceLayer::Internal::TalariaUtils::DisableWcmPMConfig();
        }

        /*
        * This is called any time a basic or enhanced commissioning window is closed.
        */
        void OnCommissioningWindowClosed() override {
            if (CommissioningSessionInprogress == false)
            {
                chip::DeviceLayer::Internal::TalariaUtils::RestoreWcmPMConfig();
            }
        }
    };
} // namespace DeviceCommissioning
} // namespace talaria
} // namespace CHIP

/*-----------------------------------------------------------*/
