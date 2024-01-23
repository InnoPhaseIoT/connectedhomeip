
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

#include <common/DeviceCommissioningManager.h>
#include <common/Utils.h>

namespace chip
{
namespace talaria
{
namespace DeviceCommissioning
{
    struct callout CommissioningSetupButton::mCallout;
    uint32_t CommissioningSetupButton::mTimeout = 3000;
    uint8_t CommissioningSetupButton::mTriggerGpio;
    SemaphoreHandle_t CommissioningSetupButton::mSem = nullptr;
    TaskHandle_t UserIntentCommissioningManager::mCommissioningTask = nullptr;
    CommissioningInput* UserIntentCommissioningManager::mCommissioningInput = nullptr;
    bool UserIntentCommissioningGpioTwoSecondHolded = false;

    /* ------------------------------------ CommissioningManager ------------------------------------------------------------------------- */

    CommissioningManager::CommissioningManager()
    {

    }

    CommissioningManager::~CommissioningManager()
    {

    }

    void CommissioningManager::GetCommissioningFlowType()
    {
        os_printf("\nCOMMISSIONING_FLOW_TYPE = %d ", static_cast<int>(type));
    }

    void CommissioningManager::EnableCommissioning()
    {
        chip::DeviceLayer::PlatformMgr().ScheduleBackgroundWork(OpenCommissioningWindow, reinterpret_cast<intptr_t>(nullptr));
    }

    void CommissioningManager::OpenCommissioningWindow(intptr_t arg)
    {
        chip::Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow();
    }

    // bool CommissioningManager::IsPrimaryCommissioningDone()
    // {
    //     if(Server::GetInstance().GetFabricTable().FabricCount() != 0)
    //     {
    //         return true;
    //     }

    //     return false;
    // }

    /* ----------------------------------- StandardCommissioning  ------------------------------------------------------------------------ */

    StandardCommissioningManager::StandardCommissioningManager(struct StdCommissioningParam &param)
    {
        type = matterutils::COMMISSIONING_FLOW_TYPE_STANDARD;
    }

    StandardCommissioningManager::~StandardCommissioningManager()
    {

    }

    CHIP_ERROR StandardCommissioningManager::StartCommissioningFlow(void)
    {
        if(matterutils::IsNodeCommissioned() == false)
        {
            CommissioningManager::EnableCommissioning();
        }
        return CHIP_NO_ERROR;
    }

    /* ---------------------------------- UserIntentCommissioning ------------------------------------------------------------------------ */

    UserIntentCommissioningManager::UserIntentCommissioningManager(struct UiCommissioningParam &param)
    {
        // uint16_t value;

        type = matterutils::COMMISSIONING_FLOW_TYPE_USER_INTENT;

        // if(DeviceLayer::ConfigurationMgr().GetInitialPairingHint(&value) == )
        // {
                struct CommissioningSetupButton::SetupButtonParam SetupParam;
                SetupParam.TriggerGpio = param.TriggerGpio;
                mCommissioningInput = new CommissioningSetupButton(SetupParam);
        // }

        xTaskCreate( CommissionigTask, "commissioning_task", configMINIMAL_STACK_SIZE, NULL, 5, &mCommissioningTask);
    }

    UserIntentCommissioningManager::~UserIntentCommissioningManager()
    {

    }

    CHIP_ERROR UserIntentCommissioningManager::StartCommissioningFlow(void)
    {
        if(mCommissioningInput == nullptr)
        {
            ChipLogError(DeviceLayer, "Cannot perform user intent commission, invalid setup button configuration");
            return CHIP_ERROR_INVALID_ARGUMENT; // to do: return corect error code
        }
        if(matterutils::IsNodeCommissioned() == false && chip::Server::GetInstance().GetCommissioningWindowManager().IsCommissioningWindowOpen() == 0)
        {
            mCommissioningInput->Enable();
        }
        return CHIP_NO_ERROR;
    }

    void UserIntentCommissioningManager::CommissionigTask(void * arg)
    {
        uint32_t retry_cout = 0;

        while(1)
        {
            vTaskDelay(300);
            if(chip::Server::GetInstance().GetCommissioningWindowManager().IsCommissioningWindowOpen() == 0 && matterutils::IsNodeCommissioned() == false )
            {
                mCommissioningInput->TriggerWait();
            }
            if(chip::Server::GetInstance().GetCommissioningWindowManager().IsCommissioningWindowOpen() == 1)
             {
                /* Commissioning window has alredy been opened */
                /* A small delay before enabling the setup button, to avoid continous loop in the worst case */
                vTaskDelay(300);
                continue;
            }
            CommissioningManager::EnableCommissioning();

            /* Delay before enabling the setup button again */
        }
    }

    /* ---------------------------------- CustomCommissioning ---------------------------------------------------------------------------- */

