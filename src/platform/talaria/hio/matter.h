/**
 ******************************************************************************
 * @file   matter.h
 *
 * @brief  definitions for host api for matter

 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023, InnoPhase IoT, Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/*
 * MATTER API specification
 */

#include <kernel/hostio.h>
#include <stdio.h>
#include <string.h>
#include <CHIPProjectAppConfig.h>

#define FOTA_BUFF_LEN 512
#define DOORLOCK_BUFF_LEN 32

enum matter_clusters
{
    DOOR_LOCK = 1,
    OTA_SOFTWARE_UPDATE_REQUESTOR,
    THERMOSTAT,
    SPEAKER,
    BASIC_VIDEO_PLAYER,
    SMOKE_CO_ALARM,
    FACTORY_RESET,
    MAX_CLUSTER,
    /*USERS CAN ADD CLUSTERS HERE*/
};

enum osur_cluster_cmd
{
    FOTA_COMMAND_BY_ID = 1,
    FOTA_ANNOUNCE_OTAPROVIDER,
    FOTA_READ_BY_ID,
    FOTA_READ,
    FOTA_WRITE_BY_ID,
    FOTA_WRITE,
    FOTA_FORCE_WRITE,
    FOTA_SUBSCRIBE_BY_ID,
    FOTA_SUBSCRIBE,
    FOTA_READ_EVENT_BY_ID,
    FOTA_READ_EVENT,
    FOTA_SUBSCRIBE_EVENT_BY_ID,
    FOTA_SUBSCRIBE_EVENT,
    FOTA_HASH,
    FOTA_NAME,
    FOTA_IMAGE_INTEGRITY_CHECK,
};

enum doorlock_cluster_cmd
{
    LOCK_DOOR = 1,
    UNLOCK_DOOR,
    UNLOCK_WITH_TIMEOUT,
    SET_WEEK_DAY_SCHEDULE,
    GET_WEEK_DAY_SCHEDULE,
    CLEAR_WEEK_DAY_SCHEDULE,
    SET_YEAR_DAY_SCHEDULE,
    GET_YEAR_DAY_SCHEDULE,
    CLEAR_YEAR_DAY_SCHEDULE,
    SET_HOLIDAY_SCHEDULE,
    GET_HOLIDAY_SCHEDULE,
    CLEAR_HOLIDAY_SCHEDULE,
    SET_USER,
    GET_USER,
    CLEAR_USER,
    SET_CREDENTIAL,
    GET_CREDENTIAL_STATUS,
    CLEAR_CREDENTIAL,
    UNBOLT_DOOR,
    READ_BY_ID,
    READ,
    WRITE_BY_ID,
    FORCE_WRITE,
    WRITE,
    SUBSCRIBE_BY_ID,
    SUBSCRIBE,
    READ_EVENT_BY_ID,
    READ_EVENT,
    SUBSCRIBE_EVENT_BY_ID,
    SUBSCRIBE_EVENT,
};

enum factory_reset_cluster_cmd {
	FACTORY_RESET_1 = 1,
	FACTORY_RESET_2,
	RESET,
	MAX_COMMAND=999,
};

#if CHIP_DEVICE_CONFIG_DEVICE_TYPE == 34
enum speaker_cluster_cmd
{
    SPEAKER_UNMUTE = 1,
    SPEAKER_MUTE,
    SPEAKER_MOVE_TO_LEVEL,
};
#endif /* CHIP_DEVICE_CONFIG_DEVICE_TYPE SPEAKER */

#if CHIP_DEVICE_CONFIG_DEVICE_TYPE == 769
enum thermostat_cluster_cmd
{
    THERMOSTAT_SETPOINT_RAISE_LOWER = 1,
    THERMOSTAT_READ_TEMPERATURE,
    THERMOSTAT_GET_DATA,
};
#endif /* CHIP_DEVICE_CONFIG_DEVICE_TYPE THERMOSTAT */

#if (CHIP_DEVICE_CONFIG_DEVICE_TYPE == 40)
enum basic_video_player_cluster_cmd
{   
    MEDIA_PLAYBACK_ON = 1,
    MEDIA_PLAYBACK_OFF,
    MEDIA_PLAYBACK_PLAY,
    MEDIA_PLAYBACK_PAUSE,
    MEDIA_PLAYBACK_STOP,
    MEDIA_PLAYBACK_STARTOVER,
    MEDIA_PLAYBACK_PREVIOUS,
    MEDIA_PLAYBACK_NEXT,
    MEDIA_PLAYBACK_REWIND,
    MEDIA_PLAYBACK_FAST_FORWARD,
    MEDIA_PLAYBACK_SKIP_FORWARD,
    MEDIA_PLAYBACK_SKIP_BACKWARD,
    MEDIA_PLAYBACK_SEEK,
    KEYPAD_INPUT_SEND_KEY,
};
#endif /* CHIP_DEVICE_CONFIG_DEVICE_TYPE BASIC VIDEO PLAYER */

