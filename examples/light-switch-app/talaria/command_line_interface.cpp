
#ifdef __cplusplus
extern "C" {
#endif
#include "FreeRTOS.h"
#include "task.h"
#include <kernel/uart.h>

#ifdef __cplusplus
}
#endif

#define ONOFF "onoff"
#define OFF "off"
#define ON  "on"
#define TOGGLE "toggle"
#define IDENTIFY "identify"
#define DELIMITERS " "
#define IDENTIFY_END_POINT 1
#define TRIGGER_EFFECT "trigger-effect"
#define UART_RX_BUFFER_SIZE 1000
#define PRINT_COMMAND_USAGE \
    "\nError: Invalid Command \n\nUsage:\n\t"\
    "onoff on\n\t" \
    "onoff off\n\t" \
    "onoff toggle\n\t" \
    "identify identify Identify-time\n\t"\
    "identify trigger-effect EffectIdentifier EffectVariant\n\t"\
    "colorcontrol move-hue MoveMode Rate OptionsMask OptionsOverride\n\t"\
    "colorcontrol move-to-hue Hue Direction TransitionTime OptionsMask OptionsOverride\n\t"\
    "colorcontrol step-hue StepMode StepSize TransitionTime OptionsMask OptionsOverride\n\t"\
    "colorcontrol move-saturation MoveMode Rate OptionsMask OptionsOverride\n\t"\
    "colorcontrol move-to-saturation Saturation TransitionTime OptionsMask OptionsOverride\n\t"\
    "colorcontrol step-saturation StepMode StepSize TransitionTime OptionsMask OptionsOverride\n\t"\
    "colorcontrol move-to-hue-and-saturation Hue Saturation TransitionTime OptionsMask OptionsOverride\n\t"\
    "colorcontrol stop-move-step OptionsMask OptionsOverride\n\t"\
    "colorcontrol read current-hue\n\t"\
    "colorcontrol read current-saturation\n\t"

#define DEVICE_NOT_COMMISSIONED "Light-switch it not commissioned\n"
#define UART_BAUDRATE 2457600
#define INVALID_COMMAND -1
#define ERR_DEVICE_NOT_COMMISSIONED -2
#define COLORCONTROL "colorcontrol"
#define MOVE_HUE "move-hue"
#define MOVE_TO_HUE "move-to-hue"
#define STEP_HUE "step-hue"
#define MOVE_SATURATION "move-saturation"
#define MOVE_TO_SATURATION "move-to-saturation"
#define STEP_SATURATION "step-saturation"
#define MOVE_TO_HUE_AND_SATURATION "move-to-hue-and-saturation"
#define STOP_MOVE_STEP "stop-move-step"
#define READ "read"
#define CURRENT_HUE "current-hue"
#define CURRENT_SATURATION "current-saturation"
#define IDENTIFY_TIME_ARGUMENT_COUNT 1
#define IDENTIFY_TRIGGER_EFFECT_ARGUMENT_COUNT 2
#define MOVE_HUE_ARGUMENT_COUNT 4
#define MOVE_TO_HUE_ARGUMENT_COUNT 5
#define STEP_HUE_ARGUMENT_COUNT 5
#define MOVE_SATURATION_ARGUMENT_COUNT 4
#define MOVE_TO_SATURATION_ARGUMENT_COUNT 4
#define STEP_SATURATION_ARGUMENT_COUNT 5
#define MOVE_TO_HUE_AND_SATURATION_ARGUMENT_COUNT 5
#define STOP_MOVE_STEP_ARGUMENT_COUNT 2
#define ZERO_ARGUMENT_COUNT 0

#include "BindingHandler.h"
#include <platform/CHIPDeviceLayer.h>
#include <app/server/Server.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <common/Utils.h>

using namespace chip;
using namespace chip::talaria;

enum ReadCommandsEnum
{
    Hue=1,
    Saturation,
    MaxReadCommand
};

enum OnOffCommandsEnum
{
    On=1,
    Off,
    Toggle,
    MaxOnOffCommand
};

enum IdentifyCommandsEnum
{
    IdentifyCommandTime=1,
    TriggerEffectCommand,
    MaxIdentifyCommand
};

enum ColorControlCommandsEnum
{
    MoveHue=1,
    MoveToHue,
    StepHue,
    MoveSaturation,
    MoveToSaturation,
    StepSaturation,
    MoveToHueAndSaturation,
    StopMoveStep,
    Read,
    MaxAttribuet
};

enum ClusterNameEnum
{
    OnOff=1,
    Identify,
    ColorControl,
    MAxCluster
};

static TaskHandle_t receive_thread = NULL;

static int GetTotalArguemntFromCommand(int * data, int len, char *token)
{
    if (len == 0)
        return 0;

    int index = 0;
    token = strtok(NULL, DELIMITERS);

    for (index = 0; index < len; index++)
    {
        if (token != NULL)
        {
            data[index] = atoi(token);
            token = strtok(NULL, DELIMITERS);
        }
        else
        {
            return INVALID_COMMAND;
        }
    }

    return 0;
}

