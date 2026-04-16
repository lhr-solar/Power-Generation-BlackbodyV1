#pragma once
#include "stm32xx_hal.h"
#include <stdio.h>
#include "UART.h"
#include "printf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOSConfig.h"

//0 to write 1 to read
#define MCP9600_7BIT_ADDR_0 (0x60)
#define MCP9600_7BIT_ADDR_1 (0x61)
#define MCP9600_7BIT_ADDR_2 (0x62)
#define MCP9600_7BIT_ADDR_3 (0x63)
#define MCP9600_7BIT_ADDR_4 (0x64)
#define MCP9600_7BIT_ADDR_5 (0x65)
#define MCP9600_7BIT_ADDR_6 (0x66)
#define MCP9600_7BIT_ADDR_7 (0x67)

#define MCP9600_8BIT_ADDR (MCP9600_7BIT_ADDR << 1)


#define MCP9600_FINAL_TEMP_REG (0x00)
#define MCP9600_CHANGE_TEMP_REG (0x01)
#define MCP9600_COLDJUNC_TEMP_REG (0x02)

#define MCP9600_STATUS_REG (0x04) 
#define MCP9600_SENSOR_CONFIG_REG (0x05)  //controls how chip measures tempurature
#define MCP9600_DEVICE_CONFIG_REG (0x06)  //controls device level settings

#define MCP9600_ALERT_1_REG (0x10)
#define MCP9600_ALERT_2_REG (0x11)
#define MCP9600_ALERT_3_REG (0x12)
#define MCP9600_ALERT_4_REG (0x13)