#if (CHIP_DEVICE_CONFIG_DEVICE_TYPE == 118)
enum smoke_co_alarm_cluster_cmd
{
    SMOKE_CO_ALARM_SELF_TEST_REQUEST = 1,
    SMOKE_CO_ALARM_GET_DATA,
};
#endif /* CHIP_DEVICE_CONFIG_DEVICE_TYPE SMOKE CO ALARM */

struct ota_data
{
    char fota_img_data[FOTA_BUFF_LEN]; /**< username */
};

struct dl_unlock_with_timeout
{
    uint32_t unlock_time_out; /**< unlock_time_out */
};

struct dl_set_get_user
{
    uint8_t operationtype;            /**< operationtype */
    uint8_t userindex;                /**< userindex */
    uint8_t credentialindex;          /**< credentialindex */
    char username[DOORLOCK_BUFF_LEN]; /**< username */
    uint32_t useruniqueid;            /**< useruniqueid */
    uint8_t userstatus;               /**< userstatus */
    uint8_t usertype;                 /**< usertyp*/
    uint8_t credentialrule;           /**< credentialrule */
    uint8_t FabricIndexCreator;       /**< creater FabricIndex */
    uint8_t FabricIndexModifier ;     /**< modifier FabricIndex*/
};

struct dl_set_get_credential
{
    uint8_t operationtype;                  /**< operationtype */
    uint32_t credential;                    /**< credential */
    char credentialdata[DOORLOCK_BUFF_LEN]; /**< credentialdata */
    uint8_t userindex;                      /**< userindex */
    uint8_t userstatus;                     /**< userstatus */
    uint8_t usertype;                       /**< usertyp*/
    uint8_t FabricIndexCreator;             /**< creater FabricIndex */
    uint8_t FabricIndexModifier;            /**< modifier FabricIndex */
};

#if CHIP_DEVICE_CONFIG_DEVICE_TYPE == 34
struct speaker_set_data
{
    uint8_t onoff; /**< Volume Level*/
    uint8_t Level; /**< Volume Level*/
};
#endif /* CHIP_DEVICE_CONFIG_DEVICE_TYPE SPEAKER */

#if CHIP_DEVICE_CONFIG_DEVICE_TYPE == 769
struct thermostat_read_temperature
{
    int16_t LocalTemperature;
};

struct thermostat_set_data
{
    uint8_t mode;            /**< Setpoint mode, heating/cooling*/
    int16_t CoolingSetpoint; /**< Occupied Cooling SetPoint */
    int16_t HeatingSetpoint; /**< Occupied Heating SetPoint */
};

#endif /* CHIP_DEVICE_CONFIG_DEVICE_TYPE THERMOSTAT */

#if (CHIP_DEVICE_CONFIG_DEVICE_TYPE == 40)
struct media_playback_set_data
{
    uint8_t onoff;         /**< On/Off media player*/
    uint64_t seek_pos;      /**< seek position interval in milliseconds*/
    uint64_t skip_forward;  /**< skip forward interval in milliseconds*/
    uint64_t skip_backward; /**< skip backward interval in milliseconds*/
};

struct keypad_input_set_data
{
    uint8_t KeyCode; /**< Key Code to send */
};
#endif /* CHIP_DEVICE_CONFIG_DEVICE_TYPE BASIC VIDEO PLAYER */

#if (CHIP_DEVICE_CONFIG_DEVICE_TYPE == 118)
struct smoke_co_alarm_set_data
{
    uint8_t self_test_request;         /**< self_test_request */
};

struct smoke_co_alarm_get_data
{
    uint8_t ExpressedState;         /**< Expressed State */
    uint8_t SmokeState;             /**< Smoke State */
    uint8_t COState;	            /**< CO State */
    uint8_t BatteryAlert;           /**< Battery Alert status */
    uint8_t DeviceMuted;            /**< Device Muted State */
    bool TestInProgress;            /**< TestIn Progress status */
    bool HardwareFaultAlert;        /**< HardwareFault Alert status */
    uint8_t EndOfServiceAlert;      /**< End Of ServiceAlert status */
    uint8_t InterconnectSmokeAlarm; /**< InterconnectSmokeAlarm state */
    uint8_t InterconnectCOAlarm;    /**< InterconnectCOAlarm state */
    uint8_t ContaminationState;     /**< Contamination State */
    uint8_t SmokeSensitivityLevel;  /**< SmokeSensitivityLevel State */
    uint32_t ExpiryDate;            /**< ExpiryDate */
    uint8_t PowerSourceStatus;      /**< Power Source Status */
    uint8_t BatChargeLevel;         /**< Battery Charge Level */
    bool BatReplacementNeeded;      /**< Battery Replacement Needed */
    uint8_t BatReplaceability;      /**< Battery Replaceability */
};
#endif /* CHIP_DEVICE_CONFIG_DEVICE_TYPE SMOKE CO ALARM */

int matter_notify(const uint32_t cluster, const uint32_t cmd, const uint32_t payload_len, void * data);
void matter_hio_init(void);