static void SendReadCommand(uint32_t clusterId, uint32_t attributeId)
{
    BindingCommandData * data = Platform::New<BindingCommandData>();
    data->clusterId = clusterId;
    data->attributeId = attributeId;
    data->commandId = chip::kInvalidCommandId; /* Since there is no command ID for Read */
    chip::DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, data);
}

static enum ReadCommandsEnum GetReadAttribute(char * token)
{
    if (strncmp(token, CURRENT_HUE, strlen(CURRENT_HUE)) == 0)
    {
        return Hue;
    }
    else if (strncmp(token, CURRENT_SATURATION, strlen(CURRENT_SATURATION)) == 0)
    {
        return Saturation;
    }
}

static int ReadCommand(char * token, uint32_t clusterId)
{
    int ret = INVALID_COMMAND;

    token = strtok(NULL, DELIMITERS);

    if (token == NULL)
        return INVALID_COMMAND;

    enum ReadCommandsEnum value = GetReadAttribute(token);

    switch(value)
    {
        case Hue:
            SendReadCommand(clusterId, chip::app::Clusters::ColorControl::Attributes::CurrentHue::Id);
            ret = 0;
            break;

        case Saturation:
            SendReadCommand(clusterId, chip::app::Clusters::ColorControl::Attributes::CurrentSaturation::Id);
            ret = 0;
            break;
    }

    return ret;
}

static int SendClusterCommandFromBinder(char *token, int len, uint32_t clusterId, uint32_t commandId)
{
    int ret = INVALID_COMMAND;
    int dataArgument[len];

    ret = GetTotalArguemntFromCommand(dataArgument, len, token);
    if (ret == INVALID_COMMAND)
        return ret;

    BindingCommandData * data = Platform::New<BindingCommandData>();
    data->clusterId = clusterId;
    data->commandId = commandId;
    /* Command arguments of command of colorcontrol cluster Cluster */
    for (int index = 0; index < len; index++)
    {
        data->args[index] = dataArgument[index];
    }
    chip::DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, data);

    return 0;
}

static enum ColorControlCommandsEnum GetColorControlCommand(char *token)
{
    if (strncmp(token, MOVE_TO_HUE_AND_SATURATION, strlen(MOVE_TO_HUE_AND_SATURATION)) == 0)
    {
        return MoveToHueAndSaturation;
    }
    if (strncmp(token, MOVE_HUE, strlen(MOVE_HUE)) == 0)
    {
        return MoveHue;
    }
    if (strncmp(token, MOVE_TO_HUE, strlen(MOVE_TO_HUE)) == 0)
    {
        return MoveToHue;
    }
    if (strncmp(token, STEP_HUE, strlen(STEP_HUE)) == 0)
    {
        return StepHue;
    }
    if (strncmp(token, MOVE_SATURATION, strlen(MOVE_SATURATION)) == 0)
    {
        return MoveSaturation;
    }
    if (strncmp(token, MOVE_TO_SATURATION, strlen(MOVE_TO_SATURATION)) == 0)
    {
        return MoveToSaturation;
    }
    if (strncmp(token, STEP_SATURATION, strlen(STEP_SATURATION)) == 0)
    {
        return StepSaturation;
    }
    if (strncmp(token, STOP_MOVE_STEP, strlen(STOP_MOVE_STEP)) == 0)
    {
        return StopMoveStep;
    }
    if (strncmp(token, READ, strlen(READ)) == 0)
    {
        return Read;
    }
}

static int ColorControlCommand(char *uartbuff)
{
    int ret = INVALID_COMMAND;
    uint32_t clusterId = chip::app::Clusters::ColorControl::Id;

    char *token = strtok(uartbuff, DELIMITERS);

    if (token == NULL)
    {
        return INVALID_COMMAND;
    }
    enum ColorControlCommandsEnum value = GetColorControlCommand(token);

    switch(value)
    {
        case MoveHue:
            ret = SendClusterCommandFromBinder(token, MOVE_HUE_ARGUMENT_COUNT, clusterId,
                    chip::app::Clusters::ColorControl::Commands::MoveHue::Id);
            break;

        case MoveToHue:
            ret = SendClusterCommandFromBinder(token, MOVE_TO_HUE_ARGUMENT_COUNT, clusterId,
                    chip::app::Clusters::ColorControl::Commands::MoveToHue::Id);
            break;

        case StepHue:
            ret = SendClusterCommandFromBinder(token, STEP_HUE_ARGUMENT_COUNT, clusterId,
                    chip::app::Clusters::ColorControl::Commands::StepHue::Id);
            break;

        case MoveSaturation:
            ret = SendClusterCommandFromBinder(token, MOVE_SATURATION_ARGUMENT_COUNT, clusterId,
                    chip::app::Clusters::ColorControl::Commands::MoveSaturation::Id);
            break;

        case MoveToSaturation:
            ret = SendClusterCommandFromBinder(token, MOVE_TO_SATURATION_ARGUMENT_COUNT, clusterId,
                    chip::app::Clusters::ColorControl::Commands::MoveToSaturation::Id);
            break;

        case StepSaturation:
            ret = SendClusterCommandFromBinder(token, STEP_SATURATION_ARGUMENT_COUNT, clusterId,
                    chip::app::Clusters::ColorControl::Commands::StepSaturation::Id);
            break;

        case MoveToHueAndSaturation:
            ret = SendClusterCommandFromBinder(token, MOVE_TO_HUE_AND_SATURATION_ARGUMENT_COUNT, clusterId,
                    chip::app::Clusters::ColorControl::Commands::MoveToHueAndSaturation::Id);
            break;

        case StopMoveStep:
            ret = SendClusterCommandFromBinder(token, STOP_MOVE_STEP_ARGUMENT_COUNT, clusterId,
                    chip::app::Clusters::ColorControl::Commands::StopMoveStep::Id);
            break;

        case Read:
            ret = ReadCommand(token, clusterId);
            break;
    }

    return ret;
}

