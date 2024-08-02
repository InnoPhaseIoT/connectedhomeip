/**
  ******************************************************************************
  * @file   main.c
  *
  * @brief  The example code which Talaria TWO publishes a text message to a
            MQTT Broker and subscribes to a topic and displays the received
            message over Talaria TWO’s console. The application can be
            evaluated in secure and non-secured mode of MQTT operations.

  ******************************************************************************
  * @attention
  *
  *
  *  Copyright (c) 2018-2024, InnoPhase IoT, Inc.
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
#include "wifi/wcm.h"
#include <kernel/os.h>

#include "ssl_wrap/inc/ssl_wrap.h"

#include "mqtt/include/mqtt.h"

#include "app_mqtt.h"

#include "osal.h"
#include "wifi_utils.h"

#define APP_THREAD_STACK_SIZE (8048 + 4096)
#define LEN_OF_MAC_ID 6

/* last will topics for connections */
#define MQTT_LWT_TOPIC_1 "will_con1"
#define MQTT_LWT_TOPIC_2 "will_con2"
/* LWT messages */
#define APP_LWT_MESSAGE_1 "Connection-1 Terminated"
#define APP_LWT_MESSAGE_2 "Connection-1 Terminated"
#define MQTT_TOPIC_LEN_MAX 40

//#define ALPN_ENABLE
#ifdef ALPN_ENABLE
#define MQTT_ALPN_STR "http/1.1" /* Just an example ALPN string */
#endif

enum { APP_EVT_MSG_TYPE = 6069 };

typedef enum {
    TCP,
    TLS,
    TLS_CLIENT_VERFIY,
    TLS_NO_CERT_VERIFY,
    WEBSOCK,
    SECURED_WEBSOCK,
    SECURED_WEBSOCK_NO_CERT_VERIFY
} mqtt_tranport_mode;

typedef enum {
    APP_MQTT_CONNECT = 1,
    APP_WIFI_DISCONNECTED,
    APP_MQTT_PUBLISH,
    APP_MQTT_YIELD
} APP_EVENTS;

struct app_evt_msg {
    int event;
};

typedef struct app_boot_args {
    const char *ssid;
    const char *passphrase;
    const char *cloud_url;
    uint16_t cloud_port;
    const char *cloud_usr_name;
    const char *cloud_usr_psw;
    const char *ca_cert;
    const char *client_cert;
    const char *client_key;
    const char *mqtt_client_id;
    int pub_qos;
    int sub_qos;
    const char *pub_topic;
    const char *pub_payload;
    const char *sub_topic;
    mqtt_tranport_mode transport_mode;
    int mqtt_no_poll; /**<0 - Normal poll mode
                          1 - No poll done for data. Demo of this feature is
                              done using two connections */
    int num_conn;
    const char *websock_url;
    const char *brokeraddr; /* host name */
    const char
        *mqtt_cmd_timeout; /* publish and subscribe response timeout in sec */
    int clean_session;
} app_boot_args_t;

QueueHandle_t mqttapp_xTaskQueue;
app_boot_args_t bargs;
struct wcm_handle *h;
bool wcm_connected = false;
app_mqtt_param_t mqtt_param_1;
app_mqtt_param_t mqtt_param_2;

app_mqtt_conn_t c1;
app_mqtt_conn_t c2;

TaskHandle_t app_thread = NULL;
int wifi_con_status = 0;

void
app_msg_send(int event)
{
    long task_yield_req;
    struct app_evt_msg *msg = (struct app_evt_msg *)osal_alloc(sizeof(*msg));
    assert(msg != NULL);
    msg->event = event;

    /* Write to the queue. */
    if (in_interrupt()) {
        xQueueSendFromISR(mqttapp_xTaskQueue, (void *)&msg, &task_yield_req);
        if (task_yield_req)
            portYIELD_FROM_ISR(task_yield_req);
    } else {
        xQueueSend(mqttapp_xTaskQueue, (void *)&msg, portMAX_DELAY);
    }
}

/*
 * Gets the Talaria TWO module's MAC id.
 * */
