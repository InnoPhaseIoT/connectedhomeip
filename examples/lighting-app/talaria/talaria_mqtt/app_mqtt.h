/**
  ******************************************************************************
  * @file   app_mqtt.h
  *
  * @brief  The example code definitions
  *
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
#define MAX_TOPIC_LEN 64     
#define MAX_MQTT_CLIENT_ID_LEN 32
#define MAX_LWT_MSG_LEN 32
#define MAX_LWT_TOPIC_LEN 32	

#define MAX_PUB_PAYLOAD_LEN 128

#define PUBLISH_MESSAGE "Hello From T2"
//#define PUBLISH_MESSAGE "{\"from\":\"Talaria T2\",\"to\":\"AWS\",\"msg\":\"Howdy Ho\",\"msg_id\":1}"


typedef enum{
    APP_MQTT_TM_TCP,
    APP_MQTT_TM_TLS,
    APP_MQTT_TM_TLS_NO_CERT_VERIFY,
    APP_MQTT_TM_WEBSOCK,
    APP_MQTT_TM_SECURED_WEBSOCK,
    APP_MQTT_TM_SECURED_WEBSOCK_NO_CERT_VERIFY
}mqtt_tranport_t;

typedef struct app_mqtt_param_t{    
    const char *cloud_url;
    uint16_t cloud_port;
    const char *cloud_usr_name;
    const char *cloud_usr_psw;
    const char *ca_cert;
    const char *client_cert;
    const char *client_key;
    mqtt_tranport_t transport;
    char client_id[MAX_MQTT_CLIENT_ID_LEN];
    char subscribe_topic[MAX_TOPIC_LEN];
    char publish_topic[MAX_TOPIC_LEN];
    /* If the payload is internal buffer, directly, pass the buffer pointer to
       app_mqtt_publish(), in main.c*/
    char pub_payload[MAX_PUB_PAYLOAD_LEN];
    int pub_qos;
    int sub_qos;
    int ka_interval;
    int clean_session;
    char lwt_message[MAX_LWT_MSG_LEN];
    uint16_t lwt_msg_len;
    uint8_t lwt_qos;
    uint8_t lwt_retain_enable;
    char lwt_topic_name[MAX_LWT_TOPIC_LEN];
    uint8_t mqtt_lwt_enable;
    ssl_wrap_cfg_t cfg;
    const char *websock_url;
    const char *brokeraddr; /*host name*/
    const char *mqtt_cmd_timeout;/*publish and subscribe response timeout in sec*/
}app_mqtt_param_t;

typedef struct app_mqtt_conn{
    MQTTNetwork *mqtt_network;
    MQTTClient *mqtt_client;
    unsigned char *sendbuf;
    unsigned char *readbuf;
    int transport;
    int connected;
    int clean_session;
}app_mqtt_conn_t;

int app_mqtt_init(app_mqtt_conn_t *c);
void app_mqtt_close(app_mqtt_conn_t *cn);
int app_mqtt_connect(app_mqtt_conn_t *cn, struct app_mqtt_param_t *m);
int app_mqtt_publish(app_mqtt_conn_t *cn, app_mqtt_param_t *m,
                             char *payload);
int app_subscribe(app_mqtt_conn_t *cn, app_mqtt_param_t *m);
int app_mqtt_conn_status(app_mqtt_conn_t *cn);
