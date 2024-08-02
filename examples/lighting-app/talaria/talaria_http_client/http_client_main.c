/**
 *****************************************************************************
 * @file   main.c
 *
 * @brief  This file is part of the example application that demostartes the
 * use of HTTP Client module APIs provided by T2 SDK. This showcases the
 * http_client capabilities Using this application, one can quickly check
 * the http cpabilities of the Talaria module
 *****************************************************************************
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
 *****************************************************************************
 */

/*
 *
 *
 * main.c
 *
 * This is an example application that uses the http module(component)
 * and showcases its usage. This showcases the http_client capabilities
 * Using this application, one can quickly check the http cpabilities of the
 * Talaria module
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <kernel/os.h>
#include <lwip/ip4_addr.h>
#include <lwip/ip6_addr.h>
#include <lwip/ip_addr.h>
#include <lwip/netif.h>
#include <wifi/wcm.h>

#include "http/inc/http_client.h"
#include "mbedtls/certs.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "utils/inc/utils.h"
#include "utils/inc/wifi_utils.h"

#include "osal.h"

#ifdef __cplusplus
}
#endif

#define APP_NAME "HTTP  application"
#define APP_VERSION "2.0"

OS_APPINFO { .stack_size = 4096 };

#define INPUT_PARAMETER_HOST "host"
#define INPUT_PARAMETER_URL "url"
#define INPUT_PARAMETER_PATH "path"
#define INPUT_PARAMETER_PORT "port"
#define INPUT_PARAMETER_SECURED "secured"
#define INPUT_PARAMETER_METHOD "method"
#define INPUT_PARAMETER_CA "ca_cert"
#define INPUT_PARAMETER_POST_LEN "post_len"
#define INPUT_PARAMETER_CLIENT_CERT "client_cert"
#define INPUT_PARAMETER_CLIENT_KEY "client_key"

#define INPUT_PARAMETER_HDR1_NAME "hdr1_name"
#define INPUT_PARAMETER_HDR1_VAL "hdr1_val"
#define INPUT_PARAMETER_HDR2_NAME "hdr2_name"
#define INPUT_PARAMETER_HDR2_VAL "hdr2_val"
#define INPUT_PARAMETER_HDR3_NAME "hdr3_name"
#define INPUT_PARAMETER_HDR3_VAL "hdr3_val"
#define INPUT_PARAMETER_TEST_ITER "test_iterations"
#define INPUT_PARAMETER_USE_CA_BUNDLE "use_ca_bundle"
#define INPUT_PARAMETER_POST_DATA "post_data"
#define INPUT_PARAMETER_POST_DATA_FILE "post_data_file"

#define NULL_STR ""

#define NON_SECURED 0
#define SECURE_NO_CERT_VERIFY 1
#define SECURE_WITH_CERT_VERIFY 2

struct param_t {
    const char *url;
    const char *host;
    const char *path;
    const char *port;
    const char *secured;
    const char *method;
    const char *ca_cert;
    const char *post_len;
    const char *client_cert;
    const char *client_key;

    const char *hdr1_name;
    const char *hdr1_val;
    const char *hdr2_name;
    const char *hdr2_val;
    const char *hdr3_name;
    const char *hdr3_val;

    const char *test_iterations;
    const char *use_ca_bundle;
    const char *post_data;
    const char *post_data_file;
};

/* CA certificate bundle */
extern uint8_t ca_bundle_start[] asm("_binary_ca_bundle_start");
extern uint8_t ca_bundle_end[] asm("_binary_ca_bundle_end");

struct param_t param;
char default_port[8];
char default_secured[8];
char default_post_len[8];

static char host[128];
static char path[128];