void
app_fetch_t2_macid(uint8_t *mac_id)
{
    const uint8_t *mac_addr = wcm_get_hwaddr(h);
    os_printf("mac id:");
    for (int index = 0; index < 6; index++) {
        mac_id[index] = *(mac_addr + index);
        os_printf("%x", mac_id[index]);
    }
}

/**
 * Auto Generate unique MQTT Client ID and mqtt topics.
 * Note !!: This is Strictly for Demo purpose
 * In actual scenario, use proper topics and client id
 */
static void
app_auto_generate_params(app_mqtt_param_t *m, int conn_num)
{
    uint8_t t2_mac_id[LEN_OF_MAC_ID];
    char buf[64];
    int index = 0;

    /* Get MAC ID */
    app_fetch_t2_macid(t2_mac_id);
    for (int i = 0; i < LEN_OF_MAC_ID; i++) {
        index += snprintf(&buf[index], 128 - index, "%x", t2_mac_id[i]);
    }

    /* The client ID, publish topic, pub payload and sub topic take effect
     * only if the conn_num = 1
     */
    if (0 == strlen(m->client_id) || bargs.num_conn > 1) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation="
        snprintf(
            m->client_id, MAX_MQTT_CLIENT_ID_LEN, "T2_%s_%d", buf, conn_num);
#pragma GCC diagnostic pop
    }
    if (0 == strlen(m->publish_topic) || bargs.num_conn > 1) {
        snprintf(m->publish_topic, MAX_TOPIC_LEN, "%s%s_%d", m->client_id,
            "/pt2", conn_num);
    }
    if (0 == strlen(m->pub_payload) || bargs.num_conn > 1) {
        snprintf(m->pub_payload, MAX_PUB_PAYLOAD_LEN, PUBLISH_MESSAGE);
    }
    if (0 == strlen(m->subscribe_topic) || bargs.num_conn > 1) {
        snprintf(m->subscribe_topic, MAX_TOPIC_LEN, "%s%s_%d", m->client_id,
            "/st2", conn_num);
    }

    m->mqtt_lwt_enable = 1;
    m->lwt_qos = QOS1;
    m->lwt_retain_enable = 0;
    if (conn_num == 1) {
        m->lwt_msg_len = strlen(APP_LWT_MESSAGE_1);
        strncpy(m->lwt_topic_name, MQTT_LWT_TOPIC_1, MAX_LWT_TOPIC_LEN);
        memcpy((uint8_t *)m->lwt_message, (uint8_t *)APP_LWT_MESSAGE_1,
            m->lwt_msg_len);
    } else {
        m->lwt_msg_len = strlen(APP_LWT_MESSAGE_2);
        strncpy(m->lwt_topic_name, MQTT_LWT_TOPIC_2, MAX_LWT_TOPIC_LEN);
        memcpy((uint8_t *)m->lwt_message, (uint8_t *)APP_LWT_MESSAGE_2,
            m->lwt_msg_len);
    }

    os_printf("\r\n------------------------------------------------------\r\n");
    os_printf("T2 MQTT Client id      : %s\r\n", m->client_id);
    os_printf("T2 MQTT publish topic  : %s\r\n", m->publish_topic);
    os_printf("T2 MQTT subscribe topic: %s\r\n", m->subscribe_topic);
    os_printf("T2 LWT topic           : %s\r\n", m->lwt_topic_name);
    os_printf("--------------------------------------------------------\r\n");
}

/**
 *Sets the transport mode based on the boot argument value
 */
