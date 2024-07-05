/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "BindingHandler.h"
#include "app/CommandSender.h"
#include "app/clusters/bindings/BindingManager.h"
#include "app/server/Server.h"
#include "controller/InvokeInteraction.h"
#include "platform/CHIPDeviceLayer.h"
#include <app/clusters/bindings/bindings.h>
#include <lib/support/CodeUtils.h>
#include "LightSwitch_ProjecConfig.h"

#if CONFIG_ENABLE_CHIP_SHELL
#include "lib/shell/Engine.h"
#include "lib/shell/commands/Help.h"
#endif // ENABLE_CHIP_SHELL

void Retry_PrepareBindingCommand(void);
void Retry_PrepareBindingCommand_Level_Control(void);
void Retry_PrepareBindingCommand_Colour_Control(void);
void PrepareBindingCommand_identifyTime(void);

using namespace chip;
using namespace chip::app;
using chip::app::Clusters::LevelControl::LevelControlOptions;
using namespace chip::app::Clusters::ColorControl;


#if CONFIG_ENABLE_CHIP_SHELL
using Shell::Engine;
using Shell::shell_command_t;
using Shell::streamer_get;
using Shell::streamer_printf;

Engine sShellSwitchSubCommands;
Engine sShellSwitchOnOffSubCommands;

Engine sShellSwitchGroupsSubCommands;
Engine sShellSwitchGroupsOnOffSubCommands;

Engine sShellSwitchBindingSubCommands;
#endif // defined(ENABLE_CHIP_SHELL)

