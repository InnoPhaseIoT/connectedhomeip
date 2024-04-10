/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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
#ifdef __cplusplus
extern "C" {
#endif

#include <hio/matter.h>
#include <hio/matter_hio.h>

#ifdef __cplusplus
}
#endif

#include "KeypadInputManager.h"
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/util/config.h>

using namespace chip;
using namespace chip::app::Clusters::KeypadInput;

void KeypadInputManager::HandleSendKey(CommandResponseHelper<SendKeyResponseType> & helper, const CecKeyCodeType & keycCode)
{
    Commands::SendKeyResponse::Type response;
    uint8_t key_code = static_cast<int8_t>(keycCode);
    ChipLogProgress(AppServer, "[KeypadInput] SEND-KEY command received, KeyCode: %d", key_code);

    if (keycCode != CecKeyCodeType::kUnknownEnumValue && keycCode <= CecKeyCodeType::kData)
    {
	    struct keypad_input_set_data input_data;
	    int payload = sizeof(struct keypad_input_set_data);
	    input_data.KeyCode = key_code;

	    int ret = matter_notify(BASIC_VIDEO_PLAYER, KEYPAD_INPUT_SEND_KEY, payload, (struct keypad_input_set_data *) &input_data);
	    if(ret != 0)
	    {
		    response.status = chip::app::Clusters::KeypadInput::KeypadInputStatusEnum::kInvalidKeyInCurrentState;
		    helper.Success(response);
		    ChipLogError(AppServer, "[KeypadInput] SEND-KEY command failed, error: %d", ret);
	    }
	    else
	    {
		    response.status = chip::app::Clusters::KeypadInput::KeypadInputStatusEnum::kSuccess;
		    helper.Success(response);
		    ChipLogProgress(AppServer, "[KeypadInput] SEND-KEY command successfull, KeyCode: %d", key_code);
	    }
    }
    else
    {
	    response.status = chip::app::Clusters::KeypadInput::KeypadInputStatusEnum::kUnsupportedKey;
	    helper.Success(response);
	    ChipLogError(AppServer, "[KeypadInput] SEND-KEY command failed, Unsupported Key.");
    }
}

uint32_t KeypadInputManager::GetFeatureMap(chip::EndpointId endpoint)
{
    if (endpoint >= EMBER_AF_CONTENT_LAUNCHER_CLUSTER_SERVER_ENDPOINT_COUNT)
    {
        return mDynamicEndpointFeatureMap;
    }

    uint32_t featureMap = 0;
    Attributes::FeatureMap::Get(endpoint, &featureMap);
    return featureMap;
}