static int
app_mqtt_params_set(app_mqtt_param_t *m)
{
    memset(m, 0, sizeof(app_mqtt_param_t));

    m->cloud_url = bargs.cloud_url;
    m->cloud_port = bargs.cloud_port;
    m->cloud_usr_name = bargs.cloud_usr_name;
    m->cloud_usr_psw = bargs.cloud_usr_psw;
    m->ca_cert = bargs.ca_cert;
    m->client_cert = bargs.client_cert;
    m->client_key = bargs.client_key;
    m->pub_qos = bargs.pub_qos;
    m->sub_qos = bargs.sub_qos;
    if (bargs.mqtt_client_id)
        strncpy(m->client_id, bargs.mqtt_client_id, MAX_MQTT_CLIENT_ID_LEN - 1);
    if (bargs.pub_topic)
        strncpy(m->publish_topic, bargs.pub_topic, MAX_TOPIC_LEN - 1);
    if (bargs.pub_payload)
        strncpy(m->pub_payload, bargs.pub_payload, MAX_PUB_PAYLOAD_LEN - 1);
    if (bargs.sub_topic)
        strncpy(m->subscribe_topic, bargs.sub_topic, MAX_TOPIC_LEN - 1);

    m->websock_url = bargs.websock_url;
    m->mqtt_cmd_timeout = bargs.mqtt_cmd_timeout;

    if (bargs.transport_mode == TCP) {
        m->transport = APP_MQTT_TM_TCP;
    } else if (bargs.transport_mode == TLS) {
        m->transport = APP_MQTT_TM_TLS;
    } else if (bargs.transport_mode == TLS_CLIENT_VERFIY) {
        m->transport = APP_MQTT_TM_TLS; /* With Client Authentication if the
                                         * Server forces it */
    } else if (bargs.transport_mode == TLS_NO_CERT_VERIFY) {
        m->transport = APP_MQTT_TM_TLS_NO_CERT_VERIFY;
    } else if (bargs.transport_mode == WEBSOCK) {
        m->transport = APP_MQTT_TM_WEBSOCK;
    } else if (bargs.transport_mode == SECURED_WEBSOCK) {
        m->transport = APP_MQTT_TM_SECURED_WEBSOCK;
    } else if (bargs.transport_mode == SECURED_WEBSOCK_NO_CERT_VERIFY) {
        m->transport = APP_MQTT_TM_SECURED_WEBSOCK_NO_CERT_VERIFY;
    }

    m->cfg.auth_mode = SSL_WRAP_VERIFY_NONE;
    if (m->transport == APP_MQTT_TM_TLS
        || m->transport == APP_MQTT_TM_SECURED_WEBSOCK) {
        if (m->ca_cert != NULL) {
            /* CA certificate */
            m->cfg.ca_cert.buf
                = utils_file_get(m->ca_cert, &m->cfg.ca_cert.len);
            if (m->cfg.ca_cert.buf == NULL) {
                os_printf("Provide a valid path for the CA certificate-1\r\n");
                return -2;
            }
        } else {
            os_printf("Provide a valid path for the CA certificate\r\n");
            return -2;
        }
        m->cfg.auth_mode = SSL_WRAP_VERIFY_REQUIRED;
    }
    if (m->client_cert != NULL) {
        /* Client certificate */
        m->cfg.client_cert.buf
            = utils_file_get(m->client_cert, &m->cfg.client_cert.len);
        if (m->cfg.client_cert.buf == NULL
            && bargs.transport_mode == TLS_CLIENT_VERFIY) {
            os_printf("Provide a valid path for client certificate\r\n");
            return -2;
        }
    } else if (bargs.transport_mode == TLS_CLIENT_VERFIY) {
        os_printf("Provide a valid path for client certificate\r\n");
        return -2;
    }
    if (m->client_key != NULL) {
        /* Client key */
        m->cfg.client_key.buf
            = utils_file_get(m->client_key, &m->cfg.client_key.len);
        if (m->cfg.client_key.buf == NULL
            && bargs.transport_mode == TLS_CLIENT_VERFIY) {
            os_printf("Provide a valid path for Client key\r\n");
            return -2;
        }
    } else if (bargs.transport_mode == TLS_CLIENT_VERFIY) {
        os_printf("Provide a valid path for Client key\r\n");
        return -2;
    }
#ifdef ALPN_ENABLE
    m->cfg.alpn_lst[0] = MQTT_ALPN_STR;
#endif
    return 0;
}

