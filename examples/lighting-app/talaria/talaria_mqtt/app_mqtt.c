/**
 ******************************************************************************
 * @file   app_mqtt.c
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
#include "lwip/netdb.h"
#include <kernel/os.h>
#include <lwip/ip_addr.h>
#include <lwip/netif.h>
#include <lwip/sockets.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ssl_wrap/inc/ssl_wrap.h"

#include "mqtt/include/mqtt.h"

#include "app_mqtt.h"

#include "utils.h"
#include "utils/inc/fs_utils.h"
#include "utils/inc/utils.h"

#define APP_MQTT_SEND_BUF_SIZE 1024
#define APP_MQTT_READ_BUF_SIZE 1024

/* Default configs */
#define APP_MQTT_DEFAULT_KA_INTR 60
#define APP_MQTT_DEFAULT_CLEAN_SESSION 1

void
app_mqtt_nw_disconnect(app_mqtt_conn_t *cn)
{
    /* Disconnect the transport layer connection i.e TCP/SSL/Websock connection
     */
    if (cn->transport == APP_MQTT_TM_TCP) {
        MQTTNetworkDisconnect(cn->mqtt_network);
        os_printf("MQTT network disconnected\r\n");
    } else if (cn->transport == APP_MQTT_TM_TLS
        || cn->transport == APP_MQTT_TM_TLS_NO_CERT_VERIFY) {
        MQTTNetworkDisconnect_Tls(cn->mqtt_network);
        os_printf("MQTT network Tls disconnected\r\n");
    } else if (cn->transport == APP_MQTT_TM_WEBSOCK
        || cn->transport == APP_MQTT_TM_SECURED_WEBSOCK
        || cn->transport == APP_MQTT_TM_SECURED_WEBSOCK_NO_CERT_VERIFY) {
        /* Network disconnect */
        MQTTNetworkDisconnect_Ws(cn->mqtt_network);
        os_printf("MQTT network Websocket disconnected\r\n");
    }
}

/**
 *@brief: Closes the MQTT connection.
 *@param: None.
 *@retval:None.
 *
 */
void
app_mqtt_close(app_mqtt_conn_t *cn)
{
    /* MQTT disconnect */
    MQTTDisconnect(cn->mqtt_client);
    app_mqtt_nw_disconnect(cn);
    cn->connected = 0;

    vPortFree(cn->sendbuf);
    vPortFree(cn->readbuf);
    vPortFree(cn->mqtt_network);
    vPortFree(cn->mqtt_client);
    memset(cn, 0, sizeof(app_mqtt_conn_t));
}

/**
 * @brief:Allocates the datastructures and buffers for a MQTT connection
 * @param: None.
 * @retval 0 if successfully allocated the buffers.
 * @retval -1 if failed to allocate the buffes.
 */
int
app_mqtt_conn_init(app_mqtt_conn_t *c)
{
    os_printf("\n%s", __FUNCTION__);
    if (!c->sendbuf) {
        c->sendbuf = pvPortMalloc(APP_MQTT_SEND_BUF_SIZE);
        if (NULL == c->sendbuf)
            goto exit_error;
    }
    if (!c->readbuf) {
        c->readbuf = pvPortMalloc(APP_MQTT_READ_BUF_SIZE);
        if (NULL == c->readbuf)
            goto exit_error;
    }
    if (!c->mqtt_network) {
        c->mqtt_network = pvPortMalloc(sizeof(MQTTNetwork));
        if (NULL == c->mqtt_network) {
            os_printf("\nMalloc Fail @%s:%d", __FUNCTION__, __LINE__);
            goto exit_error;
        }
    }
    memset(c->mqtt_network, 0, sizeof(MQTTNetwork));

    if (!c->mqtt_client) {
        c->mqtt_client = pvPortMalloc(sizeof(MQTTClient));
        if (NULL == c->mqtt_client) {
            os_printf("\nMalloc Fail @%s:%d", __FUNCTION__, __LINE__);
            goto exit_error;
        }
    }
    memset(c->mqtt_client, 0, sizeof(MQTTClient));
    return 0;

exit_error:

    vPortFree(c->sendbuf);
    vPortFree(c->readbuf);
    vPortFree(c->mqtt_network);
    vPortFree(c->mqtt_client);

    return -1;
}

