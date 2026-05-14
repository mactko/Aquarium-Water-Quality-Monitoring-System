#include "esp_wifi.h"
#include "esp_log.h"

#define _MAXRETRY 10
#define WIFI_SUCCESS BIT0
#define WIFI_FAIL BIT1


void wifi_event_handler(void *hanlder_arg, esp_event_base_t base, int32_t id, void *event_data);

void ip_event_handler(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data);

esp_err_t init_wifi();

esp_err_t wait_wifi();

int check_wifi();