void
parse_boot_args(void)
{
    bargs.ssid = os_get_boot_arg_str("ssid");
    bargs.passphrase = os_get_boot_arg_str("passphrase");
    bargs.cloud_url = os_get_boot_arg_str("cloud_url");
    bargs.cloud_port = os_get_boot_arg_int("cloud_port", 1883);
    bargs.cloud_usr_name = os_get_boot_arg_str("cloud_usr_name");
    bargs.cloud_usr_psw = os_get_boot_arg_str("cloud_usr_psw");
    bargs.ca_cert = os_get_boot_arg_str("ca_cert");
    bargs.client_cert = os_get_boot_arg_str("client_cert");
    bargs.client_key = os_get_boot_arg_str("client_key");
    bargs.mqtt_client_id = os_get_boot_arg_str("mqtt_client_id");
    bargs.pub_qos = os_get_boot_arg_int("pub_qos", 1);
    bargs.sub_qos = os_get_boot_arg_int("sub_qos", 0);
    bargs.pub_topic = os_get_boot_arg_str("pub_topic");
    bargs.pub_payload = os_get_boot_arg_str("pub_payload");
    bargs.sub_topic = os_get_boot_arg_str("sub_topic");
    bargs.transport_mode = os_get_boot_arg_int("transport_mode", 0);
    bargs.mqtt_no_poll = os_get_boot_arg_int("mqtt_no_poll", 0);
    bargs.num_conn = os_get_boot_arg_int("num_conn", 1);
    bargs.websock_url = os_get_boot_arg_str("websock_url");
    bargs.clean_session = os_get_boot_arg_int("clean_session", 1);
}

struct callout timeout;
struct callout reconnection_timeout;

void
app_wifi_status_cb(void *ctx, struct os_msg *msg)
{
    os_printf("\n%s: status = %d", __FUNCTION__, msg->msg_type);
    if (NULL == app_thread)
        return; /* Still in the init phase */
    if (WCM_NOTIFY_MSG_CONNECTED == msg->msg_type) {
        app_msg_send(APP_MQTT_CONNECT);
    } else if (WCM_NOTIFY_MSG_DISCONNECT_DONE == msg->msg_type) {
        app_msg_send(APP_WIFI_DISCONNECTED);
        wifi_con_status = 1;
    }
}

void
app_time_out_cb(struct callout *co)
{
    app_msg_send(APP_MQTT_PUBLISH);
}

static void
app_setup_timeout(void)
{
    static int init_done = 0;
    if (init_done)
        return;
    init_done = 1;

    callout_init(&timeout, app_time_out_cb);
    /* Schedule a callback */
    callout_schedule(&timeout, SYSTIME_SEC(2));
}

void
app_mqtt_reconnect_tmo_cb()
{
    os_printf("\nconnection_retrying\n");
    app_msg_send(APP_MQTT_CONNECT);
}

/*
 * mqtt thread
 */