/**
 *@brief: Performs the MQTT connect and Subscribe
 *@param: struct param_t param - Structure containing the mqtt connection
 *parameters.
 *@retval: 0 - If successfully connected.
 *        -1 - If failed to connect.
 *
 */
int
app_mqtt_connect(app_mqtt_conn_t *cn, struct app_mqtt_param_t *m)
{
    int ret = 1;

    os_printf("\n%s %d", __FUNCTION__, __LINE__);
    if (app_mqtt_conn_init(cn) < 0) {
        goto exit;
    }
    /* MQTT N/w connect, based on the transport */
    if (m->transport == APP_MQTT_TM_TCP) {
        /* Non secured MQTT */
        MQTTNetworkInit(cn->mqtt_network);
        ret = MQTTNetworkConnect(
            cn->mqtt_network, (char *)m->cloud_url, m->cloud_port);
        if (ret != 0) {
            os_printf("NetworkConnect = %d\n", ret);
            goto exit;
        }
    } else if ((m->transport == APP_MQTT_TM_TLS)
        || (m->transport == APP_MQTT_TM_TLS_NO_CERT_VERIFY)) {
        /* Secured MQTT */
        MQTTNetworkInit_Tls(cn->mqtt_network);
        ret = MQTTNetworkConnect_Tls(
            cn->mqtt_network, (char *)m->cloud_url, m->cloud_port, &m->cfg);
        if (ret < 0) {
            os_printf("\r\nmqtt_connect_tls %d !!", ret);
            goto exit;
        }
    } else if (m->transport == APP_MQTT_TM_WEBSOCK
        || m->transport == APP_MQTT_TM_SECURED_WEBSOCK
        || m->transport == APP_MQTT_TM_SECURED_WEBSOCK_NO_CERT_VERIFY) {
        websock_config_t ws_cfg;
        /* Init mqtt */
        MQTTNetworkInit_Ws(cn->mqtt_network);
        /* Connect to broker over websocket */
        memset(&ws_cfg, 0, sizeof(ws_cfg));
        os_printf("\nmqttbroker_address = %s", m->cloud_url);
        ws_cfg.hostname = m->cloud_url;
        ws_cfg.uri = (char *)m->websock_url;
        ws_cfg.port = m->cloud_port;
        ws_cfg.time_out = 300;
        ws_cfg.secured = (m->transport == APP_MQTT_TM_WEBSOCK) ? 0 : 1;
        memcpy((char *)&ws_cfg.ssl_config, (const char *)&m->cfg,
            sizeof((m->cfg)));
        ret = MQTTNetworkConnect_Ws(cn->mqtt_network, &ws_cfg);

        if (ret < 0) {
            os_printf("\r\nmqtt_connect_ws %d ", ret);
            goto exit;
        }
    }
    /* Network level (TCP/TLS/WebSock) connection done
     * Need to do MQTT CONNECT
     */
    cn->transport = m->transport;
    /* 1. Configure the client connection with parameters like timeout and
     *    buffers
     * 2. Do MQTT connect. (CONN, CONACK)
     */
    MQTTClientInit(cn->mqtt_client, cn->mqtt_network, 15 * 1000, cn->sendbuf,
        APP_MQTT_SEND_BUF_SIZE, cn->readbuf, APP_MQTT_READ_BUF_SIZE);
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.willFlag = 0;
    data.willFlag = m->mqtt_lwt_enable;
    if (data.willFlag) {
        data.will.qos = m->lwt_qos;
        data.will.struct_version = 3;
        data.will.retained = m->lwt_retain_enable;
        data.will.topicName.lenstring.len = strlen(m->lwt_topic_name);
        data.will.topicName.lenstring.data = m->lwt_topic_name;
        data.will.message.lenstring.len = m->lwt_msg_len;
        data.will.message.lenstring.data = m->lwt_message;
    }
    data.MQTTVersion = 3;
    data.clientID.cstring = (char *)m->client_id;
    data.username.cstring = (char *)m->cloud_usr_name;
    data.password.cstring = (char *)m->cloud_usr_psw;
    data.keepAliveInterval = APP_MQTT_DEFAULT_KA_INTR;
    data.cleansession = APP_MQTT_DEFAULT_CLEAN_SESSION;
    data.kaRespTimeout = 0;

    os_printf("\r\nConnecting ...\n");
    ret = MQTTConnect(cn->mqtt_client, &data);
    if (0 == ret) {
        cn->connected = 1;
        os_printf("\nMQTTConnect Success. ret = %d", ret);
    } else {
        os_printf("\nMQTTConnect Failed. ret = %d", ret);
        /* Disconnect TCP/TLS/Websock connection */
        app_mqtt_nw_disconnect(cn);
    }

    return 0;
exit:
    return -1;
}

