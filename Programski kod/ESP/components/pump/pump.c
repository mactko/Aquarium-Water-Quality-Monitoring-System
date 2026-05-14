#include "pump.h"

#define PUMP_CHANNEL LEDC_CHANNEL_0
#define PUMP_TIMER LEDC_TIMER_1
#define PUMP_GPIO GPIO_NUM_18
#define PUMP_MIN_DUTY (3072) //(1024)
#define PUMP_MID_DUTY (3584)
#define PUMP_MAX_DUTY (4096)
#define PUMP_FADE_TIME (1000)
#define HIST 10

static ledc_timer_config_t pump_config;

void pump_control(int turbidity){

    if (turbidity > 80) {
        ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, PUMP_CHANNEL, PUMP_MAX_DUTY, PUMP_FADE_TIME);
    } else if(turbidity > 30){
        ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, PUMP_CHANNEL, PUMP_MID_DUTY, PUMP_FADE_TIME);
    } else if(turbidity < 30) {
        ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, PUMP_CHANNEL, PUMP_MIN_DUTY, PUMP_FADE_TIME);
    } 

    ledc_fade_start(LEDC_HIGH_SPEED_MODE, PUMP_CHANNEL, LEDC_FADE_NO_WAIT);
}

esp_err_t ledc_init()
{
    
    pump_config.duty_resolution = LEDC_TIMER_12_BIT;
    pump_config.freq_hz = 1000;
    pump_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    pump_config.timer_num = PUMP_TIMER;
    pump_config.clk_cfg = LEDC_USE_APB_CLK;

    if (ledc_timer_config(&pump_config) != ESP_OK) return ESP_FAIL;

    ledc_channel_config_t pump_channel_config  = {
        .channel = PUMP_CHANNEL,
        .duty = PUMP_MIN_DUTY,
        .gpio_num = PUMP_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = PUMP_TIMER,
        .flags.output_invert = 0,
    };

    if (ledc_channel_config(&pump_channel_config) != ESP_OK) return ESP_FAIL;

    if (ledc_fade_func_install(0) != ESP_OK) return ESP_FAIL;

    return ESP_OK;
}