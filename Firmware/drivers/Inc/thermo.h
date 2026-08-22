#pragma once
#include "stm32xx_hal.h"
#include <stdio.h>
#include "UART.h"
#include "printf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOSConfig.h"

#define MCP9600_7BIT_ADDR_0 (0x60)
#define MCP9600_7BIT_ADDR_1 (0x61)
#define MCP9600_7BIT_ADDR_2 (0x62)
#define MCP9600_7BIT_ADDR_3 (0x63)
#define MCP9600_7BIT_ADDR_4 (0x64)
#define MCP9600_7BIT_ADDR_5 (0x65)
#define MCP9600_7BIT_ADDR_6 (0x66)
#define MCP9600_7BIT_ADDR_7 (0x67)

#define MCP9600_DEV_ID (0x20)

#define MCP9600_FINAL_TEMP_REG (0x00)
#define MCP9600_CHANGE_TEMP_REG (0x01)
#define MCP9600_COLDJUNC_TEMP_REG (0x02)
#define MCP9600_RAW_ADC_VALUE_REG (0x03)

#define MCP9600_STATUS_REG (0x04)
#define MCP9600_SENSOR_CONFIG_REG (0x05)
#define MCP9600_DEVICE_CONFIG_REG (0x06)

#define MCP9600_ALERT_1_TEMP_REG (0x10)
#define MCP9600_ALERT_2_TEMP_REG (0x11)
#define MCP9600_ALERT_3_TEMP_REG (0x12)
#define MCP9600_ALERT_4_TEMP_REG (0x13)

#define MCP9600_ALERT_1_HYSTERESIS_REG (0x0C)
#define MCP9600_ALERT_2_HYSTERESIS_REG (0x0D)
#define MCP9600_ALERT_3_HYSTERESIS_REG (0x0E)
#define MCP9600_ALERT_4_HYSTERESIS_REG (0x0F)

#define MCP9600_ALERT_1_CONFIG_REG (0x08)
#define MCP9600_ALERT_2_CONFIG_REG (0x09)
#define MCP9600_ALERT_3_CONFIG_REG (0x0A)
#define MCP9600_ALERT_4_CONFIG_REG (0x0B)

/* Status REG Bits */
#define MCP9600_STATUS_ALERT1_BIT            (1U << 0)
#define MCP9600_STATUS_ALERT2_BIT            (1U << 1)
#define MCP9600_STATUS_ALERT3_BIT            (1U << 2)
#define MCP9600_STATUS_ALERT4_BIT            (1U << 3)
#define MCP9600_STATUS_TEMP_UPDATE_BIT       (1U << 4)
#define MCP9600_STATUS_BURST_COMPLETE_BIT    (1U << 7)

/* Thermo Sensor Config Register */
#define MCP9600_SENSOR_CONFIG_TC_TYPE_POS    (4U)
#define MCP9600_SENSOR_CONFIG_TC_TYPE_MASK   (0x07U << MCP9600_SENSOR_CONFIG_TC_TYPE_POS)

#define MCP9600_SENSOR_CONFIG_FILTER_POS     (0U)
#define MCP9600_SENSOR_CONFIG_FILTER_MASK    (0x0FU << MCP9600_SENSOR_CONFIG_FILTER_POS)

/* Device Config Register */
#define MCP9600_DEVICE_CFG_SENSOR_RES_POS    (7U)
#define MCP9600_DEVICE_CFG_SENSOR_RES_MASK   (0x01U << MCP9600_DEVICE_CFG_SENSOR_RES_POS)

#define MCP9600_DEVICE_CFG_ADC_RES_POS       (5U)
#define MCP9600_DEVICE_CFG_ADC_RES_MASK      (0x03U << MCP9600_DEVICE_CFG_ADC_RES_POS)

#define MCP9600_DEVICE_CONFIG_TEMP_SAMPLES_POS  (2U)
#define MCP9600_DEVICE_CONFIG_TEMP_SAMPLES_MASK (0x07U << MCP9600_DEVICE_CONFIG_TEMP_SAMPLES_POS)

#define MCP9600_DEVICE_CONFIG_SHUTDOWN_MODES_POS  (0U)
#define MCP9600_DEVICE_CONFIG_SHUTDOWN_MODES_MASK (0x03U << MCP9600_DEVICE_CONFIG_SHUTDOWN_MODES_POS)

