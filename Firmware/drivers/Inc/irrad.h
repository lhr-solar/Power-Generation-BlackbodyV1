#pragma once
#include "stm32xx_hal.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdint.h>

#define TSL25911FN_7BIT_ADDRESS (0x29)
#define TSL25911FN_8BIT_ADDRESS (0x29 << 1)

#define TSL25911FN_REG_CMD (0xA0)
#define TSL25911FN_REG_ENABLE (0x00) //power on/off
#define TSL25911FN_REG_CONTROL (0x01)
#define TSL25911FN_ENABLE_POWER_ON (TSL25911FN_BIT_PON | TSL25911FN_BIT_AEN)
#define TSL25911FN_ENABLE_POWER_OFF (0x00)

#define TSL25911FN_REG_PACKID (0x11) //Device Package ID
#define TSL25911FN_REG_DEVID (0x12)
#define TSL25911FN_REG_STATUS (0x13)

#define TSL25911FN_REG_C0DATAL (0x14) //Channel 0 Data Low
#define TSL25911FN_REG_C0DATAH (0x15) //Channel 0 Data High
#define TSL25911FN_REG_C1DATAL (0x16) //Channel 1 Data Low
#define TSL25911FN_REG_C1DATAH (0x17) //Channel 1 Data High

#define TSL25911FN_REG_AILTL (0x04) //ALS interrupt low threshold low byte
#define TSL25911FN_REG_AILTH (0x05) //ALS interrupt low threshold high byte
#define TSL25911FN_REG_AIHTL (0x06) //ALS interrupt high threshold low byte
#define TSL25911FN_REG_AIHTH (0x07) //ALS interrupt high threshold high byte

#define TSL25911FN_REG_NPAILTL (0x08) //No Persist ALS interrupt low threshold low byte
#define TSL25911FN_REG_NPAILTH (0x09) //No Persist ALS interrupt low threshold high byte
#define TSL25911FN_REG_NPAILHTL (0x0A) //No Persist ALS interrupt high threshold low byte
#define TSL25911FN_REG_NPAIHTH (0x0B) //No Persist ALS interrupt high threshold high byte

#define TSL25911FN_REG_PERSIST (0x0C) //Interrupt persistence filter

#define TSL25911FN_BIT_PON (1 << 0) //power on
#define TSL25911FN_BIT_AEN (1 << 1) //Ambient Light Sensor enabled
#define TSL25911FN_BIT_AVALID (1 << 0 ) //Checks if ALS valid in Status Reg

typedef enum {
    TSL25911FN_OK,
    TSL25911FN_ERR,
    TSL25911FN_INIT_FAIL,
    TSL25911FN_READ_FAIL,
    TSL25911FN_WRITE_FAIL
} tsl25911fn_status_t;

typedef enum {
    TSL25911FN_GAIN_LOW  = 0x00,
    TSL25911FN_GAIN_MED  = 0x10,
    TSL25911FN_GAIN_HIGH = 0x20,
    TSL25911FN_GAIN_MAX  = 0x30
} tsl25911fn_gain_t;

typedef enum {
    TSL25911FN_TIME_100MS = 0x00,
    TSL25911FN_TIME_200MS = 0x01,
    TSL25911FN_TIME_300MS = 0x02,
    TSL25911FN_TIME_400MS = 0x03,
    TSL25911FN_TIME_500MS = 0x04,
    TSL25911FN_TIME_600MS = 0x05
} tsl25911fn_time_t;

typedef struct {
    uint8_t device_addr;
    I2C_HandleTypeDef *hi2c;
    uint8_t gain;
    uint8_t time;
    uint8_t control;
    SemaphoreHandle_t tsl_i2c_smphr;
} TSL25911FN_HandleTypeDef;

typedef struct{
    uint16_t ch0;
    uint16_t ch1;
    int32_t irrad_whitelight_q16;
    int32_t irrad_infrared_q16;
} TSL25911FN_data_t;

tsl25911fn_status_t tsl25911fn_init(TSL25911FN_HandleTypeDef *handle, I2C_HandleTypeDef *hi2c);
tsl25911fn_status_t tsl25911fn_power_on(TSL25911FN_HandleTypeDef *handle, TickType_t delay);
tsl25911fn_status_t tsl25911fn_power_off(TSL25911FN_HandleTypeDef *handle, TickType_t delay);
tsl25911fn_status_t tsl25911fn_set_control(TSL25911FN_HandleTypeDef *handle, uint8_t control, TickType_t delay);
tsl25911fn_status_t tsl25911fn_read_reg(TSL25911FN_HandleTypeDef *handle, uint8_t reg, volatile uint8_t *value, TickType_t delay);
tsl25911fn_status_t tsl25911fn_write_reg(TSL25911FN_HandleTypeDef *handle, uint8_t reg, uint8_t value, TickType_t delay);
tsl25911fn_status_t tsl25911fn_read_channels(TSL25911FN_HandleTypeDef *handle, uint16_t *ch0, uint16_t *ch1, TickType_t delay);
tsl25911fn_status_t tsl25911fn_read_data(TSL25911FN_HandleTypeDef *handle, TSL25911FN_data_t *data, TickType_t delay);

extern volatile uint8_t tsl_i2c_tx_done;
extern volatile uint8_t tsl_i2c_error;




