#ifndef SENSOR_DRIVER_HPP
#define SENSOR_DRIVER_HPP

#include <Arduino.h>
#include <SPI.h>

class SensorDriver {
public:
    struct RawSample {
        int16_t x;
        int16_t y;
        int16_t z;
    };

    SensorDriver(uint8_t cs_pin);
    bool begin();
    bool setODR_100Hz();
    uint8_t getFifoEntries();
    bool readFifoBlock(RawSample* buffer, uint8_t num_samples);

private:
    uint8_t _cs_pin;
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
};

#endif