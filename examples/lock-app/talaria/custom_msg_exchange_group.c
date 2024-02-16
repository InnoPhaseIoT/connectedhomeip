/**
  ******************************************************************************
  * @file   custom_group.c
  *
  * @brief  contains routines for custom groups.

  ******************************************************************************
  * @attention
  *
  *
  *  Copyright (c) 2018-2023, InnoPhase IoT, Inc.
  *
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  AND NONINFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
  *  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
  *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
#include "custom_msg_exchange_group.h"
#include <errno.h>
#include <kernel/hostio.h>
#include <kernel/os.h>
#include <string.h>

static inline struct packet *
alloc_custom_data_rsp(struct custom_msg_rsp **rsp)
{
    struct packet *pkt = packet_alloc(sizeof(struct hio_msghdr) + sizeof **rsp);
    if (pkt) {
        pfrag_reserve(packet_first_frag(pkt), sizeof(struct hio_msghdr));
        *rsp = packet_insert_tail(pkt, sizeof **rsp);
    }
    return pkt;
}

static struct packet *
custom_send_resp(void)
{
    char t2_rsp[64] = "Resp from T2";
    struct packet *pkt;
    struct custom_msg_rsp *rsp;
    pkt = OS_ERROR_ON_NULL(alloc_custom_data_rsp(&rsp));
    rsp->status = 0;
    memcpy(rsp->echo_rsp, t2_rsp, sizeof(t2_rsp));
    return pkt;
}

static struct packet *
custom_data_send_recieve(void *ctx, struct packet *msg)
{
    struct custom_msg_req *req = packet_data(msg);
    os_printf("host sent:%s\r\n", req->echo_req);
    return custom_send_resp();
}

static const struct hio_api custom_msg_exchange_api = { .group = 158,
    .num_handlers = 1,
    .handler = {
        custom_data_send_recieve,
    } };

void
custom_msg_exchange_api_init()
{
    os_printf("Registering custom_msg_exchange apis\n");
    hio_api_init(&custom_msg_exchange_api, NULL);
}