#include "wifi.h"

static const char *WIFI_TAG = "WIFI";

static EventGroupHandle_t wifi_event_group;

static int retry_num = 0;

void wifi_event_handler(void *hanlder_arg, esp_event_base_t base, int32_t id, void *event_data){
    if(base == WIFI_EVENT && id == WIFI_EVENT_STA_START){
        ESP_LOGI(WIFI_TAG, "Connecting to AP ...");
        esp_wifi_connect();
    } else if(base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED){
        if(xEventGroupGetBits(wifi_event_group) && WIFI_SUCCESS != 0){
            xEventGroupClearBits(wifi_event_group, WIFI_SUCCESS);
        }
        if(retry_num > _MAXRETRY){
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL);
        } else{
            ESP_LOGI(WIFI_TAG, "Retrying connection to AP ....");
            esp_wifi_connect();
            retry_num++;
        }
    }
}

void ip_event_handler(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data){
    if(base == IP_EVENT && id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t* got_ip_event = (ip_event_got_ip_t*) event_data; 
        ESP_LOGI(WIFI_TAG, "STA_IP: " IPSTR, IP2STR(&got_ip_event->ip_info.ip));
        retry_num = 0;
        xEventGroupClearBits(wifi_event_group, WIFI_FAIL);
        xEventGroupSetBits(wifi_event_group, WIFI_SUCCESS);
    }
}

esp_err_t init_wifi(){

    wifi_event_group = xEventGroupCreate(); 

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if(esp_wifi_init(&cfg) != ESP_OK) return ESP_FAIL; 

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "Galaxy A54 M",
            .password = "matkatign",
        }
    };

    if(esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) return ESP_FAIL;
    if(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)) return ESP_FAIL;
    if(esp_wifi_start() != ESP_OK) return ESP_FAIL; 
    ESP_LOGI(WIFI_TAG, "STA initialisation complete");

    return ESP_OK;
}

esp_err_t wait_wifi(){

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_SUCCESS | WIFI_FAIL, pdFALSE, pdFALSE, portMAX_DELAY);
    //xEventGroupClearBits(wifi_event_group, WIFI_SUCCESS|WIFI_FAIL);
    if(bits & WIFI_SUCCESS) {
        ESP_LOGI(WIFI_TAG, "Connected to AP");
        return ESP_OK;
    } else if (bits & WIFI_FAIL){
        ESP_LOGI(WIFI_TAG, "Failed to connect to AP");
        return ESP_FAIL;
    } else {
        ESP_LOGI(WIFI_TAG, "Unexpected event");
        return ESP_FAIL;
    }
}

int check_wifi() {
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_SUCCESS | WIFI_FAIL, pdFALSE, pdFALSE, portMAX_DELAY);
    return (bits & WIFI_SUCCESS);
}