#pragma once
#include "mbed.h"

// MCU: STM32G474RE

// ---- Serial 
static constexpr PinName PIN_USB_TX = _____;
static constexpr PinName PIN_USB_RX = _____;

// ---- CAN 
static constexpr PinName PIN_CAN_TX = _____;
static constexpr PinName PIN_CAN_RX = _____;

// ---- I2C1 
static constexpr PinName PIN_I2C1_SDA = _____;
static constexpr PinName PIN_I2C1_SCL = _____;

// ---- Shared interrupt line from breakout Irrad Sensor
static constexpr PinName PIN_INT = _____;

// ---- LEDs ----
static constexpr PinName PIN_LED_HEARTBEAT = _____;
static constexpr PinName PIN_LED_DB_CONN_1  = _____;
static constexpr PinName PIN_LED_DB_CONN_2  = _____;
static constexpr PinName PIN_LED_DB_CONN_3  = _____;

// ---- Feature toggles ----
static constexpr bool ENABLE_HEARTBEAT = true;
static constexpr bool ENABLE_TSL_INT   = true;