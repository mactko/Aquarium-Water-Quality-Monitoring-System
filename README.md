# Aquarium Water Quality Monitoring System

A final project for the Faculty of Electrical Engineering and Computing, University of Zagreb.

**Author:** Matko Marjanac  
**Mentor:** Prof. dr. sc. Hrvoje Džapo

---

## Overview

This project implements an embedded system for continuous monitoring and maintenance of water quality in an aquarium. The system measures **water temperature** and **turbidity**, automatically controlling a water pump and filter to maintain optimal conditions for aquatic life. Sensor data is transmitted wirelessly to Microsoft Azure cloud storage for remote monitoring.

### Key Features

- Continuous temperature measurement via DS18B20 sensor (1-Wire protocol)
- Water turbidity measurement via SEN0189 sensor with median + deadzone digital filtering
- Automatic pump speed regulation based on turbidity readings (PWM via LEDC peripheral)
- Wi-Fi connectivity with robust reconnection handling (FreeRTOS Event Groups)
- Secure MQTT data transmission to Azure Event Grid (MQTT over TLS using mbedTLS)
- Cloud data storage in Azure Table Storage, triggered via Service Bus and Function App

---

## Hardware

| Component | Description |
|---|---|
| ESP32-DevKitC (ESP32-WROOM-32E) | Main microcontroller — dual-core Xtensa LX6, Wi-Fi, BLE |
| DS18B20 | Waterproof digital temperature sensor, 1-Wire interface |
| SEN0189 | Analog water turbidity sensor (DF Robot), connected to ADC1 channel (GPIO33) |
| Water pump | DC pump controlled via MOSFET transistor switch + PWM |
| 4-stage filter | Mechanical filtration unit driven by the pump |
| 12V DC adapter | Powers the pump directly |
| 5V linear regulator | Steps down 12V to 5V for MCU and sensors |

---

## Project Structure

```
project-root/
│
├── Slike/                  # Project images and diagrams
├── Text/                   # Project documentation (thesis)
├── 3D modeli/              # 3D-printed enclosure models
│
└── Programski kod/
    ├── ESP/                # ESP32 firmware (ESP-IDF)
    │   ├── CMakeLists.txt
    │   ├── README.md
    │   ├── sdkconfig
    │   ├── build/
    │   ├── components/     # Hardware drivers
    │   │   ├── wifi/       # Wi-Fi connection management
    │   │   ├── ds18b20/    # DS18B20 temperature sensor driver (1-Wire)
    │   │   ├── pump/       # Pump control via LEDC PWM
    │   │   └── turbidity/  # SEN0189 turbidity sensor driver + filtering
    │   ├── main/
    │   │   ├── main.c      # Application entry point
    │   │   ├── keys/       # TLS private keys (user-provided)
    │   │   └── certificates/ # TLS certificates (user-provided)
    │   ├── .devcontainer/
    │   └── .vscode/
    │
    └── Azure/              # Azure cloud-side components
        └── Function App/   # readtelemetrydata() — Service Bus queue consumer
                            # Decodes Base64 payload, writes to Table Storage
```

---

## Getting Started

### Prerequisites

- [ESP-IDF v5.x](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/) installed and configured
- A Microsoft Azure account with the following resources provisioned:
  - **Event Grid Namespace** (MQTT broker with X.509 authentication enabled)
  - **Event Grid Topic** (telemetry routing)
  - **Service Bus Namespace + Queue** (`mqttamsqueue`)
  - **Function App** (`readtelemetrydata`)
  - **Storage Account + Table** (`mqtttelemetrydata`)

---

### Azure Setup

#### 1. Generate TLS Certificates

Use OpenSSL to generate the required keys and certificates:

```bash
# Generate CA key and certificate
openssl genrsa -out ca.key 2048
openssl req -new -x509 -days 365 -key ca.key -out brokerCert.pem

# Generate device key and certificate signing request
openssl genrsa -out deviceCert.key 2048
openssl req -new -key deviceCert.key -out device.csr

# Sign the device certificate with the CA
openssl x509 -req -days 365 -in device.csr -CA brokerCert.pem -CAkey ca.key \
  -CAcreateserial -out deviceCert.pem
```

#### 2. Register the Device Certificate in Azure

Upload `deviceCert.pem` to your Azure Event Grid Namespace under **Clients → Certificates** and create a client entry for your ESP32.

#### 3. Place Certificates in the Firmware

Copy the generated files into the ESP firmware directory:

```
ESP/main/certificates/brokerCert.pem    ← CA / broker certificate
ESP/main/certificates/deviceCert.pem   ← Device certificate
ESP/main/keys/deviceCert.key            ← Device private key
```

These files are embedded into flash memory at build time via `target_add_binary_data()` in `CMakeLists.txt` and stored in `.rodata`.

