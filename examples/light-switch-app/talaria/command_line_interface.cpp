
#ifdef __cplusplus
extern "C" {
#endif
#include "FreeRTOS.h"
#include "task.h"
#include <kernel/uart.h>

#ifdef __cplusplus
}
#endif

#define OFF "onoff off"
#define ON  "onoff on"
#define TOGGLE "onoff toggle"
#define IDENTIFY "identify"
#define DELIMITERS " "
#define IDENTIFY_END_POINT 1
#define TRIGGER_EFFECT "trigger-effect"
#define UART_RX_BUFFER_SIZE 1000
#define PRINT_COMMAND_USAGE \
    "\nError: Invalid Command \n\nUsage: \n\t" ON "\n\t" OFF "\n\t" TOGGLE "\n\t"  IDENTIFY " "\
    IDENTIFY " <Identify-time-value>\n\t" IDENTIFY " " TRIGGER_EFFECT  " <EffectIdentifier-value> <EffectVariant-value>\n\t"
#define DEVICE_NOT_COMMISSIONED "Light-switch it not commissioned\n"
#define UART_BAUDRATE 2457600
#define INVALID_COMMAND -1
#define ERR_DEVICE_NOT_COMMISSIONED -2

#include "BindingHandler.h"
#include <platform/CHIPDeviceLayer.h>
#include <app/server/Server.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <common/Utils.h>

using namespace chip;
using namespace chip::talaria;

void PrepareBindingCommand(uint8_t state);
TaskHandle_t receive_thread = NULL;
int IdentifyData[2];

void PrepareBindingCommand_identifyTime()
{
    BindingCommandData * data = Platform::New<BindingCommandData>();
    data->clusterId = chip::app::Clusters::Identify::Id;
    data->commandId = chip::app::Clusters::Identify::Commands::Identify::Id;
    data->attributeId = chip::app::Clusters::Identify::Attributes::IdentifyTime::Id;
    /* Command arguments for Identify Cluster:
    * data->args[0] - Identify-Time*/
    data->args[0] = IdentifyData[0];
    SwitchWorkerFunction(data);
}

void Retry_PrepareBindingCommand_identifyTime(void)
{
    PrepareBindingCommand_identifyTime();
}

void PrepareBindingCommand_identify_TriggerEffect()
{
    BindingCommandData * data = Platform::New<BindingCommandData>();
    data->clusterId = chip::app::Clusters::Identify::Id;
    data->commandId = chip::app::Clusters::Identify::Commands::TriggerEffect::Id;
    /* Command arguments for Identify Cluster:
    * data->args[0] - Identify-Time*/
    data->args[0] = IdentifyData[0];
    data->args[1] = IdentifyData[1];
    SwitchWorkerFunction(data);
}

void Retry_PrepareBindingCommand_identify_TriggerEffect(void)
{
    PrepareBindingCommand_identify_TriggerEffect();
}

static int IdentifyCommand(char *uartbuff)
{
    char *token = strtok(uartbuff, DELIMITERS);

    if (token == NULL)
    {
        return INVALID_COMMAND;
    }

    if (strncmp(token, IDENTIFY, strlen(IDENTIFY)) == 0)
    {
        token = strtok(NULL, DELIMITERS);
        if (token == NULL)
        {
            return INVALID_COMMAND;
        }

        IdentifyData[0] = atoi(token);
        chip::DeviceLayer::PlatformMgr().ScheduleWork(PrepareBindingCommand_identifyTime);
        return 0;
    }
    if (strncmp(token, TRIGGER_EFFECT, strlen(TRIGGER_EFFECT)) == 0)
    {
        token = strtok(NULL, DELIMITERS);
        if (token == NULL)
        {
            return INVALID_COMMAND;
        }
        IdentifyData[0] = atoi(token);

        token = strtok(NULL, DELIMITERS);

        if (token == NULL)
        {
            return INVALID_COMMAND;
        }
        IdentifyData[1] = atoi(token);

        chip::DeviceLayer::PlatformMgr().ScheduleWork(PrepareBindingCommand_identify_TriggerEffect);
        return 0;
    }

}

static int CommandReceivedFromUart(char *uartbuff)
{
    int ret = 0;
    if (chip::talaria::matterutils::IsNodeCommissioned() != true)
    {
        os_printf("\nDevice is not Commissioned");
        return ERR_DEVICE_NOT_COMMISSIONED;
    }

    if (strncmp(OFF, uartbuff, strlen(OFF)) == 0 )
    {
        chip::DeviceLayer::PlatformMgr().ScheduleWork(PrepareBindingCommand, 0);
    }
    else if (strncmp(ON, uartbuff, strlen(ON)) == 0)
    {
        chip::DeviceLayer::PlatformMgr().ScheduleWork(PrepareBindingCommand, 1);
    }
    else if (strncmp(TOGGLE, uartbuff, strlen(TOGGLE)) == 0)
    {
        BindingCommandData * data = Platform::New<BindingCommandData>();
        data->clusterId = chip::app::Clusters::OnOff::Id;
        data->commandId = chip::app::Clusters::OnOff::Commands::Toggle::Id;
        chip::DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, data);
    }
    else if (strncmp(IDENTIFY, uartbuff, strlen(IDENTIFY)) == 0)
    {
        ret = IdentifyCommand(uartbuff + strlen(IDENTIFY) + 1); // To Omit "identify " from the command received
        return ret;
    }
    else
    {
        return INVALID_COMMAND;
    }
    os_printf("\nCommand Receievd from UART : %s", uartbuff);
    return 0;
}

static void
uart_rxthread(void *arg)
{
    int ch;
    int ret = 0;
    struct uart *uarthandle;
    char uartbuff[UART_RX_BUFFER_SIZE];
    int index = -1;
    memset(uartbuff, 0, strlen(uartbuff));
    uarthandle = uart_open(UART_BAUDRATE);
    if (uarthandle == NULL)
    {
        os_printf("Error: Failed to Intialize CLI interface\n");
    }
    uart_puts(uarthandle, "\r\n>");

    while (1) {
        ch = uart_getchar(uarthandle);
        switch(ch) {
        case '\n':
            if (index > 0)
            {
                uart_putchar(uarthandle, ch);
                ret = CommandReceivedFromUart(uartbuff);
                if (ret == INVALID_COMMAND)
                {
                    uart_puts(uarthandle, PRINT_COMMAND_USAGE);
                }
                else if (ret == ERR_DEVICE_NOT_COMMISSIONED)
                {
                    uart_puts(uarthandle, DEVICE_NOT_COMMISSIONED);
                }
            }
            uart_puts(uarthandle, "\n>");
            memset(uartbuff, 0, strlen(uartbuff));
            index = -1;
            break;

        case '\b':
            uart_putchar(uarthandle, ch);
            memset(uartbuff + index, 0, 1);
            index--;
            break;

        default:
            uart_putchar(uarthandle, ch);
            index++;
            uartbuff[index] = ch;
            break;
        }
    }
    return;
}

void startCommandLineInterface()
{
    BaseType_t xReturned;

    xReturned = xTaskCreate(uart_rxthread, /* The function that implements the task. */
        "cmdreceive", /* The text name assigned to the task - for debug only as
                       * it is not used by the kernel. */
        4096 / 4, /* The size of the stack to allocate to the task. */
        NULL, /* The parameter passed to the task - not used in this case. */
        (tskIDLE_PRIORITY + 1), /* The priority assigned to the task. */
        &receive_thread);

    if (xReturned != pdPASS) {
        /* Task was not created successfully */
        os_printf("Task creation failed!\n");
    }
}