/**
 *@brief: Publishes the  MQTT messages
 *@param: char *pmessage: Address of the buffer containing the message.
 *@param: int len: Length of the mesage to publish
 *@retval: 0 - If successfully published.
 *        -1 - If failed to publish.
 *
 */
int
app_mqtt_publish(app_mqtt_conn_t *cn, app_mqtt_param_t *m, char *payload)
{
    int rc;
    MQTTMessage message;
    static int s_cnt = 0, f_cnt = 0;

    message.payload = payload;
    message.payloadlen = strlen(payload);
    message.qos = m->pub_qos;
    message.retained = 0;

    rc = MQTTPublish(cn->mqtt_client, m->publish_topic, &message);
    if (rc != 0) {
        os_printf("\nMQTTPublish failed. Ret= %d", rc);
        f_cnt++;
        return -1;
    } else {
        os_printf("\nMessage Published Successfully");
        s_cnt++;
    }
    os_printf("\nPublish stats: Success = %d, Failure = %d", s_cnt, f_cnt);
    return 0;
}

/* MQTT Subscriber call back */
void
app_mqtt_subscribe_cb(MessageData *Msg)
{
    int i;

    os_printf("\nMQTTSubscribe Call back");
    if (Msg->topicName->cstring) {
        os_printf("\n\ttopic = %s", Msg->topicName->cstring);
    } else {
        os_printf("\n\ttopic = ");
        for (i = 0; i < Msg->topicName->lenstring.len; i++)
            os_printf("%c", Msg->topicName->lenstring.data[i]);
        os_printf("\n");
    }
    os_printf("\n\tMessage = ");
    char *p = Msg->message->payload;
    for (i = 0; i < Msg->message->payloadlen; i++)
        os_printf("%c", p[i]);
    os_printf("\n");
}

/**
 *@brief: Subscribes to a topic.
 *@param: mqttsub_param *sub_param : Pointer to the buffer of the structure
 *containing the mqtt subscribe parameters.
 *@retval : 0 Success
 *
 */
int
app_subscribe(app_mqtt_conn_t *cn, app_mqtt_param_t *m)
{
    os_printf("\n%s:%d", __FUNCTION__, __LINE__);
    MQTTSubscribe(
        cn->mqtt_client, m->subscribe_topic, m->sub_qos, app_mqtt_subscribe_cb);

    os_printf("\n%s: %d", __FUNCTION__, __LINE__);
    return 0;
}

/**
 *@brief: Checks for an active MQTT connection.
 *@param: None.
 *@retval 0: Connection is not active.
 *        1: Connection is active.
 */

int
app_mqtt_conn_status(app_mqtt_conn_t *cn)
{
    if (cn->mqtt_client->isconnected == 1) {
        return 1;
    }
    return 0;
}