---

### Firmware Configuration

Open `sdkconfig` or use `idf.py menuconfig` to set:

```
CONFIG_WIFI_SSID      → Your Wi-Fi network name
CONFIG_WIFI_PASSWORD  → Your Wi-Fi password
CONFIG_MQTT_BROKER    → Your Azure Event Grid MQTT broker hostname
CONFIG_MQTT_PORT      → 8883 (MQTT over TLS)
```

Alternatively, these can be defined directly as constants in the source if menuconfig is not used.

---

### Build and Flash

```bash
# Navigate to the ESP firmware directory
cd "Programski kod/ESP"

# Build the project
idf.py build

# Flash to the connected ESP32-DevKitC
idf.py -p <PORT> flash

# Monitor serial output
idf.py -p <PORT> monitor
```

---

## System Architecture

![System Block Diagram](Blok_shema.png)

The ESP32-DevKitC sits at the centre of the system. It reads the SEN0189 turbidity sensor via ADC and the DS18B20 temperature sensor over the 1-Wire bus. Based on the turbidity reading it drives the pump and filter via a PWM signal. Sensor data is published to the Azure Event Grid MQTT broker over a TLS-secured connection, from where it flows through an Event Grid Topic and Service Bus Queue into a Function App that stores the values in Azure Table Storage.

---

## Data Flow

1. The ESP32 reads temperature (DS18B20 over 1-Wire) and turbidity (SEN0189 via ADC1) every 60 seconds.
2. Turbidity readings are filtered using a **median filter** (removes impulse noise from suspended particles) followed by a **deadzone filter** (suppresses sensor noise drift).
3. Pump speed is adjusted via **PWM (LEDC peripheral)** based on the filtered turbidity value.
4. Sensor data is formatted as `"<temperature>|<turbidity>"` and published to the Azure Event Grid MQTT broker over a TLS-secured connection.
5. The Event Grid routes incoming messages through a Service Bus Queue to a Function App, which decodes the Base64-encoded payload and writes the values to Azure Table Storage.

---

## Telemetry Payload Format

Messages are published to the topic `telemetry/mydevice/temperature` with the following format:

```
<temperature_float>|<turbidity_int>

Example: 27.500000|234
```

---

## Hardware Schematics

### SEN0189 Turbidity Sensor Circuit

The SEN0189 uses an IR LED (D3) and a photo-NPN transistor (Q1) to measure light attenuation through water. Two LMV358 op-amp stages (U1A, U1B) condition the signal, providing both a digital threshold output (`D_out`) and an analog voltage output (`A_out`) that is fed into the ESP32 ADC1 channel on GPIO33.

![SEN0189 Schematic](SEN0189_shema.png)

### Pump Motor Driver Circuit

The pump is controlled by a MOSFET transistor switch (Q2) driven from GPIO D18 through a 470 Ω gate resistor (R10). A 100 µF capacitor (C4) across the motor suppresses voltage spikes when the inductive load switches off. The motor runs directly from the 12 V supply rail.

![Transistor Switch Schematic](tranzistorska_sklopka_shema.png)

---

## Test Setup

The demonstrator consists of a small acrylic aquarium, the 3D-printed control enclosure, and a 4-stage filter housing — all assembled and tested for 12 continuous hours.

![Demonstrator / Test Setup](Pokazni_model.jpg)

Key observations from testing:
- Sensor data was successfully read and uploaded to Azure Table Storage every 60 seconds throughout the test.
- The system recovered automatically from sudden Wi-Fi and MQTT broker disconnections without requiring a reboot.
- The two-stage digital filter (median + deadzone) effectively suppressed impulse noise caused by suspended particles passing in front of the turbidity sensor.
- Sedimentation of suspended solids over time produces a gradual drift in turbidity readings that cannot be fully compensated by filtering alone — periodic sensor cleaning is recommended.

---

## Troubleshooting

| Issue | Likely Cause | Solution |
|---|---|---|
| Device fails to connect to Wi-Fi | Incorrect credentials or AP out of range | Verify `SSID` and `PASSWORD` in config; check signal strength |
| MQTT connection refused | Certificate mismatch or device not registered in Azure | Regenerate certificates; re-register the client in Event Grid |
| Turbidity reads 0 or negative | SEN0189 not immersed or cable disconnected | Check wiring to GPIO33; ensure sensor is submerged |
| Temperature reads 85°C | DS18B20 power-up default (no conversion completed) | Check pull-up resistor on data line; verify 1-Wire bus |
| No data in Table Storage | Function App not triggered or parsing error | Check Service Bus Queue backlog; review Function App logs in Azure Portal |

---

## License

This project was developed as a final undergraduate thesis at the Faculty of Electrical Engineering and Computing, University of Zagreb. All rights reserved by the author.
