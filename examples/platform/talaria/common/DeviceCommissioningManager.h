#pragma once

#include <lib/core/CHIPError.h>
#include <app/server/Server.h>
#include <common/Utils.h>

// using namespace chip::talaria::matterutils;

namespace chip
{
namespace talaria
{
namespace DeviceCommissioning
{
    class CommissioningInput
    {
    public:
        struct InputParam
        {

        };
        CommissioningInput(struct InputParam &param){};
        virtual ~CommissioningInput(){};

        virtual void Enable() = 0;
        virtual void Disable() = 0;
        virtual void TriggerWait() = 0;

    private:
        CommissioningInput() = delete;
    protected:

    };

    class CommissioningSetupButton : public CommissioningInput
    {
    public:
        struct SetupButtonParam
        {
            struct InputParam Param;
            uint8_t TriggerGpio;
        };
        CommissioningSetupButton(struct SetupButtonParam &param);
        ~CommissioningSetupButton();

        void Enable();
        void Disable();
        void TriggerWait();

    private:
        CommissioningSetupButton() = delete;
        static SemaphoreHandle_t mSem;
        static struct callout mCallout;
        static uint32_t mTimeout;
        static uint8_t mTriggerGpio;

    protected:
        static void Event(struct callout *co);
        static void Irq(void *c);

    };

    class CommissioningManager
    {
    public:
        struct CommissioningParam
        {
            matterutils::CommissioningFlowType type = matterutils::COMMISSIONING_FLOW_TYPE_INVALID;
        };
    
        CommissioningManager();
        virtual ~CommissioningManager();

        virtual CHIP_ERROR StartCommissioningFlow(void) = 0;
        void GetCommissioningFlowType();
        static bool IsPrimaryCommissioningDone();

    private:
        static void OpenCommissioningWindow(intptr_t arg);

    protected:
        chip::talaria::matterutils::CommissioningFlowType type;
        static void EnableCommissioning();
    };

    class StandardCommissioningManager : public CommissioningManager
    {
    public:
        struct StdCommissioningParam
        {

        };
        StandardCommissioningManager(struct StdCommissioningParam &param);
        ~StandardCommissioningManager();
        CHIP_ERROR StartCommissioningFlow(void);

    private:
        StandardCommissioningManager() = delete;

    protected:

    };

    class UserIntentCommissioningManager : public CommissioningManager
    {
    public:
        struct UiCommissioningParam
        {
            uint8_t TriggerGpio;
        };
        UserIntentCommissioningManager(struct UiCommissioningParam &param);
        ~UserIntentCommissioningManager();
        CHIP_ERROR StartCommissioningFlow(void);

    private:
        UserIntentCommissioningManager() = delete;
        static void CommissionigTask(void * arg);
        static CommissioningInput* mCommissioningInput;
        static TaskHandle_t mCommissioningTask;

    protected:

    };

    class CustomCommissioningManager : public CommissioningManager
    {
    public:
        struct CustCommissioningParam
        {

        };
        CustomCommissioningManager(struct CustCommissioningParam &param);
        ~CustomCommissioningManager();
        CHIP_ERROR StartCommissioningFlow(void);

    private:
        CustomCommissioningManager() = delete;

    protected:

    };

} // namespace DeviceCommissioning
} // namespace talaria
} // namespace CHIP

/*-----------------------------------------------------------*/
