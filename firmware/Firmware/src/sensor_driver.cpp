#include "sensor_driver.hpp"

#define ADXL345_REG_DEVID       0x00
#define ADXL345_REG_BW_RATE     0x2C
#define ADXL345_REG_POWER_CTL   0x2D
#define ADXL345_REG_FIFO_CTL    0x38
#define ADXL345_REG_FIFO_STATUS 0x39
#define ADXL345_REG_DATAX0      0x32

SensorDriver::SensorDriver(uint8_t cs_pin) : _cs_pin(cs_pin) {}

bool SensorDriver::begin() {
    pinMode(_cs_pin, OUTPUT);
    digitalWrite(_cs_pin, HIGH);
    
    // Inicializar SPI explícitamente con los pines físicos del ESP32 (SCK, MISO, MOSI, SS)
    SPI.begin(18, 19, 23, _cs_pin);
    SPI.setClockDivider(SPI_CLOCK_DIV16); // Reducir velocidad a ~5MHz o menos por seguridad
    SPI.setDataMode(SPI_MODE3);          // ADXL345 soporta Modo 0 o Modo 3
    
    uint8_t devid = readRegister(ADXL345_REG_DEVID);
    if (devid != 0xE5) {
        return false; // Aquí es donde está fallando y devolviendo el error
    }

    writeRegister(ADXL345_REG_POWER_CTL, 0x00);
    setODR_100Hz();
    writeRegister(ADXL345_REG_FIFO_CTL, 0x80 | 0x10); // Stream mode, watermark = 16
    writeRegister(ADXL345_REG_POWER_CTL, 0x08);

    return true;
}

bool SensorDriver::setODR_100Hz() {
    writeRegister(ADXL345_REG_BW_RATE, 0x0A); // 100 Hz
    return true;
}

uint8_t SensorDriver::getFifoEntries() {
    uint8_t status = readRegister(ADXL345_REG_FIFO_STATUS);
    return status & 0x3F;
}

bool SensorDriver::readFifoBlock(RawSample* buffer, uint8_t num_samples) {
    uint8_t reg = ADXL345_REG_DATAX0 | 0x80 | 0x40;
    
    digitalWrite(_cs_pin, LOW);
    SPI.transfer(reg);
    for (uint8_t i = 0; i < num_samples; i++) {
        uint8_t raw_data[6];
        for (uint8_t j = 0; j < 6; j++) {
            raw_data[j] = SPI.transfer(0x00);
        }
        buffer[i].x = (int16_t)((raw_data[1] << 8) | raw_data[0]);
        buffer[i].y = (int16_t)((raw_data[3] << 8) | raw_data[2]);
        buffer[i].z = (int16_t)((raw_data[5] << 8) | raw_data[4]);
    }
    digitalWrite(_cs_pin, HIGH);
    return true;
}

void SensorDriver::writeRegister(uint8_t reg, uint8_t value) {
    digitalWrite(_cs_pin, LOW);
    SPI.transfer(reg & 0x7F);
    SPI.transfer(value);
    digitalWrite(_cs_pin, HIGH);
}

uint8_t SensorDriver::readRegister(uint8_t reg) {
    digitalWrite(_cs_pin, LOW);
    SPI.transfer(reg | 0x80);
    uint8_t value = SPI.transfer(0x00);
    digitalWrite(_cs_pin, HIGH);
    return value;
}