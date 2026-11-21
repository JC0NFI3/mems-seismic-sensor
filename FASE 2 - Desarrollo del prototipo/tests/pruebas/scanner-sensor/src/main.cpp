#include <Arduino.h>
#include <Wire.h>

void setup() {
  // Inicializa la comunicación serie a 115200 baudios
  Serial.begin(115200);
  while (!Serial) delay(10); 
  
  // Inicializa el bus I2C en los pines por defecto del ESP32 (SDA = GPIO 21, SCL = GPIO 22)
  Wire.begin(21, 22);
  
  Serial.println("\n--- Buscando dispositivos I2C ---");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Dispositivo I2C encontrado en la dirección 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      if (address == 0x53) {
        Serial.println("-> ¡Excelente! Es el ADXL345 configurado correctamente.");
      }
      nDevices++;
    } else if (error == 4) {
      Serial.print("Error desconocido en la dirección 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (nDevices == 0) {
    Serial.println("No se encontraron dispositivos I2C.\nRevisa VCC, GND, CS, SDA y SCL.");
  } else {
    Serial.println("Escaneo finalizado.");
  }

  delay(3000); // Vuelve a escanear cada 3 segundos
}