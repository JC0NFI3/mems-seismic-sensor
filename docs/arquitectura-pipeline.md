
```mermaid
graph TD
    A[ADXL345 Sensor] --> B(Procesador ESP32)
graph TD
    classDef hardware fill:#e1f5fe,stroke:#01579b,stroke-width:2px;
    classDef firmware fill:#e8f5e9,stroke:#2e7d32,stroke-width:2px;
    classDef storage fill:#fff3e0,stroke:#e65100,stroke-width:2px;
    classDef python fill:#f3e5f5,stroke:#4a148c,stroke-width:2px;

    subgraph Hardware Layer ["Capa de Hardware y Adquisición"]
        A[ADXL345 / ADXL355 Sensor] -->|Interrupción DRDY / FIFO Watermark| B(Capa de Abstracción del Sensor / HAL)
        C[Módulo SNTP / GPS+PPS] -->|Timestamp UTC| D(Sincronización Temporal)
    end

    subgraph Firmware Layer ["Capa de Firmware (ESP32 / PlatformIO)"]
        B -->|Muestreo Estable 100/200Hz| E[Buffer Circular en RAM]
        D -->|Bloque de Datos + Marca Temporal| E
        E -->|Escritura por Bloques| F[(Tarjeta MicroSD - Archivo Binario/CSV)]
        E -->|Serialización por Bloques| G[Cliente MQTT / WiFi]
    end

    subgraph Pipeline Analítico en PC ["Pipeline Analítico y Validación (Python / ObsPy)"]
        F -->|Descarga de Registros Crudos| H[Conversión a MiniSEED via ObsPy]
        G -->|Telemetría / Métricas de Red| I[Monitoreo de Paquetes y Latencia]
        H -->|Calibración 6 Posiciones & Conversión a m/s²| J[Corrección de Offset y Ganancia]
        J -->|Prueba de Ruido Nocturno 30-60 min| K[Estimación de PSD / Welch vs Datasheet]
        J -->|Excitación Controlada / Impacto| L[Verificación de FFT y Picos]
        J -->|Señales de Referencia| M[Implementación de Algoritmo STA/LTA Offline]
    end

    class A,C hardware;
    class B,E,G firmware;
    class F storage;
    class H,I,J,K,L,M python;