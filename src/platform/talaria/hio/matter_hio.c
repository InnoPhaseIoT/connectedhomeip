/**
  ******************************************************************************
  * @file   matter_hio.c
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
#include "FreeRTOS.h"
// #include "hio_utils.h"
#include "matter.h"
// #include "osal.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include <assert.h>
#include <kernel/hostio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <talaria_two.h>

#pragma GCC diagnostic push
/* Suppress the "-Wstringop-truncation" warning. It was created by cogg logic.!
 */
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#include "matter_hio.h"
#pragma GCC diagnostic pop
#include <CHIPProjectAppConfig.h>

struct os_semaphore hio_ind_sem;

// #define TESTCODE
static bool hio_matter_thread_init = false;
TaskHandle_t hio_matter_thread     = NULL;
QueueHandle_t matter_cmd_queue;
extern SemaphoreHandle_t Getdata;

#if (CHIP_DEVICE_CONFIG_DEVICE_TYPE == 10)
extern struct dl_set_get_user revd_user;
extern struct dl_set_get_credential revd_credential;
#endif /* CHIP_DEVICE_CONFIG_DEVICE_TYPE DOORLOCK */

#if (CHIP_DEVICE_CONFIG_DEVICE_TYPE == 769)
extern struct thermostat_get_data revd_data;
extern struct thermostat_read_temperature revd_temp;
#endif /* CHIP_DEVICE_CONFIG_DEVICE_TYPE THERMOSTAT */

#if (CHIP_DEVICE_CONFIG_DEVICE_TYPE == 118)
extern struct smoke_co_alarm_get_data revd_smoke_co_alarm_data;
#endif /* CHIP_DEVICE_CONFIG_DEVICE_TYPE SMOKE CO ALARM */

enum
{
    HIO_MATTER_EVT_MSG = 9999,
};

/* Declarations from hio_utils.h */
static struct hio_msg_hdr
{
    /* Arbitrary integer representing the message type */
    int msg_type;
    void * msg_sender;
};

static struct hio_req_msg
{
    struct hio_msg_hdr hdr;
    int event;
    struct packet * packet;
};

void hio_reqmsg_send(QueueHandle_t xQueue, int msg_type, int event, struct packet * pkt);

void hio_reqmsg_free(struct hio_msg_hdr * msg);

/*fota code start*/

#include "json.h"

#define MOUNT_PATH "/data/"
#define FOTA_CFG_FILE_PATH MOUNT_PATH "fota_config.json"
#define FOTA_CFG_FLAG_PATH MOUNT_PATH "fota_flag.json"

enum
{
    JSON_PARSE_STATE_KEY,
    JSON_PARSE_STATE_SEPRATOR,
    JSON_PARSE_STATE_VAL
};

#define MAX_BUFFER_SIZE 512

static int confDataIndex;
static char * confData = NULL;

void conf_clear()
{
    vPortFree(confData);
    confData = NULL;
}

static char confbufgetc(char * buf)
{
    char ch = buf[confDataIndex];
    confDataIndex++;
    return ch;
}