namespace {
int retryCountForSendingCommandToLightingApp = 0;


void ProcessOnOffUnicastBindingCommand(CommandId commandId, const EmberBindingTableEntry & binding,
                                       Messaging::ExchangeManager * exchangeMgr, const SessionHandle & sessionHandle)
{
    auto onSuccess = [](const ConcreteCommandPath & commandPath, const StatusIB & status, const auto & dataResponse) {
        retryCountForSendingCommandToLightingApp = 0;
        ChipLogProgress(NotSpecified, "OnOff command succeeds");
    };

    auto onFailure = [](CHIP_ERROR error) {
        if(retryCountForSendingCommandToLightingApp == 0)
        {
            retryCountForSendingCommandToLightingApp++;
            ChipLogError(NotSpecified, "OnOff command failed, Retrying to send Command");
            Retry_PrepareBindingCommand();
        }
        else
        {
            retryCountForSendingCommandToLightingApp = 0;
            ChipLogError(NotSpecified, "OnOff command failed: %" CHIP_ERROR_FORMAT, error.Format());
        }
    };

    switch (commandId)
    {
    case Clusters::OnOff::Commands::Toggle::Id:
        Clusters::OnOff::Commands::Toggle::Type toggleCommand;
        Controller::InvokeCommandRequest(exchangeMgr, sessionHandle, binding.remote, toggleCommand, onSuccess, onFailure);
        break;

    case Clusters::OnOff::Commands::On::Id:
        Clusters::OnOff::Commands::On::Type onCommand;
        Controller::InvokeCommandRequest(exchangeMgr, sessionHandle, binding.remote, onCommand, onSuccess, onFailure);
        break;

    case Clusters::OnOff::Commands::Off::Id:
        Clusters::OnOff::Commands::Off::Type offCommand;
        Controller::InvokeCommandRequest(exchangeMgr, sessionHandle, binding.remote, offCommand, onSuccess, onFailure);
        break;
    }
}

void ProcessOnOffGroupBindingCommand(CommandId commandId, const EmberBindingTableEntry & binding)
{
    Messaging::ExchangeManager & exchangeMgr = Server::GetInstance().GetExchangeManager();

    switch (commandId)
    {
    case Clusters::OnOff::Commands::Toggle::Id:
        Clusters::OnOff::Commands::Toggle::Type toggleCommand;
        Controller::InvokeGroupCommandRequest(&exchangeMgr, binding.fabricIndex, binding.groupId, toggleCommand);
        break;

    case Clusters::OnOff::Commands::On::Id:
        Clusters::OnOff::Commands::On::Type onCommand;
        Controller::InvokeGroupCommandRequest(&exchangeMgr, binding.fabricIndex, binding.groupId, onCommand);

        break;

    case Clusters::OnOff::Commands::Off::Id:
        Clusters::OnOff::Commands::Off::Type offCommand;
        Controller::InvokeGroupCommandRequest(&exchangeMgr, binding.fabricIndex, binding.groupId, offCommand);
        break;
    }
}

#if ENABLE_LEVEL_CONTROL
void ProcessLevelControlUnicastBindingCommand(BindingCommandData * data, const EmberBindingTableEntry & binding,
                                              OperationalDeviceProxy * peer_device)
{
    auto onSuccess = [](const ConcreteCommandPath & commandPath, const StatusIB & status, const auto & dataResponse) {
        retryCountForSendingCommandToLightingApp = 0;
        ChipLogProgress(NotSpecified, "LevelControl command succeeds");
    };

    auto onFailure = [](CHIP_ERROR error) {
        if(retryCountForSendingCommandToLightingApp == 0)
        {
            retryCountForSendingCommandToLightingApp++;
            ChipLogError(NotSpecified, "LevelControl command failed, Retrying to send Command");
            Retry_PrepareBindingCommand_Level_Control();
        }
        else
        {
            retryCountForSendingCommandToLightingApp = 0;
            ChipLogError(NotSpecified, " LevelControl command failed: %" CHIP_ERROR_FORMAT, error.Format());
        }
    };

    Clusters::LevelControl::Commands::MoveToLevel::Type moveToLevelCommand;
    Clusters::LevelControl::Commands::Move::Type moveCommand;
    Clusters::LevelControl::Commands::Step::Type stepCommand;
    Clusters::LevelControl::Commands::Stop::Type stopCommand;
    Clusters::LevelControl::Commands::MoveToLevelWithOnOff::Type moveToLevelWithOnOffCommand;
    Clusters::LevelControl::Commands::MoveWithOnOff::Type moveWithOnOffCommand;
    Clusters::LevelControl::Commands::StepWithOnOff::Type stepWithOnOffCommand;
    Clusters::LevelControl::Commands::StopWithOnOff::Type stopWithOnOffCommand;

    switch (data->commandId)
    {
    case Clusters::LevelControl::Commands::MoveToLevel::Id:
        moveToLevelCommand.level           = static_cast<uint8_t>(data->args[0]);
        moveToLevelCommand.transitionTime  = static_cast<DataModel::Nullable<uint16_t>>(data->args[1]);
        moveToLevelCommand.optionsMask     = static_cast<chip::BitMask<LevelControlOptions>>(data->args[2]);
        moveToLevelCommand.optionsOverride = static_cast<chip::BitMask<LevelControlOptions>>(data->args[3]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         moveToLevelCommand, onSuccess, onFailure);
        break;

    case Clusters::LevelControl::Commands::Move::Id:
        moveCommand.moveMode        = static_cast<EmberAfMoveMode>(data->args[0]);
        moveCommand.rate            = static_cast<DataModel::Nullable<uint8_t>>(data->args[1]);
        moveCommand.optionsMask     = static_cast<chip::BitMask<LevelControlOptions>>(data->args[2]);
        moveCommand.optionsOverride = static_cast<chip::BitMask<LevelControlOptions>>(data->args[3]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         moveCommand, onSuccess, onFailure);
        break;

    case Clusters::LevelControl::Commands::Step::Id:
        stepCommand.stepMode        = static_cast<EmberAfStepMode>(data->args[0]);
        stepCommand.stepSize        = static_cast<uint8_t>(data->args[1]);
        stepCommand.transitionTime  = static_cast<DataModel::Nullable<uint16_t>>(data->args[2]);
        stepCommand.optionsMask     = static_cast<chip::BitMask<LevelControlOptions>>(data->args[3]);
        stepCommand.optionsOverride = static_cast<chip::BitMask<LevelControlOptions>>(data->args[4]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         stepCommand, onSuccess, onFailure);
        break;

    case Clusters::LevelControl::Commands::Stop::Id:
        stopCommand.optionsMask     = static_cast<chip::BitMask<LevelControlOptions>>(data->args[0]);
        stopCommand.optionsOverride = static_cast<chip::BitMask<LevelControlOptions>>(data->args[1]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         stopCommand, onSuccess, onFailure);
        break;

    case Clusters::LevelControl::Commands::MoveToLevelWithOnOff::Id:
        moveToLevelWithOnOffCommand.level           = static_cast<uint8_t>(data->args[0]);
        moveToLevelWithOnOffCommand.transitionTime  = static_cast<DataModel::Nullable<uint16_t>>(data->args[1]);
        moveToLevelWithOnOffCommand.optionsMask     = static_cast<chip::BitMask<LevelControlOptions>>(data->args[2]);
        moveToLevelWithOnOffCommand.optionsOverride = static_cast<chip::BitMask<LevelControlOptions>>(data->args[3]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         moveToLevelWithOnOffCommand, onSuccess, onFailure);
        break;

    case Clusters::LevelControl::Commands::MoveWithOnOff::Id:
        moveWithOnOffCommand.moveMode        = static_cast<EmberAfMoveMode>(data->args[0]);
        moveWithOnOffCommand.rate            = static_cast<DataModel::Nullable<uint8_t>>(data->args[1]);
        moveWithOnOffCommand.optionsMask     = static_cast<chip::BitMask<LevelControlOptions>>(data->args[2]);
        moveWithOnOffCommand.optionsOverride = static_cast<chip::BitMask<LevelControlOptions>>(data->args[3]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         moveWithOnOffCommand, onSuccess, onFailure);
        break;

    case Clusters::LevelControl::Commands::StepWithOnOff::Id:
        stepWithOnOffCommand.stepMode        = static_cast<EmberAfStepMode>(data->args[0]);
        stepWithOnOffCommand.stepSize        = static_cast<uint8_t>(data->args[1]);
        stepWithOnOffCommand.transitionTime  = static_cast<DataModel::Nullable<uint16_t>>(data->args[2]);
        stepWithOnOffCommand.optionsMask     = static_cast<chip::BitMask<LevelControlOptions>>(data->args[3]);
        stepWithOnOffCommand.optionsOverride = static_cast<chip::BitMask<LevelControlOptions>>(data->args[4]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         stepWithOnOffCommand, onSuccess, onFailure);
        break;

    case Clusters::LevelControl::Commands::StopWithOnOff::Id:
        stopWithOnOffCommand.optionsMask     = static_cast<chip::BitMask<LevelControlOptions>>(data->args[0]);
        stopWithOnOffCommand.optionsOverride = static_cast<chip::BitMask<LevelControlOptions>>(data->args[1]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         stopWithOnOffCommand, onSuccess, onFailure);
        break;
    }
}
#endif /* ENABLE_LEVEL_CONTROL */

#if ENABLE_COLOUR_CONTROL
void ProcessColorControlUnicastBindingCommand(BindingCommandData * data, const EmberBindingTableEntry & binding,
                                              OperationalDeviceProxy * peer_device)
{
    auto onSuccess = [](const ConcreteCommandPath & commandPath, const StatusIB & status, const auto & dataResponse) {
        retryCountForSendingCommandToLightingApp = 0;
        ChipLogProgress(NotSpecified, "ColourControl command succeeds");
    };

    auto onFailure = [](CHIP_ERROR error) {
        if(retryCountForSendingCommandToLightingApp == 0)
        {
            retryCountForSendingCommandToLightingApp++;
            ChipLogError(NotSpecified, "ColorControl command failed, Retrying to send Command");
            Retry_PrepareBindingCommand_Colour_Control();
        }
        else
        {
            retryCountForSendingCommandToLightingApp = 0;
            ChipLogError(NotSpecified, " ColorControl command failed: %" CHIP_ERROR_FORMAT, error.Format());
        }
    };

    Clusters::ColorControl::Commands::MoveToHue::Type moveToHueCommand;
    Clusters::ColorControl::Commands::MoveHue::Type moveHueCommand;
    Clusters::ColorControl::Commands::StepHue::Type stepHueCommand;
    Clusters::ColorControl::Commands::MoveToSaturation::Type moveToSaturationCommand;
    Clusters::ColorControl::Commands::MoveSaturation::Type moveSaturationCommand;
    Clusters::ColorControl::Commands::StepSaturation::Type stepSaturationCommand;
    Clusters::ColorControl::Commands::MoveToHueAndSaturation::Type moveToHueAndSaturationCommand;
    Clusters::ColorControl::Commands::MoveToColor::Type moveToColorCommand;
    Clusters::ColorControl::Commands::MoveColor::Type moveColorCommand;
    Clusters::ColorControl::Commands::StepColor::Type stepColorCommand;
    Clusters::ColorControl::Commands::MoveToColorTemperature::Type moveToColorTemperatureCommand;
    Clusters::ColorControl::Commands::EnhancedMoveToHue::Type enhancedMoveToHueCommand;
    Clusters::ColorControl::Commands::EnhancedMoveHue::Type enhancedMoveHueCommand;
    Clusters::ColorControl::Commands::EnhancedStepHue::Type enhancedStepHueCommand;
    Clusters::ColorControl::Commands::EnhancedMoveToHueAndSaturation::Type enhancedMoveToHueAndSaturationCommand;
    Clusters::ColorControl::Commands::ColorLoopSet::Type colorLoopSetCommand;
    Clusters::ColorControl::Commands::StopMoveStep::Type stopMoveStepCommand;
    Clusters::ColorControl::Commands::MoveColorTemperature::Type moveColorTemperatureCommand;
    Clusters::ColorControl::Commands::StepColorTemperature::Type stepColorTemperatureCommand;

    switch (data->commandId)
    {
    case Clusters::ColorControl::Commands::MoveToHue::Id:
        moveToHueCommand.hue             = static_cast<uint8_t>(data->args[0]);
        moveToHueCommand.direction       = static_cast<HueDirection>(data->args[1]);
        moveToHueCommand.transitionTime  = static_cast<uint16_t>(data->args[2]);
        moveToHueCommand.optionsMask     = static_cast<uint8_t>(data->args[3]);
        moveToHueCommand.optionsOverride = static_cast<uint8_t>(data->args[4]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         moveToHueCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::MoveHue::Id:
        moveHueCommand.moveMode        = static_cast<HueMoveMode>(data->args[0]);
        moveHueCommand.rate            = static_cast<uint8_t>(data->args[1]);
        moveHueCommand.optionsMask     = static_cast<uint8_t>(data->args[2]);
        moveHueCommand.optionsOverride = static_cast<uint8_t>(data->args[3]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         moveHueCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::StepHue::Id:
        stepHueCommand.stepMode        = static_cast<HueStepMode>(data->args[0]);
        stepHueCommand.stepSize        = static_cast<uint8_t>(data->args[1]);
        stepHueCommand.transitionTime  = static_cast<uint8_t>(data->args[2]);
        stepHueCommand.optionsMask     = static_cast<uint8_t>(data->args[3]);
        stepHueCommand.optionsOverride = static_cast<uint8_t>(data->args[4]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         stepHueCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::MoveToSaturation::Id:
        moveToSaturationCommand.saturation      = static_cast<uint8_t>(data->args[0]);
        moveToSaturationCommand.transitionTime  = static_cast<uint16_t>(data->args[1]);
        moveToSaturationCommand.optionsMask     = static_cast<uint8_t>(data->args[2]);
        moveToSaturationCommand.optionsOverride = static_cast<uint8_t>(data->args[3]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         moveToSaturationCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::MoveSaturation::Id:
        moveSaturationCommand.moveMode        = static_cast<SaturationMoveMode>(data->args[0]);
        moveSaturationCommand.rate            = static_cast<uint8_t>(data->args[1]);
        moveSaturationCommand.optionsMask     = static_cast<uint8_t>(data->args[2]);
        moveSaturationCommand.optionsOverride = static_cast<uint8_t>(data->args[3]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         moveSaturationCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::StepSaturation::Id:
        stepSaturationCommand.stepMode        = static_cast<SaturationStepMode>(data->args[0]);
        stepSaturationCommand.stepSize        = static_cast<uint8_t>(data->args[1]);
        stepSaturationCommand.transitionTime  = static_cast<uint8_t>(data->args[2]);
        stepSaturationCommand.optionsMask     = static_cast<uint8_t>(data->args[3]);
        stepSaturationCommand.optionsOverride = static_cast<uint8_t>(data->args[4]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         stepSaturationCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::MoveToHueAndSaturation::Id:
        moveToHueAndSaturationCommand.hue             = static_cast<uint8_t>(data->args[0]);
        moveToHueAndSaturationCommand.saturation      = static_cast<uint8_t>(data->args[1]);
        moveToHueAndSaturationCommand.transitionTime  = static_cast<uint16_t>(data->args[2]);
        moveToHueAndSaturationCommand.optionsMask     = static_cast<uint8_t>(data->args[3]);
        moveToHueAndSaturationCommand.optionsOverride = static_cast<uint8_t>(data->args[4]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         moveToHueAndSaturationCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::MoveToColor::Id:
        moveToColorCommand.colorX          = static_cast<uint16_t>(data->args[0]);
        moveToColorCommand.colorY          = static_cast<uint16_t>(data->args[1]);
        moveToColorCommand.transitionTime  = static_cast<uint16_t>(data->args[2]);
        moveToColorCommand.optionsMask     = static_cast<uint8_t>(data->args[3]);
        moveToColorCommand.optionsOverride = static_cast<uint8_t>(data->args[4]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         moveToColorCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::MoveColor::Id:
        moveColorCommand.rateX           = static_cast<uint16_t>(data->args[0]);
        moveColorCommand.rateY           = static_cast<uint16_t>(data->args[1]);
        moveColorCommand.optionsMask     = static_cast<uint8_t>(data->args[2]);
        moveColorCommand.optionsOverride = static_cast<uint8_t>(data->args[3]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         moveColorCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::StepColor::Id:
        stepColorCommand.stepX           = static_cast<uint16_t>(data->args[0]);
        stepColorCommand.stepY           = static_cast<uint16_t>(data->args[1]);
        stepColorCommand.transitionTime  = static_cast<uint16_t>(data->args[2]);
        stepColorCommand.optionsMask     = static_cast<uint8_t>(data->args[3]);
        stepColorCommand.optionsOverride = static_cast<uint8_t>(data->args[4]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         stepColorCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::MoveToColorTemperature::Id:
        moveToColorTemperatureCommand.colorTemperatureMireds = static_cast<uint16_t>(data->args[0]);
        moveToColorTemperatureCommand.transitionTime         = static_cast<uint16_t>(data->args[1]);
        moveToColorTemperatureCommand.optionsMask            = static_cast<uint8_t>(data->args[2]);
        moveToColorTemperatureCommand.optionsOverride        = static_cast<uint8_t>(data->args[3]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         moveToColorTemperatureCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::EnhancedMoveToHue::Id:
        enhancedMoveToHueCommand.enhancedHue     = static_cast<uint16_t>(data->args[0]);
        enhancedMoveToHueCommand.direction       = static_cast<HueDirection>(data->args[1]);
        enhancedMoveToHueCommand.transitionTime  = static_cast<uint16_t>(data->args[2]);
        enhancedMoveToHueCommand.optionsMask     = static_cast<uint8_t>(data->args[3]);
        enhancedMoveToHueCommand.optionsOverride = static_cast<uint8_t>(data->args[4]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         enhancedMoveToHueCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::EnhancedMoveHue::Id:
        enhancedMoveHueCommand.moveMode        = static_cast<HueMoveMode>(data->args[0]);
        enhancedMoveHueCommand.rate            = static_cast<uint16_t>(data->args[1]);
        enhancedMoveHueCommand.optionsMask     = static_cast<uint8_t>(data->args[2]);
        enhancedMoveHueCommand.optionsOverride = static_cast<uint8_t>(data->args[3]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         enhancedMoveHueCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::EnhancedStepHue::Id:
        enhancedStepHueCommand.stepMode        = static_cast<HueStepMode>(data->args[0]);
        enhancedStepHueCommand.stepSize        = static_cast<uint16_t>(data->args[1]);
        enhancedStepHueCommand.transitionTime  = static_cast<uint16_t>(data->args[2]);
        enhancedStepHueCommand.optionsMask     = static_cast<uint8_t>(data->args[3]);
        enhancedStepHueCommand.optionsOverride = static_cast<uint8_t>(data->args[4]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         enhancedStepHueCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::EnhancedMoveToHueAndSaturation::Id:
        enhancedMoveToHueAndSaturationCommand.enhancedHue     = static_cast<uint16_t>(data->args[0]);
        enhancedMoveToHueAndSaturationCommand.saturation      = static_cast<uint8_t>(data->args[1]);
        enhancedMoveToHueAndSaturationCommand.transitionTime  = static_cast<uint16_t>(data->args[2]);
        enhancedMoveToHueAndSaturationCommand.optionsMask     = static_cast<uint8_t>(data->args[3]);
        enhancedMoveToHueAndSaturationCommand.optionsOverride = static_cast<uint8_t>(data->args[4]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         enhancedMoveToHueAndSaturationCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::ColorLoopSet::Id:
        colorLoopSetCommand.updateFlags =
            static_cast<chip::BitMask<chip::app::Clusters::ColorControl::ColorLoopUpdateFlags>>(data->args[0]);
        colorLoopSetCommand.action          = static_cast<ColorLoopAction>(data->args[1]);
        colorLoopSetCommand.direction       = static_cast<ColorLoopDirection>(data->args[2]);
        colorLoopSetCommand.time            = static_cast<uint16_t>(data->args[3]);
        colorLoopSetCommand.startHue        = static_cast<uint16_t>(data->args[4]);
        colorLoopSetCommand.optionsMask     = static_cast<uint8_t>(data->args[5]);
        colorLoopSetCommand.optionsOverride = static_cast<uint8_t>(data->args[6]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         colorLoopSetCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::StopMoveStep::Id:
        stopMoveStepCommand.optionsMask     = static_cast<uint8_t>(data->args[0]);
        stopMoveStepCommand.optionsOverride = static_cast<uint8_t>(data->args[1]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         stopMoveStepCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::MoveColorTemperature::Id:
        moveColorTemperatureCommand.moveMode                      = static_cast<HueMoveMode>(data->args[0]);
        moveColorTemperatureCommand.rate                          = static_cast<uint16_t>(data->args[1]);
        moveColorTemperatureCommand.colorTemperatureMinimumMireds = static_cast<uint16_t>(data->args[2]);
        moveColorTemperatureCommand.colorTemperatureMaximumMireds = static_cast<uint16_t>(data->args[3]);
        moveColorTemperatureCommand.optionsMask                   = static_cast<uint8_t>(data->args[4]);
        moveColorTemperatureCommand.optionsOverride               = static_cast<uint8_t>(data->args[5]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         moveColorTemperatureCommand, onSuccess, onFailure);
        break;

    case Clusters::ColorControl::Commands::StepColorTemperature::Id:
        stepColorTemperatureCommand.stepMode                      = static_cast<HueStepMode>(data->args[0]);
        stepColorTemperatureCommand.stepSize                      = static_cast<uint16_t>(data->args[1]);
        stepColorTemperatureCommand.transitionTime                = static_cast<uint16_t>(data->args[2]);
        stepColorTemperatureCommand.colorTemperatureMinimumMireds = static_cast<uint16_t>(data->args[3]);
        stepColorTemperatureCommand.colorTemperatureMaximumMireds = static_cast<uint16_t>(data->args[4]);
        stepColorTemperatureCommand.optionsMask                   = static_cast<uint8_t>(data->args[5]);
        stepColorTemperatureCommand.optionsOverride               = static_cast<uint8_t>(data->args[6]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         stepColorTemperatureCommand, onSuccess, onFailure);
        break;
    }

}
#endif /* ENABLE_COLOUR_CONTROL */

void ProcessIdentifyUnicastBindingCommand(BindingCommandData * data, const EmberBindingTableEntry & binding,
                                              OperationalDeviceProxy * peer_device)
{
        auto onSuccess = [](const ConcreteCommandPath & commandPath, const StatusIB & status, const auto & dataResponse) {
        retryCountForSendingCommandToLightingApp = 0;
        ChipLogProgress(NotSpecified, "Identify command succeeds");
    };

    auto onFailure = [](CHIP_ERROR error) {
        if(retryCountForSendingCommandToLightingApp == 0)
        {
            retryCountForSendingCommandToLightingApp++;
            ChipLogError(NotSpecified, "Identify command failed, Retrying to send Command");
            PrepareBindingCommand_identifyTime();
        }
        else
        {
            retryCountForSendingCommandToLightingApp = 0;
            ChipLogError(NotSpecified, " Identify command failed: %" CHIP_ERROR_FORMAT, error.Format());
        }
    };

    Clusters::Identify::Commands::Identify::Type identify;
    Clusters::Identify::Commands::TriggerEffect::Type triggerEffect;
    switch (data->commandId)
    {
    case Clusters::Identify::Commands::Identify::Id:
        identify.identifyTime = static_cast<int>(data->args[0]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         identify, onSuccess, onFailure);
        break;
    case Clusters::Identify::Commands::TriggerEffect::Id:
        triggerEffect.effectIdentifier = static_cast<chip::app::Clusters::Identify::EffectIdentifierEnum>(data->args[0]);
        triggerEffect.effectVariant = static_cast<chip::app::Clusters::Identify::EffectVariantEnum>(data->args[1]);
        Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote,
                                         triggerEffect, onSuccess, onFailure);
        break;
    }
}

void LightSwitchChangedHandler(const EmberBindingTableEntry & binding, OperationalDeviceProxy * peer_device, void * context)
{
    VerifyOrReturn(context != nullptr, ChipLogError(NotSpecified, "OnDeviceConnectedFn: context is null"));
    BindingCommandData * data = static_cast<BindingCommandData *>(context);

    if (binding.type == EMBER_MULTICAST_BINDING && data->isGroup)
    {
        switch (data->clusterId)
        {
        case Clusters::OnOff::Id:
            ProcessOnOffGroupBindingCommand(data->commandId, binding);
            break;
        }
    }
    else if (binding.type == EMBER_UNICAST_BINDING && !data->isGroup)
    {
        switch (data->clusterId)
        {
        case Clusters::OnOff::Id:
            VerifyOrDie(peer_device != nullptr && peer_device->ConnectionReady());
            ProcessOnOffUnicastBindingCommand(data->commandId, binding, peer_device->GetExchangeManager(),
                                              peer_device->GetSecureSession().Value());
            break;
#if ENABLE_LEVEL_CONTROL
	case Clusters::LevelControl::Id:
            VerifyOrDie(peer_device != nullptr && peer_device->ConnectionReady());
            ProcessLevelControlUnicastBindingCommand(data, binding, peer_device);
            break;
#endif /* ENABLE_LEVEL_CONTROL */

#if ENABLE_COLOUR_CONTROL
	case Clusters::ColorControl::Id:
            VerifyOrDie(peer_device != nullptr && peer_device->ConnectionReady());
            ProcessColorControlUnicastBindingCommand(data, binding, peer_device);
            break;
#endif /* ENABLE_COLOUR_CONTROL */
        case Clusters::Identify::Id:
            VerifyOrDie(peer_device != nullptr && peer_device->ConnectionReady());
            ProcessIdentifyUnicastBindingCommand(data, binding, peer_device);
        }
    }
}

void LightSwitchContextReleaseHandler(void * context)
{
    VerifyOrReturn(context != nullptr, ChipLogError(NotSpecified, "Invalid context for Light switch context release handler"));

    Platform::Delete(static_cast<BindingCommandData *>(context));
}

void InitBindingHandlerInternal(intptr_t arg)
{
    auto & server = chip::Server::GetInstance();
    chip::BindingManager::GetInstance().Init(
        { &server.GetFabricTable(), server.GetCASESessionManager(), &server.GetPersistentStorage() });
    chip::BindingManager::GetInstance().RegisterBoundDeviceChangedHandler(LightSwitchChangedHandler);
    chip::BindingManager::GetInstance().RegisterBoundDeviceContextReleaseHandler(LightSwitchContextReleaseHandler);
}

#ifdef CONFIG_ENABLE_CHIP_SHELL

/********************************************************
 * Switch shell functions
 *********************************************************/

CHIP_ERROR SwitchHelpHandler(int argc, char ** argv)
{
    sShellSwitchSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR SwitchCommandHandler(int argc, char ** argv)
{
    if (argc == 0)
    {
        return SwitchHelpHandler(argc, argv);
    }

    return sShellSwitchSubCommands.ExecCommand(argc, argv);
}

/********************************************************
 * OnOff switch shell functions
 *********************************************************/

CHIP_ERROR OnOffHelpHandler(int argc, char ** argv)
{
    sShellSwitchOnOffSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR OnOffSwitchCommandHandler(int argc, char ** argv)
{
    if (argc == 0)
    {
        return OnOffHelpHandler(argc, argv);
    }

    return sShellSwitchOnOffSubCommands.ExecCommand(argc, argv);
}

CHIP_ERROR OnSwitchCommandHandler(int argc, char ** argv)
{
    BindingCommandData * data = Platform::New<BindingCommandData>();
    data->commandId           = Clusters::OnOff::Commands::On::Id;
    data->clusterId           = Clusters::OnOff::Id;

    DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, reinterpret_cast<intptr_t>(data));
    return CHIP_NO_ERROR;
}

CHIP_ERROR OffSwitchCommandHandler(int argc, char ** argv)
{
    BindingCommandData * data = Platform::New<BindingCommandData>();
    data->commandId           = Clusters::OnOff::Commands::Off::Id;
    data->clusterId           = Clusters::OnOff::Id;

    DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, reinterpret_cast<intptr_t>(data));
    return CHIP_NO_ERROR;
}

CHIP_ERROR ToggleSwitchCommandHandler(int argc, char ** argv)
{
    BindingCommandData * data = Platform::New<BindingCommandData>();
    data->commandId           = Clusters::OnOff::Commands::Toggle::Id;
    data->clusterId           = Clusters::OnOff::Id;

    DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, reinterpret_cast<intptr_t>(data));
    return CHIP_NO_ERROR;
}

/********************************************************
 * bind switch shell functions
 *********************************************************/

CHIP_ERROR BindingHelpHandler(int argc, char ** argv)
{
    sShellSwitchBindingSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR BindingSwitchCommandHandler(int argc, char ** argv)
{
    if (argc == 0)
    {
        return BindingHelpHandler(argc, argv);
    }

    return sShellSwitchBindingSubCommands.ExecCommand(argc, argv);
}

CHIP_ERROR BindingGroupBindCommandHandler(int argc, char ** argv)
{
    VerifyOrReturnError(argc == 2, CHIP_ERROR_INVALID_ARGUMENT);

    EmberBindingTableEntry * entry = Platform::New<EmberBindingTableEntry>();
    entry->type                    = EMBER_MULTICAST_BINDING;
    entry->fabricIndex             = atoi(argv[0]);
    entry->groupId                 = atoi(argv[1]);
    entry->local                   = 1; // Hardcoded to endpoint 1 for now
    entry->clusterId.SetValue(6);       // Hardcoded to OnOff cluster for now

    DeviceLayer::PlatformMgr().ScheduleWork(BindingWorkerFunction, reinterpret_cast<intptr_t>(entry));
    return CHIP_NO_ERROR;
}

CHIP_ERROR BindingUnicastBindCommandHandler(int argc, char ** argv)
{
    VerifyOrReturnError(argc == 3, CHIP_ERROR_INVALID_ARGUMENT);

    EmberBindingTableEntry * entry = Platform::New<EmberBindingTableEntry>();
    entry->type                    = EMBER_UNICAST_BINDING;
    entry->fabricIndex             = atoi(argv[0]);
    entry->nodeId                  = atoi(argv[1]);
    entry->local                   = 1; // Hardcoded to endpoint 1 for now
    entry->remote                  = atoi(argv[2]);
    entry->clusterId.SetValue(6); // Hardcode to OnOff cluster for now

    DeviceLayer::PlatformMgr().ScheduleWork(BindingWorkerFunction, reinterpret_cast<intptr_t>(entry));
    return CHIP_NO_ERROR;
}

/********************************************************
 * Groups switch shell functions
 *********************************************************/

CHIP_ERROR GroupsHelpHandler(int argc, char ** argv)
{
    sShellSwitchGroupsSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR GroupsSwitchCommandHandler(int argc, char ** argv)
{
    if (argc == 0)
    {
        return GroupsHelpHandler(argc, argv);
    }

    return sShellSwitchGroupsSubCommands.ExecCommand(argc, argv);
}

/********************************************************
 * Groups OnOff switch shell functions
 *********************************************************/

CHIP_ERROR GroupsOnOffHelpHandler(int argc, char ** argv)
{
    sShellSwitchGroupsOnOffSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR GroupsOnOffSwitchCommandHandler(int argc, char ** argv)
{
    if (argc == 0)
    {
        return GroupsOnOffHelpHandler(argc, argv);
    }

    return sShellSwitchGroupsOnOffSubCommands.ExecCommand(argc, argv);
}

CHIP_ERROR GroupOnSwitchCommandHandler(int argc, char ** argv)
{
    BindingCommandData * data = Platform::New<BindingCommandData>();
    data->commandId           = Clusters::OnOff::Commands::On::Id;
    data->clusterId           = Clusters::OnOff::Id;
    data->isGroup             = true;

    DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, reinterpret_cast<intptr_t>(data));
    return CHIP_NO_ERROR;
}

CHIP_ERROR GroupOffSwitchCommandHandler(int argc, char ** argv)
{
    BindingCommandData * data = Platform::New<BindingCommandData>();
    data->commandId           = Clusters::OnOff::Commands::Off::Id;
    data->clusterId           = Clusters::OnOff::Id;
    data->isGroup             = true;

    DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, reinterpret_cast<intptr_t>(data));
    return CHIP_NO_ERROR;
}

CHIP_ERROR GroupToggleSwitchCommandHandler(int argc, char ** argv)
{
    BindingCommandData * data = Platform::New<BindingCommandData>();
    data->commandId           = Clusters::OnOff::Commands::Toggle::Id;
    data->clusterId           = Clusters::OnOff::Id;
    data->isGroup             = true;

    DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, reinterpret_cast<intptr_t>(data));
    return CHIP_NO_ERROR;
}

/**
 * @brief configures switch matter shell
 *
 */
static void RegisterSwitchCommands()
{

    static const shell_command_t sSwitchSubCommands[] = {
        { &SwitchHelpHandler, "help", "Usage: switch <subcommand>" },
        { &OnOffSwitchCommandHandler, "onoff", " Usage: switch onoff <subcommand>" },
        { &GroupsSwitchCommandHandler, "groups", "Usage: switch groups <subcommand>" },
        { &BindingSwitchCommandHandler, "binding", "Usage: switch binding <subcommand>" }
    };

    static const shell_command_t sSwitchOnOffSubCommands[] = {
        { &OnOffHelpHandler, "help", "Usage : switch ononff <subcommand>" },
        { &OnSwitchCommandHandler, "on", "Sends on command to bound lighting app" },
        { &OffSwitchCommandHandler, "off", "Sends off command to bound lighting app" },
        { &ToggleSwitchCommandHandler, "toggle", "Sends toggle command to bound lighting app" }
    };

    static const shell_command_t sSwitchGroupsSubCommands[] = { { &GroupsHelpHandler, "help", "Usage: switch groups <subcommand>" },
                                                                { &GroupsOnOffSwitchCommandHandler, "onoff",
                                                                  "Usage: switch groups onoff <subcommand>" } };

    static const shell_command_t sSwitchGroupsOnOffSubCommands[] = {
        { &GroupsOnOffHelpHandler, "help", "Usage: switch groups onoff <subcommand>" },
        { &GroupOnSwitchCommandHandler, "on", "Sends on command to bound group" },
        { &GroupOffSwitchCommandHandler, "off", "Sends off command to bound group" },
        { &GroupToggleSwitchCommandHandler, "toggle", "Sends toggle command to group" }
    };

    static const shell_command_t sSwitchBindingSubCommands[] = {
        { &BindingHelpHandler, "help", "Usage: switch binding <subcommand>" },
        { &BindingGroupBindCommandHandler, "group", "Usage: switch binding group <fabric index> <group id>" },
        { &BindingUnicastBindCommandHandler, "unicast", "Usage: switch binding unicast <fabric index> <node id> <endpoint>" }
    };

    static const shell_command_t sSwitchCommand = { &SwitchCommandHandler, "switch",
                                                    "Light-switch commands. Usage: switch <subcommand>" };

    sShellSwitchGroupsOnOffSubCommands.RegisterCommands(sSwitchGroupsOnOffSubCommands, ArraySize(sSwitchGroupsOnOffSubCommands));
    sShellSwitchOnOffSubCommands.RegisterCommands(sSwitchOnOffSubCommands, ArraySize(sSwitchOnOffSubCommands));
    sShellSwitchGroupsSubCommands.RegisterCommands(sSwitchGroupsSubCommands, ArraySize(sSwitchGroupsSubCommands));
    sShellSwitchBindingSubCommands.RegisterCommands(sSwitchBindingSubCommands, ArraySize(sSwitchBindingSubCommands));
    sShellSwitchSubCommands.RegisterCommands(sSwitchSubCommands, ArraySize(sSwitchSubCommands));

    Engine::Root().RegisterCommands(&sSwitchCommand, 1);
}
#endif // ENABLE_CHIP_SHELL

} // namespace

/********************************************************
 * Switch functions
 *********************************************************/

void SwitchWorkerFunction(intptr_t context)
{
	
    VerifyOrReturn(context != 0, ChipLogError(NotSpecified, "SwitchWorkerFunction - Invalid work data"));

    BindingCommandData * data = reinterpret_cast<BindingCommandData *>(context);
    BindingManager::GetInstance().NotifyBoundClusterChanged(data->localEndpointId, data->clusterId, static_cast<void *>(data));

}

void BindingWorkerFunction(intptr_t context)
{
    VerifyOrReturn(context != 0, ChipLogError(NotSpecified, "BindingWorkerFunction - Invalid work data"));

    EmberBindingTableEntry * entry = reinterpret_cast<EmberBindingTableEntry *>(context);
    AddBindingEntry(*entry);

    Platform::Delete(entry);
}

CHIP_ERROR InitBindingHandler()
{
    // The initialization of binding manager will try establishing connection with unicast peers
    // so it requires the Server instance to be correctly initialized. Post the init function to
    // the event queue so that everything is ready when initialization is conducted.
    chip::DeviceLayer::PlatformMgr().ScheduleWork(InitBindingHandlerInternal);
#if CONFIG_ENABLE_CHIP_SHELL
    RegisterSwitchCommands();
#endif
    return CHIP_NO_ERROR;
}
