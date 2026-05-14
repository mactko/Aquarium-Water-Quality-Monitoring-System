#include "ds18b20.h"

#define WRITE_SCRATCHPAD 0x4E
#define READ_ROM 0x33
#define SKIP_ROM 0xCC
#define CONVERT_TEMP 0x44
#define WRITE_SCRATCHPAD 0x4E
#define READ_SCRATCHPAD 0xBE
#define COPY_SCRATCHPAD 0x48
#define RECALL_E2 0xB8
#define READ_PS 0xB4
#define ALARM_SEARCH 0xEC

static const struct _OneWireBus_Timing standard_timing = {
        6,    // A - read/write "1" master pull DQ low duration
        64,   // B - write "0" master pull DQ low duration
        60,   // C - write "1" master pull DQ high duration
        10,   // D - write "0" master pull DQ high duration
        9,    // E - read master pull DQ high duration
        55,   // F - complete read timeslot + 10ms recovery
        0,    // G - wait before reset
        480,  // H - master pull DQ low duration
        70,   // I - master pull DQ high duration
        410,  // J - complete presence timeslot + recovery
};

esp_err_t init_DS(gpio_num_t DS_PIN, DS_Info *sensor){

    sensor->DS_PIN = DS_PIN;

    if(reset_sequence(sensor->DS_PIN) != ESP_OK) {
        return ESP_FAIL;
    } 

    write_byte(READ_ROM, sensor->DS_PIN);
    read_byte(&sensor->rom_code.family_code, sensor->DS_PIN);
    for(int i = 0; i < 6; i++){
        read_byte(&sensor->rom_code.SSID[i], sensor->DS_PIN);
    }
    read_byte(&sensor->rom_code.crc, sensor->DS_PIN);

    return ESP_OK;

}

static esp_err_t reset_sequence(gpio_num_t ds_gpio){

    gpio_set_direction(ds_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(ds_gpio, 0);
    ets_delay_us(standard_timing.H);
    gpio_set_level(ds_gpio, 1);
    gpio_set_direction(ds_gpio, GPIO_MODE_INPUT);
    ets_delay_us(standard_timing.I);
    if(gpio_get_level(ds_gpio) != 0){
        return ESP_FAIL;
    }
    ets_delay_us(standard_timing.J);
    return ESP_OK;
}

esp_err_t configure_sensor(DS18B20_RESOLUTION resolution, DS_Info *sensor_info){

    uint8_t s_data[3] = {0, 0, configuration_register(resolution)};

    if(reset_sequence(sensor_info->DS_PIN) != ESP_OK) {
        return ESP_FAIL;
    } 
    write_byte(SKIP_ROM, sensor_info->DS_PIN);
    write_byte(WRITE_SCRATCHPAD, sensor_info->DS_PIN);
    for(int i = 0; i < 3; i++){
        write_byte(s_data[i], sensor_info->DS_PIN);
    }

    sensor_info->resolution = resolution;

    return ESP_OK;
}

static uint8_t configuration_register(DS18B20_RESOLUTION resolution){
    switch(resolution){
        case DS18B20_RESOLUTION_BITS_9:
            return 0x1F;
        case DS18B20_RESOLUTION_BITS_10:
            return 0x3F;
        case DS18B20_RESOLUTION_BITS_11:
            return 0x5F;
        case DS18B20_RESOLUTION_BITS_12:
            return 0x7F;
        default:
            return 0X7F;
    }
}

static void write_byte(uint8_t byte, gpio_num_t ds_gpio){
    for(int i = 0; i < 8; i++){
        write_bit(byte & ((0x01) << i), ds_gpio);
    }
    ets_delay_us(100);
}

static void read_byte(uint8_t *byte, gpio_num_t ds_gpio){
    for(int i = 0; i < 8; i++){
        *(byte) &= ~(0x1 << i);
        *(byte) |= (read_bit(ds_gpio) << i); 
        ets_delay_us(15);
    }
}

static void write_bit(uint8_t bit, gpio_num_t ds_gpio) {
    gpio_set_direction(ds_gpio, GPIO_MODE_OUTPUT);
    if(bit != 0){
        gpio_set_level(ds_gpio, 0);
        ets_delay_us(standard_timing.A);
        gpio_set_level(ds_gpio, 1);
        ets_delay_us(standard_timing.B);
    } else {
        gpio_set_level(ds_gpio, 0);
        ets_delay_us(standard_timing.C);
        gpio_set_level(ds_gpio, 1);
        ets_delay_us(standard_timing.D);
    }
}

static uint8_t read_bit(gpio_num_t ds_gpio){
    uint8_t bit = 0;
    gpio_set_direction(ds_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(ds_gpio, 0);
    ets_delay_us(standard_timing.A);
    gpio_set_level(ds_gpio, 1);
    gpio_set_direction(ds_gpio, GPIO_MODE_INPUT);
    ets_delay_us(standard_timing.E);
    if(gpio_get_level(ds_gpio) != 0){
        bit = 0x1;
    }
    ets_delay_us(standard_timing.F);
    return bit;
}

static uint16_t conversion_wait(DS18B20_RESOLUTION resolution){
    switch(resolution){
        case DS18B20_RESOLUTION_BITS_9:
            return 100;
        case DS18B20_RESOLUTION_BITS_10:
            return 200;
        case DS18B20_RESOLUTION_BITS_11:
            return 375;
        case DS18B20_RESOLUTION_BITS_12:
            return 750;
        default:
            return 750;
    }
}

static uint8_t crc_checker(uint8_t *data, uint8_t size)
{
    uint8_t crc = 0;
    uint8_t current_byte = 0;
    uint8_t temp = 0;

    while (size--) {
        current_byte = *data++;
        for (uint8_t i = 8; i > 0; i--)
        {
            temp = (crc ^ current_byte) & 0x01;
            crc >>= 1;
            if (temp != 0) {
                crc ^= (0x8C);
            }
            current_byte >>= 1;
        }
    }

    return crc;
}

DS18B20_reading get_temp_DS(DS_Info *sensor) {
    DS18B20_reading data = {ESP_FAIL, 0.f};
    gpio_num_t ds_pin = sensor->DS_PIN;

    if(reset_sequence(ds_pin) != ESP_OK){
        return data;
    }
    write_byte(SKIP_ROM, ds_pin);
    write_byte(CONVERT_TEMP, ds_pin);
    vTaskDelay(conversion_wait(sensor->resolution)/portTICK_PERIOD_MS);

    if(reset_sequence(ds_pin) != ESP_OK){
        return data;
    }
    write_byte(SKIP_ROM, ds_pin);
    write_byte(READ_SCRATCHPAD, ds_pin);

    for(int i = 0; i < 9; i++){
        read_byte(((uint8_t *)&sensor->scratchpad + i), ds_pin);
    }

    if(crc_checker(&sensor->scratchpad, 9) != 0){ return data; }

    data.reading = (float)(*((uint8_t *)&sensor->scratchpad) + (*((uint8_t *)&sensor->scratchpad + 1) *256))/16;

    data.status = ESP_OK;

    return data;
}