int fota_flag_status_set(char * param_name, char * param_val)
{

    struct json_parser json;

    int ch             = 'A';
    unsigned int state = JSON_PARSE_STATE_KEY;
    int name_found     = 0;
    char * finbuff     = NULL;
    int retval = -1, cnfDataLen = 0, endposition = 0, startposition = 0;
    int index        = 0;
    int paramLenDiff = 0;

    FILE * f_part;
    size_t bytesRead;
    f_part = fopen(FOTA_CFG_FLAG_PATH, "r+");
    if (f_part == NULL)
    {
        os_printf("Failed to open the file.\n");
        return 1;
    }

    confData = pvPortMalloc(MAX_BUFFER_SIZE);

    os_printf("Contents of the file:\n");
    bytesRead = fread(confData, 1, MAX_BUFFER_SIZE, f_part);
    if (bytesRead == 0)
    {
        os_printf("Failed to read from the file.\n");
        fclose(f_part);
        return 1;
    }
    confData[bytesRead] = '\0';
    json_init(&json);
    while (1)
    {
        if (ch == EOF || ch == '\0')
        {
            os_printf("\nEOF / End of String reached");
            break;
        }
        ch = confbufgetc(confData);
        endposition++;
        volatile int t = json_tokenizer(&json, ch);
        if (t == JSON_END)
            break;
        if (t == JSON_BEGIN_ARRAY)
        {
            state = JSON_PARSE_STATE_KEY;
            continue;
        }

        if (JSON_PARSE_STATE_KEY == state && t == JSON_STRING)
        {
            os_printf("\nKey= %s", json.token);
            state = JSON_PARSE_STATE_SEPRATOR;

            if (strcmp(json.token, param_name) == 0)
            {
                name_found = 1;
            }
            continue;
        }

        if (JSON_PARSE_STATE_SEPRATOR == state && t == JSON_MEMBER_SEPARATOR)
        {
            state = JSON_PARSE_STATE_VAL;
            continue;
        }

        if (state == JSON_PARSE_STATE_VAL)
        {

            if (t == JSON_STRING)
            {
                os_printf("\nposition = %d", endposition);
                os_printf("\nVal(str)= %s", json.token);
                if (name_found)
                {
                    break;
                }
            }
            if (t == JSON_STRING || t == JSON_NUMBER)
            {
                state = JSON_PARSE_STATE_KEY;
            }
        }
    }

    endposition   = endposition - 1;
    startposition = endposition - strlen(json.token);

    cnfDataLen   = strlen(confData);
    paramLenDiff = strlen(param_val) - strlen(json.token);

    finbuff = pvPortMalloc(cnfDataLen + paramLenDiff + 1);
    if (!finbuff)
    {
        conf_clear();
        return 1;
    }

    index = 0;
    memcpy(finbuff, confData, startposition); // copy till the start;
    index += startposition;
    memcpy(finbuff + index, param_val, strlen(param_val)); // copy new data
    index += strlen(param_val);
    memcpy(finbuff + index, confData + endposition, (cnfDataLen - endposition)); // copy remaining data
    index += (cnfDataLen - endposition);
    finbuff[index] = '\0';

    json_finish(&json);
    fseek(f_part, 0, SEEK_SET);

    if (fputs(finbuff, f_part) == EOF)
    {
        os_printf("Failed to write to file %s.\n", FOTA_CFG_FLAG_PATH);
    }
    else
    {
        os_printf("Successfully wrote to file %s.\n", FOTA_CFG_FLAG_PATH);
    }
    fclose(f_part);

    conf_clear();
    vPortFree(confData);
    return 0;
}
/*fota code end*/

static void hio_matter_data_ind_free(struct packet ** pkt)
{
    packet_free(*pkt);
    *pkt = NULL;
    os_sem_post(&hio_ind_sem);
}

int matter_notify(const uint32_t cluster, const uint32_t cmd, const uint32_t payload_len, void * data)
{
    struct packet * pkt;
    pkt = create_matter_cmd_notify_ind(cluster, cmd, payload_len, NULL);
    if (pkt == NULL)
    {
        os_printf("\nPkt alloc error");
        return -1;
    }
    if (payload_len)
    {
        packet_append(pkt, data, payload_len);
        hio_packet_set_free_hook(&pkt, NULL, &hio_matter_data_ind_free);
    }
    hio_write_msg(pkt, HIO_GROUP_MATTER, MATTER_CMD_NOTIFY_IND, 0);
    os_sem_wait(&hio_ind_sem); /* Wait here until packet is sent */
    return 0;
}

static void hio_matter_send_rsp(struct os_thread * sender, int msg_type, struct packet * pkt)
{
    hio_respmsg_send(sender, msg_type, pkt);
}

#ifdef TESTCODE
int dam = 0;
#endif

