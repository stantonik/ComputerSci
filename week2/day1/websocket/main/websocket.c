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


#define TAG "ws server"

#define SSID "StanelyESP"
#define PASS "ouiouioui"


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
        httpd_handle_t hd = (httpd_handle_t)arg;
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
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)  
        {
                nvs_flash_erase();
                nvs_flash_init();
        }

        /* Wifi station initialization */
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_ap();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                ESP_EVENT_ANY_ID,
                                &wifi_event_handler,
                                NULL,
                                NULL));

        wifi_config_t wifi_config = {
                .ap = {
                        .ssid = SSID,
                        .channel = 6,
                        .password = PASS,
                        .max_connection = 2,
                        .authmode = WIFI_AUTH_WPA3_PSK,
                        .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
                        .pmf_cfg = {
                                .required = true,
                        },
                },
        };

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d", SSID, PASS, 6);

        ESP_RETURN_ON_ERROR(mdns_init(), TAG, "mDNS service init error.");
        mdns_hostname_set("stanleyesp");
        mdns_instance_name_set("stanleyesp");

        return ESP_OK;
}

static void generate_async_resp(void *arg)
{
        // Data format to be sent from the server as a response to the client
        char http_string[250];
        char *data_string = "Test test test";
        sprintf(http_string, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", (int)strlen(data_string));

        // Initialize asynchronous response data structure
        struct async_resp_arg *resp_arg = (struct async_resp_arg *)arg;
        httpd_handle_t hd = resp_arg->hd;
        int fd = resp_arg->fd;

        // Send data to the client
        ESP_LOGI(TAG, "Executing queued work fd : %d", fd);
        httpd_socket_send(hd, fd, http_string, strlen(http_string), 0);
        httpd_socket_send(hd, fd, data_string, strlen(data_string), 0);

        free(arg);
}

static esp_err_t async_get_handler(httpd_req_t *req)
{
        struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
        resp_arg->hd = req->handle;
        resp_arg->fd = httpd_req_to_sockfd(req);
        ESP_LOGI(TAG, "Queuing work fd : %d", resp_arg->fd);
        httpd_queue_work(req->handle, generate_async_resp, resp_arg);
        return ESP_OK;
}

static const httpd_uri_t uri_handler = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = async_get_handler,
        .user_ctx = NULL,
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
