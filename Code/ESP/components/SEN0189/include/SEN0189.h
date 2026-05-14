#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"
#include "driver/gpio.h"
#include "math.h"

typedef struct TURBIDITY_reading{
    int reading;
    esp_err_t status; 
} TURBIDITY_reading;

/**
 * @note initialize turbidity sensor 
 * @brief initialize all ADC funcionality to read values from T_PIN sent by sensor
*/
esp_err_t turbidityInit(gpio_num_t T_PIN);

/**
 * @note fit the measurements and return final value
 * @brief
*/
//static int fitData(int *list); 

/**
 * @note function called from user code
 * @brief conduct measurements, fit data and return it and the status of the operation
*/
void turbidityRead(TURBIDITY_reading *data);

