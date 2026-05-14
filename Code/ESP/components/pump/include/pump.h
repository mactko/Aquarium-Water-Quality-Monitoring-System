#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"

void pump_control(int turbidity);

esp_err_t ledc_init();