static void matter_data_req(struct os_thread * sender, struct packet * msg)
{
    int ok                            = false;
    struct matter_data_send_req * req = packet_data(msg);
    struct packet * rsp_pkt;

#if (CHIP_DEVICE_CONFIG_DEVICE_TYPE == 10)
    if (strncmp(req->data, "cm_ok", 5) == 0)
    {
        openCommissionWindow();
    }
#endif /* CHIP_DEVICE_CONFIG_DEVICE_TYPE DOORLOCK */

    if (strncmp(req->data, "do_fota", 7) == 0)
    {
        fota_flag_status_set("fota_in_progress", "1");
    }

#if (CHIP_DEVICE_CONFIG_DEVICE_TYPE == 10)
    if (req->cmd == GET_USER)
    {
        // os_printf("\r\nget_user\r\n");
        memcpy(&revd_user, req->data, sizeof(struct dl_set_get_user));
        xSemaphoreGive(Getdata);
    }
    else if (req->cmd == GET_CREDENTIAL_STATUS)
    {
        // os_printf("\r\nget_credential\r\n");
        memcpy(&revd_credential, req->data, sizeof(struct dl_set_get_credential));
        xSemaphoreGive(Getdata);
    }
#endif /* CHIP_DEVICE_CONFIG_DEVICE_TYPE DOORLOCK */

#if (CHIP_DEVICE_CONFIG_DEVICE_TYPE == 769)
    if (req->cmd == THERMOSTAT_GET_DATA)
    {
        // os_printf("\r\nget_credential\r\n");
        memcpy(&revd_data, req->data, sizeof(struct thermostat_get_data));
        xSemaphoreGive(Getdata);
        os_printf("\r\n [Thermostat] get_data: data received from host...\r\n");
    }
    else if (req->cmd == THERMOSTAT_READ_TEMPERATURE)
    {
        // os_printf("\r\nget_credential\r\n");
        memcpy(&revd_temp, req->data, sizeof(struct thermostat_read_temperature));
        xSemaphoreGive(Getdata);
        os_printf("\r\n [Thermostat] get_temp: temperature received from host...\r\n");
    }
#endif /* CHIP_DEVICE_CONFIG_DEVICE_TYPE THERMOSTAT */

#if (CHIP_DEVICE_CONFIG_DEVICE_TYPE == 118)
    if (req->cmd == SMOKE_CO_ALARM_GET_DATA)
    {
        memcpy(&revd_smoke_co_alarm_data, req->data, sizeof(struct smoke_co_alarm_get_data));
        Smoke_co_alarm_update_status();
    }
#endif /* CHIP_DEVICE_CONFIG_DEVICE_TYPE SMOKE CO ALARM */

#ifdef TESTCODE
    os_printf("\r\ncluster: %d\r\n", req->cluster);
    os_printf("\r\ncmd: %d\r\n", req->cmd);
    switch (req->cmd)
    {
    case GET_USER: {
        struct dl_set_get_user usr;
        memcpy(&usr, req->data, sizeof(struct dl_set_get_user));
        os_printf("\r\n%d", usr.userindex);
        os_printf("\r\n%d", usr.operationtype);
        os_printf("\r\n%d", usr.userstatus);
        os_printf("\r\n%d", usr.usertype);
        os_printf("\r\n%d", usr.useruniqueid);
        os_printf("\r\n%d", usr.credentialrule);
        os_printf("\r\n%s", usr.username);
        break;
    }

    case GET_CREDENTIAL_STATUS: {
        struct dl_set_get_credential cred;
        memcpy(&cred, req->data, sizeof(struct dl_set_get_credential));
        os_printf("\r\n%d", cred.userindex);
        os_printf("\r\n%d", cred.operationtype);
        os_printf("\r\n%d", cred.userstatus);
        os_printf("\r\n%d", cred.usertype);
        os_printf("\r\n%d", cred.credential);
        os_printf("\r\n%s", cred.credentialdata);
        break;
    }
    default:
        printf("Invalid input\n");
        break;
    }
#endif
    ok      = true;
    rsp_pkt = OS_ERROR_ON_NULL(create_matter_data_send_rsp(ok));
    hio_matter_send_rsp(sender, MATTER_DATA_SEND_RSP, rsp_pkt);

#ifdef TESTCODE
    vTaskDelay(10000);
    dam++;

    if (dam == 1)
    {
        struct dl_unlock_with_timeout time;
        time.unlock_time_out = 5000;
        int payload          = sizeof(struct dl_unlock_with_timeout);
        matter_notify(DOOR_LOCK, UNLOCK_WITH_TIMEOUT, payload, (struct dl_unlock_with_timeout *) &time);
        vTaskDelay(7000);

        struct dl_set_get_user dd;
        memset(&dd, 0, sizeof(struct dl_set_get_user));
        dd.userindex      = 3;
        dd.operationtype  = 44;
        dd.userstatus     = 55;
        dd.usertype       = 66;
        dd.useruniqueid   = 77;
        dd.credentialrule = 88;
        strcpy(dd.username, "aa");
        payload = sizeof(struct dl_set_get_user);
        matter_notify(DOOR_LOCK, SET_USER, payload, (struct dl_set_get_user *) &dd);
        vTaskDelay(5000);

        matter_notify(DOOR_LOCK, GET_USER, payload, (struct dl_set_get_user *) &dd);
    }
    if (dam == 2)
    {
        struct dl_set_get_user ee = { 0, 3, "\0", 0, 0, 0, 0 };
        int payload               = sizeof(struct dl_set_get_user);
        matter_notify(DOOR_LOCK, CLEAR_USER, payload, (struct dl_set_get_user *) &ee);
        vTaskDelay(5000);

        ee.userindex = 3;
        matter_notify(DOOR_LOCK, GET_USER, payload, (struct dl_set_get_user *) &ee);
    }
#endif
}

