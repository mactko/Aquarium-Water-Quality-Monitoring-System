#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_sleep.h"


#include "mqtt_client.h"
#include "DS18B20.h"
#include "SEN0189.h"
#include "pump.h"
#include "wifi.h"

#define READING_RETRY 5
#define DS_PIN GPIO_NUM_32
#define T_PIN GPIO_NUM_33

#define MQTT_CONNECT BIT0
#define MQTT_FAIL BIT1

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

esp_err_t mqtt_app_start(void);

static void send_telemetry_data(TURBIDITY_reading *turbidity, DS18B20_reading *temperature);

static uint32_t GetBrokerCertLength();

static uint32_t GetDeviceCertLength();

static uint32_t GetDeviceKeyLength();