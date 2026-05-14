#include <stdio.h>
#include "SEN0189.h"
#include "esp_adc/adc_cali.h"

#define DEADZONE 1
#define MEDIAN_LEN 10

static adc_oneshot_unit_handle_t adc_handle;

esp_err_t turbidityInit(gpio_num_t T_PIN) {

    adc_oneshot_unit_init_cfg_t adc_init_cfg = {
        .ulp_mode = ADC_ULP_MODE_DISABLE,
        .clk_src = 0,
        .unit_id = ADC_UNIT_1,
    }; 

    if(adc_oneshot_new_unit(&adc_init_cfg, &adc_handle) != ESP_OK){
        return ESP_FAIL;
    }
    
    adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_11,
    };

    if(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_5, &channel_config) != ESP_OK) {
        return ESP_FAIL;
    } 

    return ESP_OK;
}  

void turbidityRead(TURBIDITY_reading *data) {

    int adc_raw[MEDIAN_LEN] = {0};
    float current_reading = 0.f;
    float voltage = 0.f;
    int key, j = 0;

    for(int i = 0; i < MEDIAN_LEN; i++){
        if(adc_oneshot_read(adc_handle, ADC_CHANNEL_5, &adc_raw[i]) != ESP_OK) {
            data->status = ESP_FAIL;
        } 
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    for(int i = 0; i < MEDIAN_LEN; i++){
        key = adc_raw[i];
        j = i - 1;
        while(j >= 0 && key < adc_raw[j]){
            adc_raw[j+1] = adc_raw[j];
            j -= 1;
        }
        adc_raw[j+1] = key;
    }    

    voltage = ((float)(adc_raw[5]+adc_raw[4])/(2*4096)) * 3.3f * 31.0/20.0;
    current_reading = -1120.4 * pow(voltage, 2) + 5742.3 * voltage - 4060;

    if(current_reading > data->reading + DEADZONE){
        data->reading = (int)current_reading - DEADZONE;
    } else if(current_reading < data->reading - DEADZONE){
        data->reading = (int)current_reading + DEADZONE;
    } 

    data->status = ESP_OK;
}