static void
app_thread_entry_fn(void *arg)
{
    int rval, ret;
    uint32_t ulValue, recv_status;
    struct app_evt_msg *app_msg;
    int event;
    /* Any one time init */
    if (bargs.mqtt_no_poll) {
        /* NO Polling for incoming packets */
        MQTTNoPollInit();
    }
    app_mqtt_params_set(&mqtt_param_1);
    app_auto_generate_params(&mqtt_param_1, 1);

    if (bargs.num_conn == 2) {
        mqtt_param_2 = mqtt_param_1;
        app_auto_generate_params(&mqtt_param_2, 2);
    }
    callout_init(&reconnection_timeout, app_mqtt_reconnect_tmo_cb);
    app_msg_send(APP_MQTT_CONNECT);

    while (1) {
        recv_status
            = xQueueReceive(mqttapp_xTaskQueue, &ulValue, portMAX_DELAY);
        if (0 == recv_status)
            continue;

        app_msg = (struct app_evt_msg *)ulValue;
        event = app_msg->event;
        osal_free(app_msg);

        switch (event) {
        case APP_MQTT_CONNECT:
            c1.clean_session = bargs.clean_session;
            /* MQTT connect */
            if (!c1.connected) {
                rval = app_mqtt_connect(&c1, &mqtt_param_1);
                if (rval < 0) {
                    callout_schedule(&reconnection_timeout, SYSTIME_SEC(60));
                    break;
                }
                callout_stop(&reconnection_timeout);
                /* MQTT Subscribe */
                app_subscribe(&c1, &mqtt_param_1);
            }

            /* MQTT connect - a second connection */
            if (bargs.num_conn == 2 && !c2.connected) {
                rval = app_mqtt_connect(&c2, &mqtt_param_2);
                if (rval < 0) {
                    callout_schedule(&reconnection_timeout, SYSTIME_SEC(60));
                    break;
                }
                callout_stop(&reconnection_timeout);
                /* MQTT Subscribe */
                app_subscribe(&c2, &mqtt_param_2);
            }
            app_setup_timeout();
            app_msg_send(APP_MQTT_PUBLISH);
            break;
        case APP_WIFI_DISCONNECTED:
            if (bargs.mqtt_no_poll) {
                callout_stop(&timeout);
            }
            if (c1.connected)
                app_mqtt_close(&c1);
            if (c2.connected)
                app_mqtt_close(&c2);
            break;
        case APP_MQTT_PUBLISH:
            /* MQTT Publish */
            if (c1.connected) {
                ret = app_mqtt_publish(
                    &c1, &mqtt_param_1, mqtt_param_1.pub_payload);
                if ((ret < 0) && (!wifi_con_status)) {
                    os_printf("%s - %d\n", __FUNCTION__, __LINE__);
                    app_mqtt_close(&c1);
                    app_msg_send(APP_MQTT_CONNECT);
                }
            }
            if (c2.connected) {
                ret = app_mqtt_publish(
                    &c2, &mqtt_param_2, mqtt_param_2.pub_payload);
                if ((ret < 0) && (!wifi_con_status)) {
                    os_printf("%s - %d\n", __FUNCTION__, __LINE__);
                    app_mqtt_close(&c2);
                    app_msg_send(APP_MQTT_CONNECT);
                }
            }
            /* break; */
        case APP_MQTT_YIELD:
            if (!bargs.mqtt_no_poll) {
                /* Call MQTT Yield periodically */
                if (c1.connected) {
                    if (MQTTYield(c1.mqtt_client, 500) != 0) {
                        os_printf("\nError: Connection(1) Closed");
                        app_mqtt_close(&c1);
                    }
                }
                if (c2.connected) {
                    if (MQTTYield(c2.mqtt_client, 500) != 0) {
                        os_printf("\nError: Connection(2) Closed");
                        app_mqtt_close(&c2);
                    }
                }
            }
            callout_schedule(&timeout, SYSTIME_SEC(2));
            break;
        default:
            break;
        }
    }
}

/* Entry point */
int
mqtt_main(void)
{
    int rval;

    os_printf("MQTT Example App starts...\n");
    /* Parse boot arguments */
    parse_boot_args();

    /* Get wcm handle */
    h = get_wcm_handle_for_additional_app();
    if (h == NULL) {
	os_printf("\nError: Unable to get the wcm handle\n");
	wcm_connected = false;
    }
    else {
	wcm_connected = true;
    }

    wifi_status_cb_set(NULL, app_wifi_status_cb);

    mqttapp_xTaskQueue = xQueueCreate(4, (UBaseType_t)sizeof(uint32_t));

    /* Create a thread to handle MQTT connect/Subscribe/Publish */
    xTaskCreate(
        app_thread_entry_fn, /* The function that implements the task. */
        "app_thread", /* The text name assigned to the task - for debug only as
                       * it is not used by the kernel. */
        APP_THREAD_STACK_SIZE
            / 4, /* The size of the stack to allocate to the task. */
        NULL, /* The parameter passed to the task - not used in this case. */
        (tskIDLE_PRIORITY + 2), /* The priority assigned to the task. */
        &app_thread);

    if (NULL == app_thread) {
        return -1;
    }

    vTaskSuspend(NULL);
    return 0;
}