/* Alert Config Register */
#define MCP9600_ALERT_CFG_INT_CLEAR_BIT      (1U << 7)

#define MCP9600_ALERT_CFG_MONITOR_POS        (4U)
#define MCP9600_ALERT_CFG_MONITOR_MASK       (0x01U << MCP9600_ALERT_CFG_MONITOR_POS)

#define MCP9600_ALERT_CFG_DIRECTION_POS      (3U)
#define MCP9600_ALERT_CFG_DIRECTION_MASK     (0x01U << MCP9600_ALERT_CFG_DIRECTION_POS)

#define MCP9600_ALERT_CFG_POLARITY_POS       (2U)
#define MCP9600_ALERT_CFG_POLARITY_MASK      (0x01U << MCP9600_ALERT_CFG_POLARITY_POS)

#define MCP9600_ALERT_CFG_MODE_POS           (1U)
#define MCP9600_ALERT_CFG_MODE_MASK          (0x01U << MCP9600_ALERT_CFG_MODE_POS)

#define MCP9600_ALERT_CFG_ENABLE_POS         (0U)
#define MCP9600_ALERT_CFG_ENABLE_MASK        (0x01U << MCP9600_ALERT_CFG_ENABLE_POS)

typedef enum {
    MCP9600_OK = 0,
    MCP9600_ERR,
    MCP9600_INIT_FAIL,
    MCP9600_READ_FAIL,
    MCP9600_WRITE_FAIL
} mcp9600_status_t;

typedef enum {
    MCP9600_THERMOCOUPLE_TYPE_K = 0x0,
    MCP9600_THERMOCOUPLE_TYPE_J = 0x1,
    MCP9600_THERMOCOUPLE_TYPE_T = 0x2,
    MCP9600_THERMOCOUPLE_TYPE_N = 0x3,
    MCP9600_THERMOCOUPLE_TYPE_S = 0x4,
    MCP9600_THERMOCOUPLE_TYPE_E = 0x5,
    MCP9600_THERMOCOUPLE_TYPE_B = 0x6,
    MCP9600_THERMOCOUPLE_TYPE_R = 0x7
} mcp9600_thermocouple_type_t;

typedef enum {
    MCP9600_FILTER_0 = 0x0,
    MCP9600_FILTER_1 = 0x1,
    MCP9600_FILTER_2 = 0x2,
    MCP9600_FILTER_3 = 0x3,
    MCP9600_FILTER_4 = 0x4,
    MCP9600_FILTER_5 = 0x5,
    MCP9600_FILTER_6 = 0x6,
    MCP9600_FILTER_7 = 0x7
} mcp9600_filter_t;

typedef enum {
    MCP9600_SENSOR_TEMP_RESOLUTION_HIGH = 0x0,
    MCP9600_SENSOR_TEMP_RESOLUTION_LOW  = 0x1
} mcp9600_sensor_resolution_t;

typedef enum {
    MCP9600_ADC_RESOLUTION_18BIT = 0x0,
    MCP9600_ADC_RESOLUTION_16BIT = 0x1,
    MCP9600_ADC_RESOLUTION_14BIT = 0x2,
    MCP9600_ADC_RESOLUTION_12BIT = 0x3
} mcp9600_adc_resolution_t;

typedef enum {
    MCP9600_BURST_MODE_SAMPLES_1   = 0x0,
    MCP9600_BURST_MODE_SAMPLES_2   = 0x1,
    MCP9600_BURST_MODE_SAMPLES_4   = 0x2,
    MCP9600_BURST_MODE_SAMPLES_8   = 0x3,
    MCP9600_BURST_MODE_SAMPLES_16  = 0x4,
    MCP9600_BURST_MODE_SAMPLES_32  = 0x5,
    MCP9600_BURST_MODE_SAMPLES_64  = 0x6,
    MCP9600_BURST_MODE_SAMPLES_128 = 0x7
} mcp9600_burst_mode_samples_t;

typedef enum {
    MCP9600_OPERATION_MODE_NORMAL   = 0x0,
    MCP9600_OPERATION_MODE_SHUTDOWN = 0x1,
    MCP9600_OPERATION_MODE_BURST    = 0x2
} mcp9600_operation_mode_t;

