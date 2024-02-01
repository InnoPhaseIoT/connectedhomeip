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
//#include "hio_utils.h"
#include "matter.h"
//#include "osal.h"
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

// #define TESTCODE
static bool hio_matter_thread_init = false;
TaskHandle_t matter_thread = NULL;
QueueHandle_t matter_cmd_queue;
extern struct dl_set_get_user revd_user;
extern struct dl_set_get_credential revd_credential;
extern SemaphoreHandle_t Getdata;

enum {
    HIO_MATTER_EVT_MSG = 9999,
};

/* Declarations from hio_utils.h */
static struct hio_msg_hdr {
    /* Arbitrary integer representing the message type */
    int msg_type;
    void *msg_sender;
};

static struct hio_req_msg {
    struct hio_msg_hdr hdr;
    int event;
    struct packet *packet;
};

void hio_reqmsg_send(
		QueueHandle_t xQueue, int msg_type, int event, struct packet *pkt);

void hio_reqmsg_free(struct hio_msg_hdr *msg);


static void
hio_matter_data_ind_free(struct packet **pkt)
{
    packet_free(*pkt);
    *pkt = NULL;
}

void
matter_doorlock_notify(const uint32_t cluster, const uint32_t cmd,
    const uint32_t payload_len, void *data)
{
    struct packet *pkt;
    pkt = create_matter_cmd_notify_ind(cluster, cmd, payload_len, NULL);
    if (pkt == NULL) {
        os_printf("\nPkt alloc error");
        return;
    }
    packet_append(pkt, data, payload_len);
    hio_packet_set_free_hook(&pkt, NULL, &hio_matter_data_ind_free);
    hio_write_msg(pkt, HIO_GROUP_MATTER, MATTER_CMD_NOTIFY_IND, 0);
}

static void
hio_matter_send_rsp(struct os_thread *sender, int msg_type, struct packet *pkt)
{
    hio_respmsg_send(sender, msg_type, pkt);
}

#ifdef TESTCODE
int dam = 0;
#endif

static void
matter_data_req(struct os_thread *sender, struct packet *msg)
{
    int ok = false;
    struct matter_data_send_req *req = packet_data(msg);
    struct packet *rsp_pkt;

    if(strncmp(req->data,"cm_ok", 5) == 0)
    {
        openCommissionWindow();
    }

    if (req->cmd == GET_USER) {
        // os_printf("\r\nget_user\r\n");
        memcpy(&revd_user, req->data, sizeof(struct dl_set_get_user));
        xSemaphoreGive(Getdata);
    } else if (req->cmd == GET_CREDENTIAL_STATUS) {
        // os_printf("\r\nget_credential\r\n");
        memcpy(&revd_credential, req->data, sizeof(struct dl_set_get_credential));
        xSemaphoreGive(Getdata);
    }

#ifdef TESTCODE
    os_printf("\r\ncluster: %d\r\n", req->cluster);
    os_printf("\r\ncmd: %d\r\n", req->cmd);
    switch (req->cmd) {
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
    ok = true;
    rsp_pkt = OS_ERROR_ON_NULL(create_matter_data_send_rsp(ok));
    hio_matter_send_rsp(sender, MATTER_DATA_SEND_RSP, rsp_pkt);

#ifdef TESTCODE
    vTaskDelay(10000);
    dam++;

    if (dam == 1) {
        struct dl_unlock_with_timeout time;
        time.unlock_time_out = 5000;
        int payload = sizeof(struct dl_unlock_with_timeout);
        matter_doorlock_notify(DOOR_LOCK, UNLOCK_WITH_TIMEOUT, payload,
            (struct dl_unlock_with_timeout *)&time);
        vTaskDelay(7000);

        struct dl_set_get_user dd;
        memset(&dd, 0, sizeof(struct dl_set_get_user));
        dd.userindex = 3;
        dd.operationtype = 44;
        dd.userstatus = 55;
        dd.usertype = 66;
        dd.useruniqueid = 77;
        dd.credentialrule = 88;
        strcpy(dd.username, "aa");
        payload = sizeof(struct dl_set_get_user);
        matter_doorlock_notify(
            DOOR_LOCK, SET_USER, payload, (struct dl_set_get_user *)&dd);
        vTaskDelay(5000);

        matter_doorlock_notify(
            DOOR_LOCK, GET_USER, payload, (struct dl_set_get_user *)&dd);
    }
    if (dam == 2) {
        struct dl_set_get_user ee = { 0, 3, "\0", 0, 0, 0, 0 };
        int payload = sizeof(struct dl_set_get_user);
        matter_doorlock_notify(
            DOOR_LOCK, CLEAR_USER, payload, (struct dl_set_get_user *)&ee);
        vTaskDelay(5000);

        ee.userindex = 3;
        matter_doorlock_notify(
            DOOR_LOCK, GET_USER, payload, (struct dl_set_get_user *)&ee);
    }
#endif
}

static void
hio_matter_thread_entry(void *arg)
{
    uint32_t msg, recv_status = 1;
    matter_cmd_queue = xQueueCreate(8, (UBaseType_t)sizeof(uint32_t));

    for (;;) {
        struct hio_req_msg *hio_req;
        recv_status = xQueueReceive(matter_cmd_queue, &msg, portMAX_DELAY);
        if (recv_status == 0) {
            continue;
        }
        hio_req = (struct hio_req_msg *)msg;
        int event = hio_req->event;
        int msg_type = hio_req->hdr.msg_type;
        struct packet *pkt = hio_req->packet;
        void *sender = (hio_req->hdr.msg_sender);

        hio_reqmsg_free(&hio_req->hdr);

        if (msg_type != HIO_MATTER_EVT_MSG) {
            continue;
        } else if (event == MATTER_DATA_SEND_REQ) {
            matter_data_req(sender, pkt);
        }
    }
    return;
}

static struct packet *
matter_data_send(void *ctx, struct packet *msg)
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
static const struct hio_api matter_api = { .group = 95,
    .num_handlers = 2,
    .handler = {
        matter_data_send,
        NULL /* cmd_notify */,
    } };

/* [[[end]]] */

static void
hio_matter_create_thread(void)
{
    if (true != hio_matter_thread_init) {
        hio_matter_thread_init = true;
        xTaskCreate(hio_matter_thread_entry, /* The function that implements the
                                              * task. */
            "hio_matter", /* The text name assigned to the task - for debug only
                           * as * it is not used by the kernel. */
            1024, /* The size of the stack to allocate to the task. */
            NULL, /* The parameter passed to the task - not used in this case.
                   */
            (tskIDLE_PRIORITY + 2), /* The priority assigned to the task. */
            &matter_thread);
    }
}

void
matter_hio_init(void)
{
    hio_api_init(&matter_api, NULL);
    hio_matter_create_thread();
}
