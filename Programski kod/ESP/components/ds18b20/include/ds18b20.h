#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <math.h>

#define noInterrupts() portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED; taskENTER_CRITICAL(&mux)
#define interrupts() taskEXIT_CRITICAL(&mux)

typedef struct Scratchpad{
    uint8_t temperature[2];
    uint8_t temperature_HI;
    uint8_t temperature_LO;
    uint8_t cofiguration;
    uint8_t reserved[3];
    uint8_t crc;
} Scratchpad;

typedef struct _OneWireBus_Timing
{
    uint32_t A, B, C, D, E, F, G, H, I, J;
} _OneWireBus_Timing;

typedef struct DS18B20_ROM{
    uint8_t family_code;
    uint8_t SSID[6];
    uint8_t crc;
} DS18B20_ROM;

typedef enum DS18B20_RESOLUTION{
    DS18B20_RESOLUTION_BITS_12 = 12,
    DS18B20_RESOLUTION_BITS_11 = 11,
    DS18B20_RESOLUTION_BITS_10 = 10,
    DS18B20_RESOLUTION_BITS_9 = 9
} DS18B20_RESOLUTION;

typedef struct DS18B20_reading{
    esp_err_t status;
    float reading;
} DS18B20_reading;

typedef struct DS_Info{
    bool initalized;
    gpio_num_t DS_PIN;
    DS18B20_RESOLUTION resolution;
    DS18B20_ROM rom_code;
    Scratchpad scratchpad;
} DS_Info;

/**
 * @brief Initialize ds18b20 sensor on pin DS_PIN
*/
esp_err_t init_DS(gpio_num_t DS_PIN, DS_Info *sensor_info);

/**
 * @brief change configuration register (resolution of measurment)
*/
esp_err_t configure_sensor(DS18B20_RESOLUTION resolution, DS_Info *sensor_info);

/**
 * @brief initiate measurement and read data after measuring;
*/
DS18B20_reading get_temp_DS(DS_Info *sensor);

/**
 * @brief initialization sequence for every communication with the sensor
*/
static esp_err_t reset_sequence();

/**
 * @brief return configuration register data for resolution 
*/
static uint8_t configuration_register(DS18B20_RESOLUTION resolution);

/**
 * @brief MCU writes byte to bus 
*/
static void write_byte(uint8_t byte, gpio_num_t ds_gpio);

/**
 * @brief MCU writes bit to bus 
*/
static void write_bit(uint8_t bit, gpio_num_t ds_gpio);

/**
 * @brief MCU reads byte from bus
*/
static void read_byte(uint8_t *byte, gpio_num_t ds_gpio);

/**
 * @brief MCU reads bit from bus 
*/
static uint8_t read_bit();

/**
 * @brief return wait time dependent on sensor resolution   
*/
static uint16_t conversion_wait();

/**
 * @brief check CRC
*/
static uint8_t crc_checker(uint8_t *data, uint8_t size);