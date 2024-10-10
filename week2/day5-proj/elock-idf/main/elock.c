/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : elock
 * @created     : Tuesday Oct 08, 2024 09:45:06 CEST
 */

/******************************/
/*         Includes           */
/******************************/
#include "esp_err.h"
#include "freertos/projdefs.h"
#include "wifi.h"
#include "webserver.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_10_BIT
#define LEDC_FREQUENCY          (50)

#define MOTOR_PIN 7
#define ENDSTOP_OPEN_PIN 1
#define ENDSTOP_CLOSE_PIN 4
#define LED_RED_PIN 2
#define LED_GREEN_PIN 9
#define SENSOR_PIN 8

#define TAG "elock"

/******************************/
/*     Global Variables       */
/******************************/

/******************************/
/*     Static Variables       */
/******************************/
enum state : uint8_t
{
        CLOSE = 1,
        OPEN,
        MOVING,
};

enum state state = 0;
bool is_forcing = false;

/******************************/
/*    Function Prototypes     */
/******************************/
static void ws_callback(const char *payload);
static void motor_turn_on(bool cw);
static void motor_turn_off();
static void loop_task(void *arg);
static esp_err_t lock_close();
static esp_err_t lock_open();

/******************************/
/*   Function Definitions     */
/******************************/
void send_state()
{
        switch (state) 
        {
                case OPEN:
                        ws_async_send("state", "open");
                        break;
                case CLOSE:
                        ws_async_send("state", "close");
                        break;
                case MOVING:
                        ws_async_send("state", "moving");
                        break;
        }
}

void ws_callback(const char *payload)
{
        if (strcmp(payload, "close") == 0)
        {
                ESP_LOGI(TAG, "closing lock..");
                ws_async_send("log", "lock closing...");
                if (lock_close() == ESP_OK)
                {
                        ESP_LOGI(TAG, "lock closed");
                }
        }
        else if (strcmp(payload, "open") == 0)
        {
                ESP_LOGI(TAG, "opening lock..");
                if (lock_open() == ESP_OK)
                {
                        ESP_LOGI(TAG, "lock opened");
                }
        }
        else if (strcmp(payload, "state") == 0)
        {
                send_state();
        }
}

void loop_task(void *arg)
{
        while (1)
        {
                // Check endstops
                if (gpio_get_level(ENDSTOP_OPEN_PIN) == 0)
                {
                        if (state != OPEN)
                        {
                                ESP_LOGI(TAG, "lock opened");
                                state = OPEN;
                        }
                }
                else if (gpio_get_level(ENDSTOP_CLOSE_PIN) == 0)
                {
                        if (state != CLOSE)
                        {
                                ESP_LOGI(TAG, "lock closed");
                                state = CLOSE;
                        }
                }
                else if (state != MOVING)
                {
                        ESP_LOGI(TAG, "lock moving");
                        state = MOVING;
                }

                // Check pressure sensor
                if (gpio_get_level(SENSOR_PIN) == 1)
                {
                        if (!is_forcing)
                        {
                                is_forcing = true;
                                ws_async_send("status", "forcing");
                        }
                }
                else
                {
                        if (is_forcing)
                        {
                                is_forcing = false;
                                ws_async_send("status", "ok");
                        }
                }


                vTaskDelay(pdMS_TO_TICKS(10));
        }
}

void motor_turn_on(bool cw)
{
        if (cw)
        {
                ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 1.1f * 1023 / 20.0f));
        }
        else
        {
                ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 1.9f * 1023 / 20.0f));
        }
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

void motor_turn_off()
{
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 1.5f * 1023 / 20.0f));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

esp_err_t lock_close()
{
        if (state == CLOSE) return ESP_FAIL;

        motor_turn_on(false);
        while(state != CLOSE)
        {
                vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        motor_turn_off();

        return ESP_OK;
}

esp_err_t lock_open()
{
        if (state == OPEN) return ESP_FAIL;

        motor_turn_on(true);
        while(state != OPEN)
        {
                vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        motor_turn_off();

        return ESP_OK;
}

uint8_t get_battery_level()
{
        return 0xFF;
}

void app_main(void)
{
        wifi_init();
        ws_init();
        ws_set_callback(ws_callback);

        gpio_reset_pin(MOTOR_PIN);
        gpio_reset_pin(ENDSTOP_CLOSE_PIN);
        gpio_reset_pin(ENDSTOP_OPEN_PIN);
        gpio_reset_pin(LED_GREEN_PIN);
        gpio_reset_pin(LED_RED_PIN);
        gpio_reset_pin(SENSOR_PIN);

        gpio_set_direction(MOTOR_PIN, GPIO_MODE_OUTPUT);
        gpio_set_direction(LED_GREEN_PIN, GPIO_MODE_OUTPUT);
        gpio_set_direction(LED_RED_PIN, GPIO_MODE_OUTPUT);
        gpio_set_direction(ENDSTOP_OPEN_PIN, GPIO_MODE_INPUT);
        gpio_set_direction(ENDSTOP_CLOSE_PIN, GPIO_MODE_INPUT);
        gpio_set_direction(SENSOR_PIN, GPIO_MODE_INPUT);

        gpio_pullup_en(ENDSTOP_CLOSE_PIN);
        gpio_pullup_en(ENDSTOP_OPEN_PIN);

        /* Init LEDC Driver */
        ledc_timer_config_t ledc_timer = 
        {
                .speed_mode       = LEDC_MODE,
                .duty_resolution  = LEDC_DUTY_RES,
                .timer_num        = LEDC_TIMER,
                .freq_hz          = LEDC_FREQUENCY,
                .clk_cfg          = LEDC_AUTO_CLK
        };
        ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

        ledc_channel_config_t ledc_channel = 
        {
                .speed_mode     = LEDC_MODE,
                .channel        = LEDC_CHANNEL,
                .timer_sel      = LEDC_TIMER,
                .intr_type      = LEDC_INTR_DISABLE,
                .gpio_num       = MOTOR_PIN,
                .duty           = 0,
                .hpoint         = 0
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

        xTaskCreate(loop_task, "loop", 4096, NULL, 2, NULL);
}
