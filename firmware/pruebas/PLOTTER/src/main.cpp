#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Compensaciones fijas de calibración estática[cite: 1]
const float OFFSET_X = 0.00;
const float OFFSET_Y = 0.55;
const float OFFSET_Z = 0.36;

void setup(void) {
  Serial.begin(115200);
  Wire.begin(21, 22);

  if (!accel.begin()) {
    while (1); // Detiene la ejecución si falla la comunicación I2C[cite: 1]
  }

  // Sensibilidad +/- 2g para máxima resolución[cite: 1]
  accel.setRange(ADXL345_RANGE_2_G);
}

void loop(void) {
  sensors_event_t event;
  accel.getEvent(&event);

  // Aplicación de la corrección estática[cite: 1]
  float x = event.acceleration.x + OFFSET_X;
  float y = event.acceleration.y + OFFSET_Y;
  float z = event.acceleration.z + OFFSET_Z;

  // Formato estricto para el Serial Plotter[cite: 1]
  Serial.print("Acc_X:"); Serial.print(x, 2); Serial.print(",");
  Serial.print("Acc_Y:"); Serial.print(y, 2); Serial.print(",");
  Serial.print("Acc_Z:"); Serial.println(z, 2);

  delay(20); // Ventana de muestreo de ~50 Hz[cite: 1]
}