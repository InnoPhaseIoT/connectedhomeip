
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
#define UART_RX_BUFFER_SIZE 1000
#define PRINT_COMMAND_USAGE "\nError: Invalid Command \n\nUsage: \n\t" ON "\n\t" OFF "\n\t" TOGGLE "\n"
#define DEVICE_NOT_COMMISSIONED "Light-switch it not commissioned\n"
#define UART_BAUDRATE 2457600
#define INVALID_COMMAND -1
#define ERR_DEVICE_NOT_COMMISSIONED -2

#include "BindingHandler.h"
#include <platform/CHIPDeviceLayer.h>
#include <app/server/Server.h>
#include <common/Utils.h>

using namespace chip;
using namespace chip::talaria;

void PrepareBindingCommand(uint8_t state);
TaskHandle_t receive_thread = NULL;

static int CommandReceivedFromUart(char * uartbuff)
{
    if (chip::talaria::matterutils::IsNodeCommissioned())
    {
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
        else
        {
            return INVALID_COMMAND;
        }
        os_printf("\nCommand Receievd from UART : %s", uartbuff);
        return 0;
    }
    os_printf("\nDevice is not Commissioned");
    return ERR_DEVICE_NOT_COMMISSIONED;
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