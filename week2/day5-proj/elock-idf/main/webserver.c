/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : websocket
 * @created     : Tuesday Oct 08, 2024 10:15:29 CEST
 */

/******************************/
/*         Includes           */
/******************************/
#include "webserver.h"

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_check.h"
#include "mdns.h"
#include <stdio.h>
#include <string.h>

/******************************/
/*     Global Variables       */
/******************************/
httpd_handle_t server = NULL;
int fd = 0;

/******************************/
/*     Static Variables       */
/******************************/
#define TAG "webserver"

void (* callback)(const char *) = NULL;

struct async_resp_arg 
{
        httpd_handle_t hd;
        int fd;
};

/******************************/
/*    Function Prototypes     */
/******************************/

/******************************/
/*   Function Definitions     */
/******************************/
void ws_broadcast(const char *key, const char *val)
{
        httpd_ws_frame_t ws_pkt;
        memset(&ws_pkt, -1, sizeof(httpd_ws_frame_t));

        ws_pkt.type = HTTPD_WS_TYPE_TEXT;
        ws_pkt.len = strlen(key) + strlen(val) + 8;
        char *buf = calloc(1, ws_pkt.len + 1);
        snprintf(buf, ws_pkt.len + 1, "%s%s%s%s%s", "{\"", key, "\": \"", val, "\"}");
        ws_pkt.payload = (uint8_t*)buf;

        httpd_ws_send_frame_async(server, fd, &ws_pkt);
        free(buf);
}

static esp_err_t ws_handler(httpd_req_t *req)
{
        if (req->method == HTTP_GET) 
        {
                ESP_LOGI(TAG, "Handshake done, the new connection was opened");
                return ESP_OK;
        }

        httpd_ws_frame_t ws_pkt;
        memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
        ws_pkt.type = HTTPD_WS_TYPE_TEXT;
        ESP_RETURN_ON_ERROR(httpd_ws_recv_frame(req, &ws_pkt, 0), TAG, "failed to get frame length");

        uint8_t *buf = NULL;
        if (ws_pkt.len) 
        {
                buf = calloc(1, ws_pkt.len + 1);
                ESP_RETURN_ON_FALSE(buf != NULL, ESP_ERR_NO_MEM, TAG, "failed to alloc buf");
                ws_pkt.payload = buf;
                esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
                if (ret != ESP_OK) 
                {
                        ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
                        free(buf);
                        return ret;
                }
                ESP_LOGI(TAG, "got packet with message: %s", ws_pkt.payload);
                if (callback) callback((const char *)ws_pkt.payload);
                fd = httpd_req_to_sockfd(req);
        }
        free(buf);
        return ESP_OK;
}

esp_err_t root_get_handler(httpd_req_t *req)
{
        httpd_resp_sendstr_chunk(req, "Hello");
        httpd_resp_sendstr_chunk(req, NULL);

        return ESP_OK;
}


esp_err_t ws_set_callback(void (* _callback)(const char *))
{
        callback = _callback;
        return ESP_OK;
}

esp_err_t ws_init()
{
        /* Start mDNS service */
        ESP_RETURN_ON_ERROR(mdns_init(), TAG, "mDNS service init error.");
        mdns_hostname_set("elock");
        mdns_instance_name_set("elock");

        httpd_config_t config = HTTPD_DEFAULT_CONFIG();

        ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
        ESP_RETURN_ON_ERROR(httpd_start(&server, &config), TAG, "error");
        ESP_LOGI(TAG, "Registering URI handlers");

        httpd_uri_t ws = {
                .uri        = "/ws",
                .method     = HTTP_GET,
                .handler    = ws_handler,
                .user_ctx   = NULL,
                .is_websocket = true
        };
        httpd_register_uri_handler(server, &ws);

        httpd_uri_t _root_get_handler = 
        {
                .uri = "/",
                .method = HTTP_GET,
                .handler = root_get_handler,
                .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &_root_get_handler);

        return ESP_OK;
}


