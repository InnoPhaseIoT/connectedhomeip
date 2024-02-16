/**
  ******************************************************************************
  * @file   custom_group.h
  *
  * @brief  contains structure definitions for custom groups.

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
#ifndef CUSTOM_GROUP
#define CUSTOM_GROUP

#include <stdint.h>

#define MAX_CUSTOM_MSG_SIZE 64

#define HIO_GROUP_CUSTOM_MSG_EXCHANGE 158
#define CUSTOM_MSG_REQ 0x00
#define CUSTOM_MSG_RSP 0x80

struct custom_msg_req
{
    char echo_req[MAX_CUSTOM_MSG_SIZE]; /*Request message from host*/
};

struct custom_msg_rsp
{
    uint32_t status;                    /**< result status, zero is success */
    char echo_rsp[MAX_CUSTOM_MSG_SIZE]; /**< response from T2 */
};

#ifdef __cplusplus
extern "C" {
#endif

void custom_msg_exchange_api_init();

#ifdef __cplusplus
}
#endif

#endif
