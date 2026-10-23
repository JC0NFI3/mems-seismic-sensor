#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// Asigna un ID único al sensor
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

void setup(void) {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("--- Lectura Continua del ADXL345 ---");

  // Inicializa I2C en los pines SDA = GPIO 21 y SCL = GPIO 22 del ESP32
  Wire.begin(21, 22);

  // Inicializa el sensor
  if (!accel.begin()) {
    Serial.println("¡Error! No se detectó el ADXL345. Revisa las conexiones.");
    while (1);
  }

  // Configura la sensibilidad a +/- 2g (máxima resolución para pequeños movimientos)
  accel.setRange(ADXL345_RANGE_2_G);

  Serial.println("ADXL345 listo. Leyendo datos...");
  Serial.println("----------------------------------------");
}

void loop(void) {
  // Obtiene una nueva lectura del sensor
  sensors_event_t event;
  accel.getEvent(&event);

  // Imprime los datos en m/s^2
  Serial.print("X: "); Serial.print(event.acceleration.x, 2); Serial.print(" m/s^2 | ");
  Serial.print("Y: "); Serial.print(event.acceleration.y, 2); Serial.print(" m/s^2 | ");
  Serial.print("Z: "); Serial.print(event.acceleration.z, 2); Serial.print(" m/s^2");
  Serial.println();

  delay(200); // Muestra 5 lecturas por segundo
}