    CustomCommissioningManager::CustomCommissioningManager(struct CustCommissioningParam &param)
    {
        type = matterutils::COMMISSIONING_FLOW_TYPE_CUSTOM;
        os_printf("\n******************** 4 CC");
    }

    CustomCommissioningManager::~CustomCommissioningManager()
    {
        os_printf("\n******************** 4~ CC");
    }

    // CHIP_ERROR CustomCommissioningManager::init(CommissioningParam *param = nullptr)
    // {
    //     type = COMMISSIONING_FLOW_TYPE_CUSTOM;
    // }

    CHIP_ERROR CustomCommissioningManager::StartCommissioningFlow(void)
    {
        return CHIP_NO_ERROR;
    }
    /* ---------------------------------- -------------------- ---------------------------------------------------------------------------- */


    /* ---------------------------------- CommissioningSetupButton ------------------------------------------------------------------------ */

    CommissioningSetupButton::CommissioningSetupButton(struct SetupButtonParam &param) : CommissioningInput(param.Param)
    {
        mTriggerGpio = param.TriggerGpio;
        os_gpio_request(GPIO_PIN(mTriggerGpio));
        os_gpio_set_input(GPIO_PIN(mTriggerGpio));
        os_gpio_set_pull(GPIO_PIN(mTriggerGpio));
        os_gpio_set_irq_level_low(GPIO_PIN(mTriggerGpio));
        os_gpio_attach_event(gpio_event_3, CommissioningSetupButton::Irq, NULL);
        callout_init(&mCallout, CommissioningSetupButton::Event);
        mSem = xSemaphoreCreateCounting(1,0);
    }

    CommissioningSetupButton::~CommissioningSetupButton()
    {

    }

    void CommissioningSetupButton::Event(struct callout *co)
    {
        UserIntentCommissioningGpioTwoSecondHolded = true;
    }

    void CommissioningSetupButton::Irq(void *c)
    {
        // static int start_time;
        os_gpio_disable_irq(GPIO_PIN(mTriggerGpio));
        os_clear_event(EVENT_GPIO_3);
        if(os_gpio_get_value(GPIO_PIN(mTriggerGpio)) == 0)
        {
            // System::Clock::Timeout maxTimeout = System::Clock::Seconds32(5000);
            // System::Clock::Timeout maxTimeout = static_cast<System::Clock::Timeout>(30);
            // DeviceLayer::SystemLayer().StartTimer(maxTimeout, setup_button_tigger, nullptr);
            os_gpio_set_irq_level_high(GPIO_PIN(mTriggerGpio));
            os_gpio_enable_irq(GPIO_PIN(mTriggerGpio), 3);
            callout_stop(&mCallout);
            callout_schedule(&mCallout, SYSTIME_MS(mTimeout));
        }
        else
        {
            os_gpio_set_irq_level_low(GPIO_PIN(mTriggerGpio));
            os_gpio_enable_irq(GPIO_PIN(mTriggerGpio), 3);
            if(UserIntentCommissioningGpioTwoSecondHolded == true)
            {
                xSemaphoreGiveFromISR(mSem, pdTRUE);
                UserIntentCommissioningGpioTwoSecondHolded = false;
            }

            callout_stop(&mCallout);
        }
    }

    void CommissioningSetupButton::Enable()
    {
        os_gpio_enable_irq(GPIO_PIN(mTriggerGpio), gpio_event_3);
    }

    void CommissioningSetupButton::Disable()
    {
        os_gpio_disable_irq(GPIO_PIN(mTriggerGpio));
        os_gpio_detach_event(GPIO_PIN(mTriggerGpio), CommissioningSetupButton::Irq);
        os_gpio_free(GPIO_PIN(mTriggerGpio));

    }

    void CommissioningSetupButton::TriggerWait()
    {
        uint16_t timeout;
        char pairingInst[chip::Dnssd::kKeyPairingInstructionMaxLength + 1];

        if( matterutils::NOT_COMMISSIONED == matterutils::GetNodeCommissionedStatus() )
        {
            DeviceLayer::ConfigurationMgr().GetInitialPairingInstruction(pairingInst, sizeof(pairingInst));
        }
        else
        {
            DeviceLayer::ConfigurationMgr().GetSecondaryPairingInstruction(pairingInst, sizeof(pairingInst));
        }

        timeout = atoi(pairingInst);
        if( timeout > 20 )
        {
            timeout = 5; // might be a wrong instruction, set to default
        }
        mTimeout = timeout * 1000;

        Enable();
        xSemaphoreTake(mSem, portMAX_DELAY);
        Disable();
    }

} // namespace DeviceCommissioning
} // namespace talaria
} // namespace CHIP

/*-----------------------------------------------------------*/
