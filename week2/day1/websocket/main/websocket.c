/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : websocket
 * @created     : Monday Oct 07, 2024 13:03:17 CEST
 */

/******************************/
/*         Includes           */
/******************************/

#include <stdio.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "mdns.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "portmacro.h"
#include <esp_http_server.h>
#include <mbedtls/sha1.h>
#include <mbedtls/base64.h>


#define TAG "ws server"

/* #define SSID "StanelyESP" */
/* #define PASS "ouiouioui" */
#define SSID "PoleDeVinci_IFT"
#define PASS "*c.r4UV@VfPn_0"


/******************************/
/*     Global Variables       */
/******************************/

/******************************/
/*     Static Variables       */
/******************************/
struct async_resp_arg
{
        httpd_handle_t hd; // Server instance
        int fd;            // Session socket file descriptor
};

httpd_handle_t server = NULL;
httpd_config_t config = HTTPD_DEFAULT_CONFIG();

/******************************/
/*    Function Prototypes     */
/******************************/

/******************************/
/*   Function Definitions     */
/******************************/
void send_data_task(void *arg)
{
        /* httpd_handle_t hd = (httpd_handle_t)arg; */
        char message[64];
        float temperature = 20.0;  // Simulated temperature

        while (1) 
        {
                temperature += 0.1;  // Simulate temperature increase
                snprintf(message, sizeof(message), "{\"temperature\": %.2f}", temperature);

                vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
}

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
        switch (event_id)
        {
                case WIFI_EVENT_STA_START:
                        printf("WiFi connecting ... \n");
                        break;
                case WIFI_EVENT_STA_CONNECTED:
                        printf("WiFi connected ... \n");
                        break;
                case WIFI_EVENT_STA_DISCONNECTED:
                        printf("WiFi lost connection ... \n");
                        break;
                case IP_EVENT_STA_GOT_IP:
                        printf("WiFi got IP ... \n\n");
                        break;
                default:
                        break;
        }
}

int wifi_connection()
{
        /* NVS init */
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)  
        {
                nvs_flash_erase();
                nvs_flash_init();
        }

        /* Wifi station initialization */
        esp_netif_init();
        esp_event_loop_create_default();
        esp_netif_create_default_wifi_sta();
        wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&wifi_initiation); //     
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
        wifi_config_t wifi_cfg = 
        {
                .sta = 
                {
                        .ssid = SSID,
                        .password = PASS,
                        .threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK,
                        .channel = 6
                }
        };

        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_STA, &wifi_cfg);
        esp_wifi_start();
        esp_wifi_connect();

        ESP_RETURN_ON_ERROR(mdns_init(), TAG, "mDNS service init error.");
        mdns_hostname_set("stanleyesp");
        mdns_instance_name_set("stanleyesp");

        return ESP_OK;
}

static void ws_async_send(void *arg)
{
        static const char * data = "Async data";
        struct async_resp_arg *resp_arg = arg;
        httpd_handle_t hd = resp_arg->hd;
        int fd = resp_arg->fd;
        httpd_ws_frame_t ws_pkt;
        memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
        ws_pkt.payload = (uint8_t*)data;
        ws_pkt.len = strlen(data);
        ws_pkt.type = HTTPD_WS_TYPE_TEXT;

        httpd_ws_send_frame_async(hd, fd, &ws_pkt);
        free(resp_arg);
}

static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req)
{
        struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
        if (resp_arg == NULL) {
                return ESP_ERR_NO_MEM;
        }
        resp_arg->hd = req->handle;
        resp_arg->fd = httpd_req_to_sockfd(req);
        esp_err_t ret = httpd_queue_work(handle, ws_async_send, resp_arg);
        if (ret != ESP_OK) {
                free(resp_arg);
        }
        return ret;
}

/*
 * This handler echos back the received ws data
 * and triggers an async send if certain message received
 */
static esp_err_t echo_handler(httpd_req_t *req)
{
        if (req->method == HTTP_GET) {
                ESP_LOGI(TAG, "Handshake done, the new connection was opened");
                return ESP_OK;
        }
        httpd_ws_frame_t ws_pkt;
        uint8_t *buf = NULL;
        memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
        ws_pkt.type = HTTPD_WS_TYPE_TEXT;
        /* Set max_len = 0 to get the frame len */
        esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
        if (ret != ESP_OK) {
                ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
                return ret;
        }
        ESP_LOGI(TAG, "frame len is %d", (int)ws_pkt.len);
        if (ws_pkt.len) {
                /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
                buf = calloc(1, ws_pkt.len + 1);
                if (buf == NULL) {
                        ESP_LOGE(TAG, "Failed to calloc memory for buf");
                        return ESP_ERR_NO_MEM;
                }
                ws_pkt.payload = buf;
                /* Set max_len = ws_pkt.len to get the frame payload */
                ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
                if (ret != ESP_OK) {
                        ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
                        free(buf);
                        return ret;
                }
                ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
        }
        ESP_LOGI(TAG, "Packet type: %d", ws_pkt.type);
        if (ws_pkt.type == HTTPD_WS_TYPE_TEXT &&
                        strcmp((char*)ws_pkt.payload,"Trigger async") == 0) {
                free(buf);
                return trigger_async_send(req->handle, req);
        }

        ret = httpd_ws_send_frame(req, &ws_pkt);
        if (ret != ESP_OK) {
                ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
        }
        free(buf);
        return ret;
}

static const httpd_uri_t uri_handler = {
        .uri = "/socket",
        .method = HTTP_GET,
        .handler = echo_handler,
        .user_ctx = NULL,
        .is_websocket = true
};

static void websocket_app_start(void)
{
        // Start the httpd server
        ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
        if (httpd_start(&server, &config) == ESP_OK)
        {
                // Registering the uri_handler
                ESP_LOGI(TAG, "Registering URI handler");
                httpd_register_uri_handler(server, &uri_handler);

                xTaskCreate(send_data_task, "send_data_task", 4096, (void *)server, 5, NULL);
        }
}

void app_main(void)
{
        wifi_connection();
        vTaskDelay(1500 / portTICK_PERIOD_MS);
        websocket_app_start();
}