static void hio_matter_thread_entry(void * arg)
{
    uint32_t msg, recv_status = 1;
    matter_cmd_queue = xQueueCreate(8, (UBaseType_t) sizeof(uint32_t));

    for (;;)
    {
        struct hio_req_msg * hio_req;
        recv_status = xQueueReceive(matter_cmd_queue, &msg, portMAX_DELAY);
        if (recv_status == 0)
        {
            continue;
        }
        hio_req             = (struct hio_req_msg *) msg;
        int event           = hio_req->event;
        int msg_type        = hio_req->hdr.msg_type;
        struct packet * pkt = hio_req->packet;
        void * sender       = (hio_req->hdr.msg_sender);

        hio_reqmsg_free(&hio_req->hdr);

        if (msg_type != HIO_MATTER_EVT_MSG)
        {
            continue;
        }
        else if (event == MATTER_DATA_SEND_REQ)
        {
            matter_data_req(sender, pkt);
        }
    }
    return;
}

static struct packet * matter_data_send(void * ctx, struct packet * msg)
{
    int event = MATTER_DATA_SEND_REQ;
    hio_reqmsg_send(matter_cmd_queue, HIO_MATTER_EVT_MSG, event, msg);
    return hio_respmsg_recv(MATTER_DATA_SEND_RSP);
}

/*
 * [[[cog
 * import cog, generate
 * generate.api(generate.Emitter(cog.out), 'matter')
 * ]]] */
static const struct hio_api matter_api = { .group        = 95,
                                           .num_handlers = 2,
                                           .handler      = {
                                               matter_data_send,
                                               NULL /* cmd_notify */,
                                           } };

/* [[[end]]] */

static void hio_matter_create_thread(void)
{
    if (true != hio_matter_thread_init)
    {
        hio_matter_thread_init = true;
        xTaskCreate(hio_matter_thread_entry, /* The function that implements the
                                              * task. */
                    "hio_matter",            /* The text name assigned to the task - for debug only
                                              * as * it is not used by the kernel. */
                    1024,                    /* The size of the stack to allocate to the task. */
                    NULL,                    /* The parameter passed to the task - not used in this case.
                                              */
                    (tskIDLE_PRIORITY + 2),  /* The priority assigned to the task. */
                    &hio_matter_thread);
    }
}

void matter_hio_init(void)
{
    hio_api_init(&matter_api, NULL);
    os_sem_init(&hio_ind_sem, 0);
    hio_matter_create_thread();
}