static int
parse_boot_args(void)
{
    int ret = 0;
    sprintf(default_port, "%d", 80);
    sprintf(default_secured, "%d", 0);
    sprintf(default_post_len, "%d", 32);

    param.url = os_get_boot_arg_str(INPUT_PARAMETER_URL);
    param.host = os_get_boot_arg_str(INPUT_PARAMETER_HOST);
    param.path = os_get_boot_arg_str(INPUT_PARAMETER_PATH);
    param.port = os_get_boot_arg_str(INPUT_PARAMETER_PORT);
    param.secured = os_get_boot_arg_str(INPUT_PARAMETER_SECURED);
    param.ca_cert = os_get_boot_arg_str(INPUT_PARAMETER_CA);
    param.method = os_get_boot_arg_str(INPUT_PARAMETER_METHOD);
    param.post_len = os_get_boot_arg_str(INPUT_PARAMETER_POST_LEN);
    param.client_cert = os_get_boot_arg_str(INPUT_PARAMETER_CLIENT_CERT);
    param.client_key = os_get_boot_arg_str(INPUT_PARAMETER_CLIENT_KEY);
    /* Headers */
    param.hdr1_name = os_get_boot_arg_str(INPUT_PARAMETER_HDR1_NAME);
    param.hdr1_val = os_get_boot_arg_str(INPUT_PARAMETER_HDR1_VAL);
    param.hdr2_name = os_get_boot_arg_str(INPUT_PARAMETER_HDR2_NAME);
    param.hdr2_val = os_get_boot_arg_str(INPUT_PARAMETER_HDR2_VAL);
    param.hdr3_name = os_get_boot_arg_str(INPUT_PARAMETER_HDR3_NAME);
    param.hdr3_val = os_get_boot_arg_str(INPUT_PARAMETER_HDR3_VAL);

    param.test_iterations = os_get_boot_arg_str(INPUT_PARAMETER_TEST_ITER);
    param.use_ca_bundle = os_get_boot_arg_str(INPUT_PARAMETER_USE_CA_BUNDLE);
    param.post_data = os_get_boot_arg_str(INPUT_PARAMETER_POST_DATA);
    param.post_data_file = os_get_boot_arg_str(INPUT_PARAMETER_POST_DATA_FILE);

    os_printf("\n[APP]Bootparams :\n--------------------");
    os_printf("\nurl=%s\nhost= %s\nport=%s\npath= %s\n"
              "secured= %s\nmethod= %s\nca_cert=%s\npost_len=%s"
              "\ntest_iterations = %s\nuse_ca_bundle = %s",
        param.url, param.host, param.port, param.path, param.secured,
        param.method, param.ca_cert, param.post_len, param.test_iterations,
        param.use_ca_bundle);
    os_printf("\nhdr1_name= %s\thdr1_val= %s\nhdr2_name= %s\thdr2_val= %s"
              "\nhdr3_name= %s\thdr3_val= %s\n",
        param.hdr1_name, param.hdr1_val, param.hdr2_name, param.hdr2_val,
        param.hdr3_name, param.hdr3_val);
    os_printf("\npost_data= %s\npost_data_file= %s", param.post_data,
        param.post_data_file);
    os_printf("\n[APP]Bootparams end here....\n");

    if (!param.url && !param.host) {
        os_printf("\n[APP]Error: No URL or host provided");
        ret = -1;
        goto exit;
    }
    if (!param.method) {
        os_printf("\n[APP]Error: method missing");
        ret = -1;
        goto exit;
    }
    param.secured = (param.secured) ? param.secured : default_secured;
    if (SECURE_WITH_CERT_VERIFY == atoi(param.secured) && !param.ca_cert) {
        /* secured with server validation must */
        if (!atoi(param.use_ca_bundle)) {
            os_printf("\n[APP]Error: CA certificate missing");
            ret = -1;
            goto exit;
        }
    }
    param.post_len = (param.post_len) ? param.post_len : default_post_len;
exit:
    os_printf("\n[APP]Bootparams check done....ret = %d\n", ret);
    return ret;
}

