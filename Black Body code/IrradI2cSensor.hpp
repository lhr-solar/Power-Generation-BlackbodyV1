#pragma once
#include "mbed.h"
#include <cstdint>
#include <Sensor/Sensor.hpp>

class IrradI2cSensor : public Sensor {
public:
    explicit IrradI2cSensor(I2C *bus, EventQueue *queue = nullptr, void (*processFnc)(float)=nullptr)
        : Sensor(queue, processFnc), i2c(bus) {}

    // Call once at boot (optional) to configure thresholds for INT.
    // This configures an "approaching saturation" interrupt on ALS channel 0.
    void configure_interrupt_saturation_warning() {
        initIfNeeded();

        // Interrupt thresholds are 16-bit.
        // Set low = 0 (ignore low-light)
        // TSL2591 full-scale is 0xFFFF
        write16(REG_AILTL, 0x0000);
        write16(REG_AIHTL, 0xF000);

        // Persistence: how many consecutive out-of-range cycles before INT.
        // 0x01 => 1 cycle (fast response) can change if needed
        write8(REG_PERSIST, 0x01);

        // Enable ALS interrupt
        uint8_t enable = read8(REG_ENABLE);
        enable |= EN_AIEN;
        write8(REG_ENABLE, enable);

        // Clear any existing interrupt state
        clearInterrupt();
    }

    
    void clearInterrupt() {
        // 0xE7 is the "CLEAR" command for TSL2591 (command register form).
        char cmd = static_cast<char>(CMD_SPECIAL_CLEAR);
        i2c->write(ADDR_7BIT << 1, &cmd, 1);
    }

private:
    I2C *i2c = nullptr;
    bool initialized = false;

    // TSL2591 constants
    static constexpr uint8_t ADDR_7BIT = 0x29;

    // Command formats
    static constexpr uint8_t CMD_BIT   = 0xA0; 
    static constexpr uint8_t CMD_SPECIAL_CLEAR = 0xE7; // "clear interrupt" special function

    // Registers
    static constexpr uint8_t REG_ENABLE   = 0x00;
    static constexpr uint8_t REG_CONTROL  = 0x01;
    static constexpr uint8_t REG_PERSIST  = 0x0C;

    static constexpr uint8_t REG_AILTL    = 0x04; // ALS INT low threshold low byte (0x04/0x05)
    static constexpr uint8_t REG_AIHTL    = 0x06; // ALS INT high threshold low byte (0x06/0x07)

    static constexpr uint8_t REG_CHAN0_LO = 0x14; // CH0 low; auto-inc reads CH0+CH1

    // ENABLE bits
    static constexpr uint8_t EN_PON  = 0x01;
    static constexpr uint8_t EN_AEN  = 0x02;
    static constexpr uint8_t EN_AIEN = 0x10;

    // CONTROL bits (gain + integration time)
    static constexpr uint8_t CONTROL_GAIN_MED = 0x20; // gain bits
    static constexpr uint8_t CONTROL_IT_200MS = 0x01; // integration bits

    void initIfNeeded() {
        if (initialized) return;

        // Power on
        write8(REG_ENABLE, EN_PON);
        ThisThread::sleep_for(5ms);

        // Enable ALS
        write8(REG_ENABLE, EN_PON | EN_AEN);

        // Set control (gain + integration)
        write8(REG_CONTROL, CONTROL_GAIN_MED | CONTROL_IT_200MS);

        initialized = true;
    }

    void write8(uint8_t reg, uint8_t val) {
        char buf[2] = { static_cast<char>(CMD_BIT | reg), static_cast<char>(val) };
        i2c->write(ADDR_7BIT << 1, buf, 2);
    }

    uint8_t read8(uint8_t reg) {
        char r = static_cast<char>(CMD_BIT | reg);
        i2c->write(ADDR_7BIT << 1, &r, 1, true);
        char v = 0;
        i2c->read(ADDR_7BIT << 1, &v, 1);
        return static_cast<uint8_t>(v);
    }

    void write16(uint8_t reg_low, uint16_t val) {
        char buf[3] = {
            static_cast<char>(CMD_BIT | reg_low),
            static_cast<char>(val & 0xFF),
            static_cast<char>((val >> 8) & 0xFF),
        };
        i2c->write(ADDR_7BIT << 1, buf, 3);
    }

    virtual void _sampleData() override {
        initIfNeeded();

        // Read CH0 and CH1 (4 bytes)
        char r = static_cast<char>(CMD_BIT | REG_CHAN0_LO);
        i2c->write(ADDR_7BIT << 1, &r, 1, true);

        char d[4] = {0,0,0,0};
        i2c->read(ADDR_7BIT << 1, d, 4);

        uint16_t ch0 = static_cast<uint16_t>(static_cast<uint8_t>(d[0]) |
                                             (static_cast<uint16_t>(static_cast<uint8_t>(d[1])) << 8));
        uint16_t ch1 = static_cast<uint16_t>(static_cast<uint8_t>(d[2]) |
                                             (static_cast<uint16_t>(static_cast<uint8_t>(d[3])) << 8));

        // Simple robust derived metric:
        // Use "visible" = CH0 - CH1 (common trick) clipped at 0.
        int32_t vis = static_cast<int32_t>(ch0) - static_cast<int32_t>(ch1);
        if (vis < 0) vis = 0;

        
        // Can later calibrate counts
        data = static_cast<float>(vis);
    }
};