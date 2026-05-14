#include "main.h"

static float temperature_data = 0;
static int turbidity_data = 0;

static const char *TAG = "MQTTS";

esp_mqtt_client_handle_t mqtt_client;

volatile uint8_t brokerC = 0;

extern const uint8_t brokerCert_pem_start[] asm("_binary_brokerCert_pem_start");
extern const uint8_t brokerCert_pem_end[] asm("_binary_brokerCert_pem_end");

extern const uint8_t deviceCert_pem_start[] asm("_binary_deviceCert_pem_start");
extern const uint8_t deviceCert_pem_end[] asm("_binary_deviceCert_pem_end");

extern const uint8_t deviceCert_key_start[] asm("_binary_deviceCert_key_start");
extern const uint8_t deviceCert_key_end[] asm("_binary_deviceCert_key_end");

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Connected to broker");
        esp_mqtt_client_publish(mqtt_client, "telemetry/my-device/temperature", "0.0|0", 0, 1, 0); 
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "Recieved PUBACK/PUBCOMP -> Test reached broker");
        brokerC = 1;
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");  
        brokerC = 0;  
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

esp_err_t mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .uri = "uri",
            },
            .verification = {
                .certificate = (char *)brokerCert_pem_start,
                .certificate_len = GetBrokerCertLength()
            },
        },
        .credentials = {
            .username = "ESP32-node",
            .authentication = {
                .certificate = (char *)deviceCert_pem_start,
                .certificate_len = GetDeviceCertLength(),
                .key = (char *)deviceCert_key_start,
                .key_len = GetDeviceKeyLength()
            }
        }
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if(mqtt_client == NULL){
        ESP_LOGI(TAG, "Failed MQTT client initilization ...");
        return ESP_FAIL;
    }
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);

    return ESP_OK;
}

static void send_telemetry_data(TURBIDITY_reading *turbidity, DS18B20_reading *temperature){
    uint16_t length_temp = snprintf(NULL, 0, "%f", temperature->reading) + snprintf(NULL, 0, "%d", turbidity->reading) + 1 + 1;

    //ESP_LOGI("LIST: ", "list size is %d", length_temp);
    ESP_LOGI(TAG, "Temperature:  %f, Turbidity: %d", temperature->reading, turbidity->reading);
    char data_string[length_temp]; 

    sprintf(data_string, "%f|%d", temperature->reading, turbidity->reading);

    esp_mqtt_client_publish(mqtt_client, "telemetry/my-device/temperature", data_string, 0, 0, 0); 
}

void wait_mqtt(){
    do{
        vTaskDelay(500);
    } while(brokerC == 0);
}

static uint32_t GetBrokerCertLength(){
    return brokerCert_pem_end - brokerCert_pem_start;
}
static uint32_t GetDeviceCertLength(){
    return deviceCert_pem_end - deviceCert_pem_start;
}
static uint32_t GetDeviceKeyLength(){
    return deviceCert_key_end - deviceCert_key_start;
}

void app_main(void)
{
    esp_err_t ret = ESP_OK;

    DS_Info ds18b20_sensor;
    TURBIDITY_reading turbidity;
    DS18B20_reading temperature;


    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    do {
        ret = init_DS(DS_PIN, &ds18b20_sensor);
    } while(ret != ESP_OK);
    
    
    ESP_ERROR_CHECK(turbidityInit(T_PIN));
    ESP_ERROR_CHECK(ledc_init());
    
    ESP_ERROR_CHECK(init_wifi());
    ESP_ERROR_CHECK(wait_wifi());
    ESP_ERROR_CHECK(mqtt_app_start());
    
    wait_mqtt();

    while(1){
        
        do{
            temperature = get_temp_DS(&ds18b20_sensor);
        } while(temperature.status != ESP_OK);
        ESP_LOGI("TEMP: ", "%f", temperature.reading);
        
        do{
            turbidityRead(&turbidity);
        } while(turbidity.status != ESP_OK);
        ESP_LOGI("TURBIDITY: ", "%d", turbidity.reading); 
        
        if(check_wifi() != 0 ) {
            send_telemetry_data(&turbidity, &temperature);
        }   

        pump_control(turbidity.reading);

        vTaskDelay(pdMS_TO_TICKS(120000));
    }
}
