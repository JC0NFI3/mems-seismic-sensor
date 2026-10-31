#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "sensor_driver.hpp"

// Credenciales de Red
const char* ssid     ="FAMILIA.AQ";
const char* password = "WYR38451";

// Definición de Pines CS (Chip Select)
#define ADXL_CS_PIN  5
#define SD_CS_PIN    4   // Ajusta según el pin físico donde conectes el módulo SD

SensorDriver adxl(ADXL_CS_PIN);

// Configuración NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -5 * 3600, 60000); // UTC-5 (Colombia)

// Servidor Web en puerto 80
WebServer server(80);

// Estructura de bloque binario
struct __attribute__((packed)) DataBlock {
    uint32_t epoch_timestamp;
    uint32_t millis_offset;
    uint8_t  sample_count;
    SensorDriver::RawSample samples[16];
};

File dataFile;
const char* filename = "/seismic_raw.bin";
bool is_recording = true;
unsigned long last_file_sync = 0;

// --- Rutas del Servidor Web ---
void handleRoot() {
    String html = "<html><head><meta charset='utf-8'></head><body style='font-family:Arial; text-align:center; margin-top:50px;'>";
    html += "<h1>Sensor S&iacute;smico ESP32 - ADXL345</h1>";
    html += "<p>Estado en MicroSD: <b>" + String(is_recording ? "Grabando en SD..." : "Pausado") + "</b></p>";
    html += "<p><a href='/download' style='padding:10px 20px; background:#007BFF; color:white; text-decoration:none; border-radius:5px;'>Descargar Archivo BIN de la SD</a></p>";
    html += "<p><a href='/toggle' style='padding:10px 20px; background:#28A745; color:white; text-decoration:none; border-radius:5px;'>" + String(is_recording ? "Pausar" : "Reanudar") + " Grabaci&oacute;n</a></p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleDownload() {
    dataFile.close(); // Cerrar temporalmente para lectura segura
    
    if (SD.exists(filename)) {
        File downloadFile = SD.open(filename, FILE_READ);
        server.streamFile(downloadFile, "application/octet-stream");
        downloadFile.close();
    } else {
        server.send(404, "text/plain", "Archivo no encontrado en la MicroSD.");
    }
    
    if (is_recording) {
        dataFile = SD.open(filename, FILE_APPEND);
    }
}

void handleToggle() {
    is_recording = !is_recording;
    if (is_recording) {
        dataFile = SD.open(filename, FILE_APPEND);
    } else {
        dataFile.close();
    }
    server.sendHeader("Location", "/");
    server.send(303);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // 1. Inicializar Tarjeta MicroSD por SPI
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("Error: Fallo al montar la MicroSD. Revisa las conexiones.");
        // No detenemos el programa para permitir depuración por serial si es necesario
    } else {
        Serial.println("MicroSD montada correctamente.");
        if (SD.exists(filename)) {
            SD.remove(filename); // Limpiar registro anterior opcional
        }
        dataFile = SD.open(filename, FILE_WRITE);
    }

    // 2. Conectar a WiFi y sincronizar NTP
    WiFi.begin(ssid, password);
    Serial.print("Conectando a WiFi");
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500);
        Serial.print(".");
        retries++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi conectado.");
        Serial.print("Panel Web y Descarga en: http://");
        Serial.println(WiFi.localIP());
        
        timeClient.begin();
        timeClient.update();
    } else {
        Serial.println("\nAviso: Sin conexión WiFi, se usará tiempo interno.");
    }

    // Configurar servidor web
    server.on("/", handleRoot);
    server.on("/download", handleDownload);
    server.on("/toggle", handleToggle);
    server.begin();

    // 3. Inicializar Sensor ADXL345 vía SPI
    if (!adxl.begin()) {
        Serial.println("Error: No se encontró el ADXL345 por SPI.");
        while (1) { delay(1000); }
    }
    Serial.println("ADXL345 inicializado a 100 Hz con FIFO Watermark.");
}

void loop() {
    // Atender peticiones HTTP en segundo plano
    server.handleClient();

    if (!is_recording || !dataFile) return;

    // Verificar si hay al menos 16 muestras listas en la FIFO
    uint8_t entries = adxl.getFifoEntries();
    
    if (entries >= 16) {
        DataBlock block;
        
        unsigned long current_ms = millis();
        block.epoch_timestamp = timeClient.isTimeSet() ? timeClient.getEpochTime() : (current_ms / 1000);
        block.millis_offset = current_ms % 1000;
        block.sample_count = 16;

        adxl.readFifoBlock(block.samples, 16);

        // Escribir bloque binario a la MicroSD
        dataFile.write((uint8_t*)&block, sizeof(DataBlock));
        
        // Volcar búfer a disco cada 5 segundos para asegurar integridad
        if (millis() - last_file_sync > 5000) {
            dataFile.flush();
            last_file_sync = millis();
        }
    }
}