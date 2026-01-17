#pragma once
#include "mbed.h"
#include <cstdint>
#include <Sensor/Sensor.hpp>

class Mcp96L00Sensor : public Sensor {
public:
    explicit Mcp96L00Sensor(I2C *bus, EventQueue *queue = nullptr, void (*processFnc)(float)=nullptr)
        : Sensor(queue, processFnc), i2c(bus) {}

    void configure_k_type_defaults() {      
    
    }

private:
    I2C *i2c = nullptr;
    bool initialized = false;

    static constexpr uint8_t ADDR_7BIT = 0x60; 

    
    static constexpr uint8_t REG_HOT_JUNCTION = 0x00; 

    void readN(uint8_t reg, char *out, int n) {
        char r = static_cast<char>(reg);
        i2c->write(ADDR_7BIT << 1, &r, 1, true);
        i2c->read(ADDR_7BIT << 1, out, n);
    }

    virtual void _sampleData() override {
        if (!initialized) {
            configure_k_type_defaults();
        }

        char d[2] = {0,0};
        readN(REG_HOT_JUNCTION, d, 2);

        int16_t raw = static_cast<int16_t>((static_cast<uint8_t>(d[0]) << 8) |
                                           static_cast<uint8_t>(d[1]));

        
        float tempC = static_cast<float>(raw) * _______f;
        data = tempC;
    }
};