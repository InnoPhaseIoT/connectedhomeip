 /**
  ******************************************************************************
  * @file   matter_hio.h
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

#include<stdio.h>
#include <string.h>
#include <kernel/hostio.h>

/*
[[[cog
import cog, generate
cog.outl('')
generate.header(generate.Emitter(cog.out), 'matter')
cog.outl('')
]]] */

#define HIO_GROUP_MATTER 95

/*
 * Message data_send
 */

#define MATTER_DATA_SEND_REQ 0x00
#define MATTER_DATA_SEND_RSP 0x80

/** matter status sending. Request structure.
 */
struct matter_data_send_req {
    uint32_t cluster; /**< cluster */
    uint32_t cmd; /**< cmd */
    uint32_t payload_len; /**< payload_len */
    char data[0]; /**< data */
};

static inline struct packet *
alloc_matter_data_send_req(struct matter_data_send_req **req)
{
    struct packet *pkt = packet_alloc(sizeof(struct hio_msghdr) + sizeof **req);
    if (pkt) {
        pfrag_reserve(packet_first_frag(pkt), sizeof(struct hio_msghdr));
        *req = packet_insert_tail(pkt, sizeof **req);
    }
    return pkt;
}

static inline struct packet *
create_matter_data_send_req(const uint32_t cluster, const uint32_t cmd, const uint32_t payload_len, const char data[0])
{
    struct matter_data_send_req *req;
    struct packet *pkt = alloc_matter_data_send_req(&req);
    if (pkt) {
        req->cluster = cluster;
        req->cmd = cmd;
        req->payload_len = payload_len;
        strncpy(req->data, data, 0);
    }
    return pkt;
}

/** matter status sending. Response structure.
 */
struct matter_data_send_rsp {
    int32_t status; /**< result status, zero is success */
};

static inline struct packet *
alloc_matter_data_send_rsp(struct matter_data_send_rsp **rsp)
{
    struct packet *pkt = packet_alloc(sizeof(struct hio_msghdr) + sizeof **rsp);
    if (pkt) {
        pfrag_reserve(packet_first_frag(pkt), sizeof(struct hio_msghdr));
        *rsp = packet_insert_tail(pkt, sizeof **rsp);
    }
    return pkt;
}

static inline struct packet *
create_matter_data_send_rsp(const int32_t status)
{
    struct matter_data_send_rsp *rsp;
    struct packet *pkt = alloc_matter_data_send_rsp(&rsp);
    if (pkt) {
        rsp->status = status;
    }
    return pkt;
}
/*
 * Message cmd_notify
 */

#define MATTER_CMD_NOTIFY_IND 0xc1

/** matter cmd notification. Indication structure.
 */
struct matter_cmd_notify_ind {
    uint32_t cluster; /**< cluster */
    uint32_t cmd; /**< cmd */
    uint32_t payload_len; /**< payload_len */
    char data[0]; /**< data */
};

static inline struct packet *
alloc_matter_cmd_notify_ind(struct matter_cmd_notify_ind **ind)
{
    struct packet *pkt = packet_alloc(sizeof(struct hio_msghdr) + sizeof **ind);
    if (pkt) {
        pfrag_reserve(packet_first_frag(pkt), sizeof(struct hio_msghdr));
        *ind = packet_insert_tail(pkt, sizeof **ind);
    }
    return pkt;
}

static inline struct packet *
create_matter_cmd_notify_ind(const uint32_t cluster, const uint32_t cmd, const uint32_t payload_len, const char data[0])
{
    struct matter_cmd_notify_ind *ind;
    struct packet *pkt = alloc_matter_cmd_notify_ind(&ind);
    if (pkt) {
        ind->cluster = cluster;
        ind->cmd = cmd;
        ind->payload_len = payload_len;
        strncpy(ind->data, data, 0);
    }
    return pkt;
}

/* [[[end]]] */