static void
app_http_cb(void *ctx, http_client_resp_info_t *resp)
{
    static int total_bytes_rcvd = 0;
    static int hdrs_printed = 0;
    int i;
    if (NULL == resp) {
        return;
    }
    if (!hdrs_printed) {
        os_printf(
            "\n\n[APP]Response:\n%d ----------------------\n", resp->resp_len);
        os_printf("\n%d", resp->status_code);
        i = 0;
        while (resp->resp_hdrs[i]) {
            vTaskDelay(10);
            os_printf("\n%s", resp->resp_hdrs[i]);
            i++;
        }
        os_printf("\n[APP]Body:\n");
        hdrs_printed = 1;
    }
    total_bytes_rcvd += resp->resp_len;
    for (i = 0; i < resp->resp_len; i++) {
        os_printf("%c", resp->resp_body[i]);
    }
    return;
}

int
http_client_main(void)
{
    int rval = 0;
    int test_iterations = 1;
    http_client_config_t cfg = { 0 };
    http_client_handle_t http_handle;

    os_printf("Http Client Demo App starts...\n");

    if (parse_boot_args()) {
        goto exit;
    }

    /* Connect to HTTP server */

    memset(&cfg, 0, sizeof(http_client_config_t));
    path[0] = '\0';
    if (param.url) {
        os_printf("\n[APP]URL = %s", param.url);
        rval = http_client_url_to_host(
            param.url, host, sizeof(host), path, sizeof(path), &cfg.port);
        if (rval < 0) {
            os_printf("\n[APP]URL is not proper");
            os_printf("\n\texample URLs:");
            os_printf("\n\t\thttps://www.google.com/");
            os_printf("\n\t\thttps://en.wikipedia.org/wiki/URL");
            os_printf("\n\t\thttp://httpbin.org/json");
        }
        cfg.hostname = host;
    } else {
        cfg.hostname = (char *)param.host;
    }
    if (param.port) { /* If specified explicietly, overide the port specified in
                       * URL */
        cfg.port = atoi(param.port);
    }
    cfg.secured = atoi(param.secured);
    if (cfg.secured) {
        if (cfg.secured == SECURE_NO_CERT_VERIFY) {
            cfg.ssl_cfg.auth_mode = SSL_WRAP_VERIFY_NONE;
        } else {
            cfg.ssl_cfg.auth_mode = SSL_WRAP_VERIFY_REQUIRED;
            if (!param.use_ca_bundle || !atoi(param.use_ca_bundle)) {
                cfg.ssl_cfg.ca_cert.buf
                    = utils_file_get(param.ca_cert, &cfg.ssl_cfg.ca_cert.len);
                if (NULL == cfg.ssl_cfg.ca_cert.buf) {
                    os_printf("Error: No CA certificate found. Required");
                    goto exit;
                }
            } else if (atoi(param.use_ca_bundle)) {
                /* If  use_ca_bundle is set, initialise CA bundle */
                ssl_wrap_crt_bundle_init((const char *)ca_bundle_start);
            }
        }
        if (param.client_cert && strlen(param.client_cert)) {
            cfg.ssl_cfg.client_cert.buf = utils_file_get(
                param.client_cert, &cfg.ssl_cfg.client_cert.len);
            if (NULL == cfg.ssl_cfg.client_cert.buf) {
                os_printf("Error: Could not open client certificate\n");
                goto exit;
            }
        }
        if (param.client_key && strlen(param.client_key)) {
            cfg.ssl_cfg.client_key.buf
                = utils_file_get(param.client_key, &cfg.ssl_cfg.client_key.len);
            if (NULL == cfg.ssl_cfg.client_key.buf) {
                os_printf("Error: Could not open client key\n");
                goto exit;
            }
        }
        cfg.secured = SECURE_NO_CERT_VERIFY;
    }
    if (NULL != param.test_iterations) {
        test_iterations = atoi(param.test_iterations);
        if (0 == test_iterations)
            test_iterations = 1;
    }
    os_printf("\n** Test Iterations = %d **\n", test_iterations);
again:
    /* Connect to HTTP server */
    os_printf(
        "\n[APP]Calling http_client_open(). heap size = %d", os_avail_heap());

    http_handle = http_client_open(&cfg);
    if (NULL == http_handle) {
        os_printf("\n[APP]Error: HTTP connection failed");
        goto exit;
    }
    os_printf("\n[APP]Succes: HTTP connection done");
    /* Get the latest config file from remote server */
    http_client_set_req_hdr(http_handle, "Host", cfg.hostname);
    http_client_set_req_hdr(http_handle, param.hdr1_name, param.hdr1_val);
    http_client_set_req_hdr(http_handle, param.hdr2_name, param.hdr2_val);
    http_client_set_req_hdr(http_handle, param.hdr3_name, param.hdr3_val);
    if (strlen(path))
        param.path = path;
    if (!strcmp(param.method, "get")) {
        /* HTTP get */
        os_printf("\n[APP]HTTP Get");
        rval = http_client_get(
            http_handle, (char *)param.path, app_http_cb, NULL, 300);
        if (rval < 0) {
            os_printf("\n[APP]Failure : http_client_get(), rval = %d", rval);
        } else {
            os_printf("\n[APP]Success: http_client_get(), rval = %d", rval);
        }

        if (rval < 0 || rval == HTTP_CLIENT_CLOSE_CONNECTION) {
            http_client_close(http_handle);
            http_handle = NULL;
        }

        test_iterations--;
        if (test_iterations) {
            http_client_close(http_handle);
            goto again;
        }
    }

    if (!strcmp(param.method, "post")) {
        /* HTTP post */
        char *post_data;
        int post_data_len = atoi(param.post_len);
        int send_len;
        char conetnt_len_hdr_val[16];

        /* If the SECUREBOOT is enabled and file system is secured then
	 * the utils_file_secured_get() needs to be used. Also disable the SECURED,
	 * if this application is enabled at the compile time. */
        os_printf("\n[APP]HTTP Post");
        if (param.post_data) {
            post_data = (char *)param.post_data;
            post_data_len = strlen(post_data);
        } else if (param.post_data_file) {
            post_data = utils_file_get(param.post_data_file, &post_data_len);
            if (NULL == post_data) {
                os_printf("\n[APP]Error opening %s file", param.post_data_file);
            }
        } else {
            post_data = osal_alloc(1024);
            if (NULL == post_data) {
                os_printf("\n[APP]Error: malloc failre for post_data");
		goto exit;
            }
            memset(post_data, 'a', 1024);
        }
        sprintf(conetnt_len_hdr_val, "%d", post_data_len);
        http_client_set_req_hdr(
            http_handle, "Content-length", conetnt_len_hdr_val);
        if (strlen(path))
            param.path = path;
        while (post_data_len) {
            send_len = post_data_len > 1024 ? 1024 : post_data_len;
            rval = http_client_post(http_handle, (char *)param.path, post_data,
                send_len, app_http_cb, NULL, 300);
            if (rval < 0) {
                os_printf(
                    "\n[APP]Failure : http_client_post(), rval = %d", rval);
                break;
            }
            post_data_len -= send_len;
        }
        if (rval >= 0) {
            os_printf("\n[APP]Success: http_client_post(), rval = %d", rval);
        }
        if (rval < 0 || rval == HTTP_CLIENT_CLOSE_CONNECTION) {
            http_client_close(http_handle);
        }
        if (!param.post_data) {
            osal_free(post_data);
        }
        post_data = NULL;
        test_iterations--;
        if (test_iterations) {
            http_client_close(http_handle);
            goto again;
        }
    }
exit:
    osal_free(cfg.ssl_cfg.client_key.buf);
    os_printf("\n\n[APP]------ Program Exit-------------\n\n");
    return 0;
}