typedef enum {
    MCP9600_ALERT_1 = 0,
    MCP9600_ALERT_2 = 1,
    MCP9600_ALERT_3 = 2,
    MCP9600_ALERT_4 = 3
} mcp9600_alert_id_t;

typedef enum {
    MCP9600_ALERT_MONITOR_TH = 0x0,
    MCP9600_ALERT_MONITOR_TC = 0x1
} mcp9600_alert_monitor_t;

typedef enum {
    MCP9600_ALERT_FALLING = 0x0,
    MCP9600_ALERT_RISING  = 0x1
} mcp9600_alert_direction_t;

typedef enum {
    MCP9600_ALERT_ACTIVE_LOW  = 0x0,
    MCP9600_ALERT_ACTIVE_HIGH = 0x1
} mcp9600_alert_polarity_t;

typedef enum {
    MCP9600_ALERT_COMPARATOR = 0x0,
    MCP9600_ALERT_INTERRUPT  = 0x1
} mcp9600_alert_mode_t;

typedef enum {
    MCP9600_ALERT_DISABLED = 0x0,
    MCP9600_ALERT_ENABLED  = 0x1
} mcp9600_alert_enable_t;

typedef struct {
    uint8_t device_addr;
    I2C_HandleTypeDef *hi2c;
    uint8_t thermocouple_type;
    uint8_t filter;
    uint8_t adc_resolution;
    SemaphoreHandle_t mcp_i2c_smphr;
} MCP9600_HandleTypeDef;

typedef struct {
    uint8_t alert_temp;
    uint8_t alert_hysteresis;
    uint8_t alert_config;
} MCP9600_alert_t;


mcp9600_status_t mcp9600_init(MCP9600_HandleTypeDef *handle, I2C_HandleTypeDef *hi2c, uint8_t addr);

mcp9600_status_t mcp9600_shutdown(MCP9600_HandleTypeDef *handle, TickType_t delay);

mcp9600_status_t mcp9600_set_thermocouple_type(MCP9600_HandleTypeDef *handle, uint8_t thermocouple_type, TickType_t delay);
mcp9600_status_t mcp9600_set_filter(MCP9600_HandleTypeDef *handle, uint8_t filter, TickType_t delay);
mcp9600_status_t mcp9600_set_adc_resolution(MCP9600_HandleTypeDef *handle, uint8_t adc_resolution, TickType_t delay);
mcp9600_status_t mcp9600_set_sensor_resolution(MCP9600_HandleTypeDef *handle, uint8_t sensor_resolution, TickType_t delay);
mcp9600_status_t mcp9600_set_burst_mode(MCP9600_HandleTypeDef *handle, uint8_t burst_mode, TickType_t delay);
mcp9600_status_t mcp9600_set_operation_mode(MCP9600_HandleTypeDef *handle, uint8_t operation_mode, TickType_t delay);

mcp9600_status_t mcp9600_alert_monitor(MCP9600_HandleTypeDef *handle, uint8_t alert_monitor, TickType_t delay);
mcp9600_status_t mcp9600_alert_direction(MCP9600_HandleTypeDef *handle, uint8_t alert_direction, TickType_t delay);
mcp9600_status_t mcp9600_alert_polarity(MCP9600_HandleTypeDef *handle, uint8_t alert_polarity, TickType_t delay);
mcp9600_status_t mcp9600_alert_mode(MCP9600_HandleTypeDef *handle, uint8_t alert_mode, TickType_t delay);
mcp9600_status_t mcp9600_alert_enable(MCP9600_HandleTypeDef *handle, uint8_t alert_enable, TickType_t delay);

mcp9600_status_t mcp9600_read_reg(MCP9600_HandleTypeDef *handle, uint8_t reg, volatile uint8_t *value, TickType_t delay);
mcp9600_status_t mcp9600_write_reg(MCP9600_HandleTypeDef *handle, uint8_t reg, uint8_t value, TickType_t delay);

mcp9600_status_t mcp9600_read_hot_junction(MCP9600_HandleTypeDef *handle, int32_t *temp_int, int32_t *temp_frac, uint16_t *raw_temperature, TickType_t delay);

extern volatile uint8_t mcp_i2c_tx_done;
extern volatile uint8_t mcp_i2c_rx_done;
extern volatile uint8_t mcp_i2c_error;