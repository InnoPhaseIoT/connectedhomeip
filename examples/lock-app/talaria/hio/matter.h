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

#define FOTA_BUFF_LEN 512
#define DOORLOCK_BUFF_LEN 32

enum matter_clusters
{
    DOOR_LOCK = 1,
    OTA_SOFTWARE_UPDATE_REQUESTOR,
    THERMOSTAT,
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
    char username[DOORLOCK_BUFF_LEN]; /**< username */
    uint32_t useruniqueid;            /**< useruniqueid */
    uint8_t userstatus;               /**< userstatus */
    uint8_t usertype;                 /**< usertyp*/
    uint8_t credentialrule;           /**< credentialrule */
};

struct dl_set_get_credential
{
    uint8_t operationtype;                  /**< operationtype */
    uint32_t credential;                    /**< credential */
    char credentialdata[DOORLOCK_BUFF_LEN]; /**< credentialdata */
    uint8_t userindex;                      /**< userindex */
    uint8_t userstatus;                     /**< userstatus */
    uint8_t usertype;                       /**< usertyp*/
};

int matter_notify(const uint32_t cluster, const uint32_t cmd, const uint32_t payload_len, void * data);
void matter_hio_init(void);
