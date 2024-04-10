/**
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

#include "MediaPlaybackManager.h"
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/util/config.h>

using namespace std;
using namespace chip::app::DataModel;
using namespace chip::app::Clusters::MediaPlayback;
using namespace chip::Uint8;
using chip::CharSpan;

PlaybackStateEnum MediaPlaybackManager::HandleGetCurrentState()
{
    ChipLogProgress(AppServer, "[MediaPlayback] CurrentState: %d", mCurrentState);
    return mCurrentState;
}

uint64_t MediaPlaybackManager::HandleGetStartTime()
{
    ChipLogProgress(AppServer, "[MediaPlayback] StartTime: %llu", mStartTime);
    return mStartTime;
}

uint64_t MediaPlaybackManager::HandleGetDuration()
{
    ChipLogProgress(AppServer, "[MediaPlayback] Duration: %llu", mDuration);
    return mDuration;
}

CHIP_ERROR MediaPlaybackManager::HandleGetSampledPosition(AttributeValueEncoder & aEncoder)
{
    ChipLogProgress(AppServer, "[MediaPlayback] SampledPosition: %llu", mPlaybackPosition);
    return aEncoder.Encode(mPlaybackPosition);
}

float MediaPlaybackManager::HandleGetPlaybackSpeed()
{
    ChipLogProgress(AppServer, "[MediaPlayback] PlaybackSpeed: %f", mPlaybackSpeed);
    return mPlaybackSpeed;
}

uint64_t MediaPlaybackManager::HandleGetSeekRangeStart()
{
    ChipLogProgress(AppServer, "[MediaPlayback] SeekRangeStart: %llu", mStartTime);
    return mStartTime;
}

uint64_t MediaPlaybackManager::HandleGetSeekRangeEnd()
{
    ChipLogProgress(AppServer, "[MediaPlayback] SeekRangeEnd: %llu", mDuration);
    return mDuration;
}

void MediaPlaybackManager::HandlePlay(CommandResponseHelper<Commands::PlaybackResponse::Type> & helper)
{
    ChipLogProgress(AppServer, "[MediaPlayback] PLAY command received");

    struct media_playback_set_data input_data;
    int payload = sizeof(struct media_playback_set_data);

    int ret = matter_notify(BASIC_VIDEO_PLAYER, MEDIA_PLAYBACK_PLAY, payload, (struct media_playback_set_data *) &input_data);
    if(ret != 0)
    {
	    Commands::PlaybackResponse::Type response;
	    response.data   = chip::MakeOptional(CharSpan::fromCharString("Play Command Failed"));
	    response.status = MediaPlaybackStatusEnum::kInvalidStateForCommand;
	    helper.Success(response);
	    ChipLogError(AppServer, "[MediaPlayback] PLAY command failed, error: %d", ret);
    }
    else
    {
	    mCurrentState  = PlaybackStateEnum::kPlaying;
	    mPlaybackSpeed = 1;

	    Commands::PlaybackResponse::Type response;
	    response.data   = chip::MakeOptional(CharSpan::fromCharString("Play Command Successfull"));
	    response.status = MediaPlaybackStatusEnum::kSuccess;
	    helper.Success(response);
	    ChipLogProgress(AppServer, "[MediaPlayback] PLAY command successfull");
    }
}

void MediaPlaybackManager::HandlePause(CommandResponseHelper<Commands::PlaybackResponse::Type> & helper)
{
    ChipLogProgress(AppServer, "[MediaPlayback] PAUSE command received");

    struct media_playback_set_data input_data;
    int payload = sizeof(struct media_playback_set_data);

    int ret = matter_notify(BASIC_VIDEO_PLAYER, MEDIA_PLAYBACK_PAUSE, payload, (struct media_playback_set_data *) &input_data);
    if(ret != 0)
    {
            Commands::PlaybackResponse::Type response;
            response.data   = chip::MakeOptional(CharSpan::fromCharString("Pause Command Failed"));
            response.status = MediaPlaybackStatusEnum::kInvalidStateForCommand;
            helper.Success(response);
            ChipLogError(AppServer, "[MediaPlayback] PAUSE command failed, error: %d", ret);
    }
    else
    {
            mCurrentState  = PlaybackStateEnum::kPaused;
            mPlaybackSpeed = 0;

            Commands::PlaybackResponse::Type response;
            response.data   = chip::MakeOptional(CharSpan::fromCharString("Pause Command Successfull"));
            response.status = MediaPlaybackStatusEnum::kSuccess;
            helper.Success(response);
            ChipLogProgress(AppServer, "[MediaPlayback] PAUSE command successfull");
    }

}

void MediaPlaybackManager::HandleStop(CommandResponseHelper<Commands::PlaybackResponse::Type> & helper)
{
    ChipLogProgress(AppServer, "[MediaPlayback] STOP command received");

    struct media_playback_set_data input_data;
    int payload = sizeof(struct media_playback_set_data);

    int ret = matter_notify(BASIC_VIDEO_PLAYER, MEDIA_PLAYBACK_STOP, payload, (struct media_playback_set_data *) &input_data);
    if(ret != 0)
    {
            Commands::PlaybackResponse::Type response;
            response.data   = chip::MakeOptional(CharSpan::fromCharString("Stop Command Failed"));
            response.status = MediaPlaybackStatusEnum::kInvalidStateForCommand;
            helper.Success(response);
            ChipLogError(AppServer, "[MediaPlayback] STOP command failed, error: %d", ret);
    }
    else
    {
            mCurrentState  = PlaybackStateEnum::kNotPlaying;
            mPlaybackSpeed = 0;
	    mPlaybackPosition = { 0, chip::app::DataModel::Nullable<uint64_t>(0) };

            Commands::PlaybackResponse::Type response;
            response.data   = chip::MakeOptional(CharSpan::fromCharString("Stop Command Successfull"));
            response.status = MediaPlaybackStatusEnum::kSuccess;
            helper.Success(response);
            ChipLogProgress(AppServer, "[MediaPlayback] STOP command successfull");
    }
}

void MediaPlaybackManager::HandleFastForward(CommandResponseHelper<Commands::PlaybackResponse::Type> & helper)
{
    ChipLogProgress(AppServer, "[MediaPlayback] FAST-FORWARD command received");

    if (mPlaybackSpeed == kPlaybackMaxForwardSpeed)
    {
        // if already at max speed, return error
        Commands::PlaybackResponse::Type response;
        response.data   = chip::MakeOptional(CharSpan::fromCharString("FastForward Command Failed"));
        response.status = MediaPlaybackStatusEnum::kSpeedOutOfRange;
        helper.Success(response);
        ChipLogError(AppServer, "[MediaPlayback] FAST-FORWARD Command Failed");
        return;
    }


    struct media_playback_set_data input_data;
    int payload = sizeof(struct media_playback_set_data);

    int ret = matter_notify(BASIC_VIDEO_PLAYER, MEDIA_PLAYBACK_FAST_FORWARD, payload, (struct media_playback_set_data *) &input_data);
    if(ret != 0)
    {
            Commands::PlaybackResponse::Type response;
            response.data   = chip::MakeOptional(CharSpan::fromCharString("FastForward Command Failed"));
            response.status = MediaPlaybackStatusEnum::kInvalidStateForCommand;
            helper.Success(response);
            ChipLogError(AppServer, "[MediaPlayback] FAST-FORWARD command failed, error: %d", ret);
    }
    else
    {
	    mCurrentState  = PlaybackStateEnum::kPlaying;
	    mPlaybackSpeed = (mPlaybackSpeed <= 0 ? 1 : mPlaybackSpeed * 2);
	    if (mPlaybackSpeed > kPlaybackMaxForwardSpeed)
	    {
		    // don't exceed max speed
		    mPlaybackSpeed = kPlaybackMaxForwardSpeed;
	    }

	    Commands::PlaybackResponse::Type response;
	    response.data   = chip::MakeOptional(CharSpan::fromCharString("FastForward Command Successfull"));
	    response.status = MediaPlaybackStatusEnum::kSuccess;
	    helper.Success(response);
	    ChipLogProgress(AppServer, "[MediaPlayback] FAST-FORWARD Command Successfull");
    }
}

void MediaPlaybackManager::HandlePrevious(CommandResponseHelper<Commands::PlaybackResponse::Type> & helper)
{
    ChipLogProgress(AppServer, "[MediaPlayback] PREVIOUS command received");

    struct media_playback_set_data input_data;
    int payload = sizeof(struct media_playback_set_data);

    int ret = matter_notify(BASIC_VIDEO_PLAYER, MEDIA_PLAYBACK_PREVIOUS, payload, (struct media_playback_set_data *) &input_data);
    if(ret != 0)
    {
            Commands::PlaybackResponse::Type response;
            response.data   = chip::MakeOptional(CharSpan::fromCharString("Previous Command Failed"));
            response.status = MediaPlaybackStatusEnum::kInvalidStateForCommand;
            helper.Success(response);
            ChipLogError(AppServer, "[MediaPlayback] PREVIOUS command failed, error: %d", ret);
    }
    else
    {
	    mCurrentState     = PlaybackStateEnum::kPlaying;
	    mPlaybackSpeed    = 1;
	    mPlaybackPosition = { 0, chip::app::DataModel::Nullable<uint64_t>(0) };

	    Commands::PlaybackResponse::Type response;
	    response.data   = chip::MakeOptional(CharSpan::fromCharString("Previous Command Successfull"));
	    response.status = MediaPlaybackStatusEnum::kSuccess;
	    helper.Success(response);
	    ChipLogProgress(AppServer, "[MediaPlayback] PREVIOUS Command Successfull");
    }
}

void MediaPlaybackManager::HandleRewind(CommandResponseHelper<Commands::PlaybackResponse::Type> & helper)
{
    ChipLogProgress(AppServer, "[MediaPlayback] REWIND command received");

    if (mPlaybackSpeed == kPlaybackMaxRewindSpeed)
    {
        // if already at max speed in reverse, return error
        Commands::PlaybackResponse::Type response;
        response.data   = chip::MakeOptional(CharSpan::fromCharString("Rewind Command Failed"));
        response.status = MediaPlaybackStatusEnum::kSpeedOutOfRange;
        helper.Success(response);
        ChipLogProgress(AppServer, "[MediaPlayback] Rewind Command Failed");
        return;
    }

    struct media_playback_set_data input_data;
    int payload = sizeof(struct media_playback_set_data);

    int ret = matter_notify(BASIC_VIDEO_PLAYER, MEDIA_PLAYBACK_REWIND, payload, (struct media_playback_set_data *) &input_data);
    if(ret != 0)
    {
            Commands::PlaybackResponse::Type response;
            response.data   = chip::MakeOptional(CharSpan::fromCharString("Rewind Command Failed"));
            response.status = MediaPlaybackStatusEnum::kInvalidStateForCommand;
            helper.Success(response);
            ChipLogError(AppServer, "[MediaPlayback] REWIND command failed, error: %d", ret);
    }
    else
    {
	    mCurrentState  = PlaybackStateEnum::kPlaying;
	    mPlaybackSpeed = (mPlaybackSpeed >= 0 ? -1 : mPlaybackSpeed * 2);
	    if (mPlaybackSpeed < kPlaybackMaxRewindSpeed)
	    {
		    // don't exceed max rewind speed
		    mPlaybackSpeed = kPlaybackMaxRewindSpeed;
	    }
	    Commands::PlaybackResponse::Type response;
	    response.data   = chip::MakeOptional(CharSpan::fromCharString("Rewind Command Successfull"));
	    response.status = MediaPlaybackStatusEnum::kSuccess;
	    helper.Success(response);
	    ChipLogProgress(AppServer, "[MediaPlayback] REWIND Command Successfull");
    }
}

void MediaPlaybackManager::HandleSkipBackward(CommandResponseHelper<Commands::PlaybackResponse::Type> & helper,
                                              const uint64_t & deltaPositionMilliseconds)
{
    ChipLogProgress(AppServer, "[MediaPlayback] SKIP-BACKWARD command received, interval: %llu ms", deltaPositionMilliseconds);

    uint64_t newPosition = (mPlaybackPosition.position.Value() > deltaPositionMilliseconds
                                ? mPlaybackPosition.position.Value() - deltaPositionMilliseconds
                                : 0);
    mPlaybackPosition    = { 0, chip::app::DataModel::Nullable<uint64_t>(newPosition) };
    
    struct media_playback_set_data input_data;
    input_data.skip_backward = newPosition;
    int payload = sizeof(struct media_playback_set_data);

    int ret = matter_notify(BASIC_VIDEO_PLAYER, MEDIA_PLAYBACK_SKIP_BACKWARD, payload, (struct media_playback_set_data *) &input_data);
    if(ret != 0)
    {
            Commands::PlaybackResponse::Type response;
            response.data   = chip::MakeOptional(CharSpan::fromCharString("SkipBackward Command Failed"));
            response.status = MediaPlaybackStatusEnum::kInvalidStateForCommand;
            helper.Success(response);
            ChipLogError(AppServer, "[MediaPlayback] SKIP-BACKWARD command failed, error: %d", ret);
    }
    else
    {
	    Commands::PlaybackResponse::Type response;
	    response.data   = chip::MakeOptional(CharSpan::fromCharString("SkipBackward Command Successfull"));
	    response.status = MediaPlaybackStatusEnum::kSuccess;
	    helper.Success(response);
	    ChipLogProgress(AppServer, "[MediaPlayback] SKIP-BACKWARD Command Successfull, newPosition: %llu ms", newPosition);
    }
}

void MediaPlaybackManager::HandleSkipForward(CommandResponseHelper<Commands::PlaybackResponse::Type> & helper,
                                             const uint64_t & deltaPositionMilliseconds)
{
    ChipLogProgress(AppServer, "[MediaPlayback] SKIP-FORWARD command received, interval: %llu ms", deltaPositionMilliseconds);

    uint64_t newPosition = mPlaybackPosition.position.Value() + deltaPositionMilliseconds;
    newPosition          = newPosition > mDuration ? mDuration : newPosition;
    mPlaybackPosition    = { 0, chip::app::DataModel::Nullable<uint64_t>(newPosition) };
    
    struct media_playback_set_data input_data;
    input_data.skip_forward = newPosition;
    int payload = sizeof(struct media_playback_set_data);

    int ret = matter_notify(BASIC_VIDEO_PLAYER, MEDIA_PLAYBACK_SKIP_FORWARD, payload, (struct media_playback_set_data *) &input_data);
    if(ret != 0)
    {
            Commands::PlaybackResponse::Type response;
            response.data   = chip::MakeOptional(CharSpan::fromCharString("SkipForward Command Failed"));
            response.status = MediaPlaybackStatusEnum::kInvalidStateForCommand;
            helper.Success(response);
            ChipLogError(AppServer, "[MediaPlayback] SKIP-FORWARD command failed, error: %d", ret);
    }
    else
    {
	    Commands::PlaybackResponse::Type response;
	    response.data   = chip::MakeOptional(CharSpan::fromCharString("SkipForward Command Successfull"));
	    response.status = MediaPlaybackStatusEnum::kSuccess;
	    helper.Success(response);
	    ChipLogProgress(AppServer, "[MediaPlayback] SKIP-FORWARD Command Successfull, newPosition: %llu", newPosition);
    }
}

void MediaPlaybackManager::HandleSeek(CommandResponseHelper<Commands::PlaybackResponse::Type> & helper,
                                      const uint64_t & positionMilliseconds)
{
    ChipLogProgress(AppServer, "[MediaPlayback] SEEK command received, newposition: %llu ms", positionMilliseconds);

    if (positionMilliseconds > mDuration)
    {
        Commands::PlaybackResponse::Type response;
        response.data   = chip::MakeOptional(CharSpan::fromCharString("Seek Command Failed"));
        response.status = MediaPlaybackStatusEnum::kSeekOutOfRange;
        helper.Success(response);
	ChipLogProgress(AppServer, "[MediaPlayback] Seek Command Failed");
    }
    else
    {
        mPlaybackPosition = { 0, chip::app::DataModel::Nullable<uint64_t>(positionMilliseconds) };
	
	struct media_playback_set_data input_data;
	input_data.seek_pos = positionMilliseconds;
	int payload = sizeof(struct media_playback_set_data);

	int ret = matter_notify(BASIC_VIDEO_PLAYER, MEDIA_PLAYBACK_SEEK, payload, (struct media_playback_set_data *) &input_data);

	if(ret != 0)
	{
		Commands::PlaybackResponse::Type response;
		response.data   = chip::MakeOptional(CharSpan::fromCharString("Seek Command Failed"));
		response.status = MediaPlaybackStatusEnum::kInvalidStateForCommand;
		helper.Success(response);
		ChipLogError(AppServer, "[MediaPlayback] SEEK command failed, error: %d", ret);
	}
	else
	{
		Commands::PlaybackResponse::Type response;
		response.data   = chip::MakeOptional(CharSpan::fromCharString("Seek Command Successfull"));
		response.status = MediaPlaybackStatusEnum::kSuccess;
		helper.Success(response);
		ChipLogProgress(AppServer, "[MediaPlayback] SEEK Command Successfull, newposition: %llu ms", positionMilliseconds);
	}
    }
}

void MediaPlaybackManager::HandleNext(CommandResponseHelper<Commands::PlaybackResponse::Type> & helper)
{
    ChipLogProgress(AppServer, "[MediaPlayback] NEXT command received");

    struct media_playback_set_data input_data;
    int payload = sizeof(struct media_playback_set_data);

    int ret = matter_notify(BASIC_VIDEO_PLAYER, MEDIA_PLAYBACK_NEXT, payload, (struct media_playback_set_data *) &input_data);
    if(ret != 0)
    {
            Commands::PlaybackResponse::Type response;
            response.data   = chip::MakeOptional(CharSpan::fromCharString("Next Command Failed"));
            response.status = MediaPlaybackStatusEnum::kInvalidStateForCommand;
            helper.Success(response);
            ChipLogError(AppServer, "[MediaPlayback] NEXT command failed, error: %d", ret);
    }
    else
    {
	    mCurrentState     = PlaybackStateEnum::kPlaying;
	    mPlaybackSpeed    = 1;
	    mPlaybackPosition = { 0, chip::app::DataModel::Nullable<uint64_t>(0) };

	    Commands::PlaybackResponse::Type response;
	    response.data   = chip::MakeOptional(CharSpan::fromCharString("Next Command Successfull"));
	    response.status = MediaPlaybackStatusEnum::kSuccess;
	    helper.Success(response);
	    ChipLogProgress(AppServer, "[MediaPlayback] NEXT Command Successfull");
    }
}

void MediaPlaybackManager::HandleStartOver(CommandResponseHelper<Commands::PlaybackResponse::Type> & helper)
{
    ChipLogProgress(AppServer, "[MediaPlayback] START-OVER command received");

    struct media_playback_set_data input_data;
    int payload = sizeof(struct media_playback_set_data);

    int ret = matter_notify(BASIC_VIDEO_PLAYER, MEDIA_PLAYBACK_STARTOVER, payload, (struct media_playback_set_data *) &input_data);
    if(ret != 0)
    {
            Commands::PlaybackResponse::Type response;
            response.data   = chip::MakeOptional(CharSpan::fromCharString("StartOver Command Failed"));
            response.status = MediaPlaybackStatusEnum::kInvalidStateForCommand;
            helper.Success(response);
            ChipLogError(AppServer, "[MediaPlayback] START-OVER command failed, error: %d", ret);
    }
    else
    {
	    mPlaybackPosition = { 0, chip::app::DataModel::Nullable<uint64_t>(0) };

	    Commands::PlaybackResponse::Type response;
	    response.data   = chip::MakeOptional(CharSpan::fromCharString("StartOver Command Successfull"));
	    response.status = MediaPlaybackStatusEnum::kSuccess;
	    helper.Success(response);
	    ChipLogProgress(AppServer, "[MediaPlayback] START-OVER Command Successfull");
    }
}

uint32_t MediaPlaybackManager::GetFeatureMap(chip::EndpointId endpoint)
{
    if (endpoint >= EMBER_AF_CONTENT_LAUNCHER_CLUSTER_SERVER_ENDPOINT_COUNT)
    {
        return mDynamicEndpointFeatureMap;
    }

    uint32_t featureMap = 0;
    Attributes::FeatureMap::Get(endpoint, &featureMap);
    return featureMap;
}
