#include "mbed.h"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <Misc/ComIds.hpp>
#include <Misc/Errors.hpp>

#include "BoardPins.hpp"
#include "IrradI2cSensor.hpp"
#include "Mcp96L00Sensor.hpp"


#define PRELUDE 0xFF
#define SAMPLE_FREQ_HZ_IRRAD 10
#define SAMPLE_FREQ_HZ_TEMP  2
#define QUEUE_SIZE 100

static EventQueue g_queue(QUEUE_SIZE * EVENTS_EVENT_SIZE);

// LEDs
static DigitalOut g_ledHeartbeat(PIN_LED_HEARTBEAT);
static DigitalOut g_led1(PIN_LED_DB_CONN_1);
static DigitalOut g_led2(PIN_LED_DB_CONN_2);
static DigitalOut g_led3(PIN_LED_DB_CONN_3);

// Comm
static BufferedSerial g_serial(PIN_USB_TX, PIN_USB_RX);
static CAN g_can(PIN_CAN_RX, PIN_CAN_TX);

// I2C
static I2C g_i2c(PIN_I2C1_SDA, PIN_I2C1_SCL);

// INT from TSL2591 breakout weird idk abt this
static InterruptIn g_int(PIN_INT);

static LowPowerTicker g_tickHeartbeat;

// Forward decl
static void pollCan();
static void processIrradianceResult(float data);
static void processTemperatureResult(float data);
static void processError(uint16_t msgId, uint16_t errorCode, uint16_t errorContext);

// Sensors
static IrradI2cSensor g_tsl(&g_i2c, &g_queue, &processIrradianceResult);
static Mcp96L00Sensor g_tc (&g_i2c, &g_queue, &processTemperatureResult);

// INT handler (never do I2C inside ISR)
static volatile uint32_t g_tslIntCount = 0;
static void onTslInterruptRiseFall() {
    g_tslIntCount++;
    // Schedule a safe job to read a fresh sample and clear latched INT.
    g_queue.call([](){
        // A one-off sample now 
        g_tsl.sampleOnce();   // may need to replace for sensor
        g_tsl.clearInterrupt();
    });
}

static void heartbeat() {
    g_ledHeartbeat = !g_ledHeartbeat;
}

static void cycleLed(DigitalOut &dout, int cycles, milliseconds dly) {
    for (int i = 0; i < cycles; i++) {
        dout = 1;
        ThisThread::sleep_for(dly);
        dout = 0;
        ThisThread::sleep_for(dly);
    }
}

int main() {
    g_serial.set_baud(9600);
    g_serial.set_format(8, BufferedSerial::None, 1);

    // I2C 400kHz
    g_i2c.frequency(400000);

    // Boot LEDs
    cycleLed(g_led1, 2, 100ms);
    cycleLed(g_led2, 2, 100ms);
    cycleLed(g_led3, 2, 100ms);

    if (ENABLE_HEARTBEAT) {
        g_tickHeartbeat.attach(&heartbeat, 1s);
    }

    // Configure sensors
    g_tc.configure_k_type_defaults();

    if (ENABLE_TSL_INT) {
        // Configure the TSL INT thresholds for saturation warning
        g_tsl.configure_interrupt_saturation_warning();

        // TSL INT is typically active-low open-drain; trigger on falling edge
        g_int.fall(&onTslInterruptRiseFall);
    }

    // Start periodic sampling 
    g_tsl.start(milliseconds(1000 / SAMPLE_FREQ_HZ_IRRAD));
    g_tc.start (milliseconds(1000 / SAMPLE_FREQ_HZ_TEMP));

    // Dispatch queue forever
    Thread eventThread;
    eventThread.start(callback(&g_queue, &EventQueue::dispatch_forever));

    while (true) {
        pollCan();
        ThisThread::sleep_for(100ms);
    }
}

/* ------------------ CAN RX ------------------ */
static void pollCan() {
    static CANMessage msg;
    if (!g_can.read(msg)) return;

    const uint16_t msgId = msg.id;

    switch (msgId) {
        case BLKBDY_EN_DIS:
            if (msg.len != 1) {
                processError(BLKBDY_FAULT, ERR_INVALID_MSG_DATA_LEN, msg.len);
                return;
            }
            if (msg.data[0] == 0) {
                g_tsl.start(milliseconds(1000 / SAMPLE_FREQ_HZ_IRRAD));
                g_tc.start (milliseconds(1000 / SAMPLE_FREQ_HZ_TEMP));
            } else if (msg.data[0] == 1) {
                g_tsl.stop();
                g_tc.stop();
            } else {
                processError(BLKBDY_FAULT, ERR_INVALID_MSG_DATA, msg.data[0]);
            }
            break;

        default:
            break;
    }
}

/* ------------------ CAN TX ------------------ */
static void processIrradianceResult(float data) {
    // Legacy format: value * 1000 in uint64, send 5 bytes
    const uint64_t value = static_cast<uint64_t>(data * 1000.0f);

    char payload[5];
    std::memcpy(payload, &value, 5);
    g_can.write(CANMessage(BLKBDY_IRRAD_1_MEAS, payload, 5));

    printf("%02x%04x%10llx", PRELUDE, BLKBDY_IRRAD_1_MEAS, value);
}

static void processTemperatureResult(float data) {
    // milli-degC
    const uint32_t value = static_cast<uint32_t>(data * 1000.0f);

    char payload[4];
    std::memcpy(payload, &value, 4);
    g_can.write(CANMessage(BLKBDY_TEMP_MEAS, payload, 4));

    printf("%02x%04x%08x", PRELUDE, BLKBDY_TEMP_MEAS, value);
}

static void processError(uint16_t msgId, uint16_t errorCode, uint16_t errorContext) {
    const uint16_t value = static_cast<uint16_t>(((errorCode & 0xFF) << 8) | (errorContext & 0xFF));

    char payload[2];
    std::memcpy(payload, &value, 2);
    g_can.write(CANMessage(msgId, payload, 2));

    printf("%02x%04x%04x", PRELUDE, msgId, value);
}