static enum IdentifyCommandsEnum GetIdentifyCommands(char *token)
{
    if (strncmp(token, IDENTIFY, strlen(IDENTIFY)) == 0)
    {
        return IdentifyCommandTime;
    }
    if (strncmp(token, TRIGGER_EFFECT, strlen(TRIGGER_EFFECT)) == 0)
    {
        return TriggerEffectCommand;
    }
}

static int IdentifyCommand(char *uartbuff)
{
    int ret = INVALID_COMMAND;
    uint32_t clusterId = chip::app::Clusters::Identify::Id;

    char *token = strtok(uartbuff, DELIMITERS);

    if (token == NULL)
    {
        return INVALID_COMMAND;
    }
    enum IdentifyCommandsEnum value = GetIdentifyCommands(token);

    switch(value)
    {
        case IdentifyCommandTime:
            ret = SendClusterCommandFromBinder(token, IDENTIFY_TIME_ARGUMENT_COUNT, clusterId,
                    chip::app::Clusters::Identify::Commands::Identify::Id);
            break;

        case TriggerEffectCommand:
            ret = SendClusterCommandFromBinder(token, IDENTIFY_TRIGGER_EFFECT_ARGUMENT_COUNT, clusterId,
                    chip::app::Clusters::Identify::Commands::TriggerEffect::Id);
            break;
    }

    return ret;
}

static enum OnOffCommandsEnum GetOnOffCommands(char *token)
{
    if (strncmp(token, ON, strlen(ON)) == 0)
    {
        return On;
    }
    if (strncmp(token, OFF, strlen(OFF)) == 0)
    {
        return Off;
    }
    if (strncmp(token, TOGGLE, strlen(TOGGLE)) == 0)
    {
        return Toggle;
    }
}

static int OnOffCommand(char *uartbuff)
{
    int ret = INVALID_COMMAND;
    uint32_t clusterId = chip::app::Clusters::OnOff::Id;

    char *token = strtok(uartbuff, DELIMITERS);

    if (token == NULL)
    {
        return INVALID_COMMAND;
    }
    enum OnOffCommandsEnum value = GetOnOffCommands(token);

    switch(value)
    {
        case On:
            ret = SendClusterCommandFromBinder(token, ZERO_ARGUMENT_COUNT, clusterId,
                    chip::app::Clusters::OnOff::Commands::On::Id);
            ret = 0;
            break;

        case Off:
            ret = SendClusterCommandFromBinder(token, ZERO_ARGUMENT_COUNT, clusterId,
                    chip::app::Clusters::OnOff::Commands::Off::Id);
            ret = 0;
            break;

        case Toggle:
            ret = SendClusterCommandFromBinder(token, ZERO_ARGUMENT_COUNT, clusterId,
                    chip::app::Clusters::OnOff::Commands::Toggle::Id);
            break;
    }

    return ret;
}

static enum ClusterNameEnum GetClusterName(char *uartbuff)
{
    if (strncmp(ONOFF, uartbuff, strlen(ONOFF)) == 0 )
    {
        return OnOff;
    }
    else if (strncmp(IDENTIFY, uartbuff, strlen(IDENTIFY)) == 0)
    {
        return Identify;
    }
    else if (strncmp(COLORCONTROL, uartbuff, strlen(COLORCONTROL)) == 0)
    {
        return ColorControl;
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

    enum ClusterNameEnum value = GetClusterName(uartbuff);

    switch(value)
    {
        case OnOff:
            ret = OnOffCommand(uartbuff + strlen(ONOFF) + 1); // To Omit "Onoff  " from the command received
            break;

        case Identify:
            ret = IdentifyCommand(uartbuff + strlen(IDENTIFY) + 1); // To Omit "identify " from the command received
            break;

        case ColorControl:
            ret = ColorControlCommand(uartbuff + strlen(COLORCONTROL) + 1); // To Omit "colorcontrol " from the command received
            break;
    }

    return ret;

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