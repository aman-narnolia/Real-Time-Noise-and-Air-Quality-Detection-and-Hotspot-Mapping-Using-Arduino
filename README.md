# 🌍 Real-Time Noise and Air Quality Detection & Hotspot Mapping

A low-cost, portable environmental monitoring system built using **Arduino Uno**, gas/noise sensors, an **ESP8266 Wi-Fi module**, and an **I2C LCD**. The system measures air-quality and noise-related sensor values in real time, displays the readings locally, and can transmit them to a cloud platform such as **ThingSpeak** for remote monitoring and hotspot visualization.

> **Project:** Real-Time Noise and Air Quality Detection and Hotspot Mapping Using Arduino

## 📌 Overview

Rapid urbanization and industrialization have increased the need for localized, real-time environmental monitoring. Traditional monitoring systems can be expensive, stationary, and unsuitable for capturing small-scale variations.

This project aims to provide an affordable and portable alternative that can:

- Monitor air-quality/pollution-related sensor values.
- Monitor environmental noise levels.
- Display readings on an LCD in real time.
- Send readings over Wi-Fi using an ESP8266.
- Upload data to a cloud IoT platform.
- Support hotspot mapping when location/GPS data is available.

The project presentation describes the system as a low-cost, portable, real-time monitoring solution for urban environments.

## ✨ Features

- 📊 Real-time air-quality monitoring
- 🔊 Noise-level monitoring
- 🖥️ 16×2 I2C LCD output
- 📡 ESP8266 Wi-Fi connectivity
- ☁️ ThingSpeak cloud integration
- 📍 Support for pollution/noise hotspot mapping
- 💻 Serial Monitor output for debugging
- 🔧 Simple Arduino-based hardware architecture

## 🧰 Hardware Components

| Component | Purpose |
|---|---|
| Arduino Uno R3 | Main controller; reads and processes sensor values |
| MQ-2 Gas Sensor | Detects smoke, LPG, CO and general air-pollution-related changes |
| KY-037 Sound Sensor | Detects sound intensity |
| ESP8266 | Provides Wi-Fi connectivity and cloud communication |
| I2C 16×2 LCD | Displays air-quality and noise readings |
| Breadboard & Jumper Wires | Prototyping and circuit connections |

The technical project document specifies MQ-2 and KY-037 for the implementation. The presentation lists MQ-135 and MQ07 as gas sensors, so the exact gas-sensor configuration should be kept consistent with the hardware/code in the repository.

## 🔌 Pin Connections

### MQ-2 Gas Sensor

| MQ-2 | Arduino Uno |
|---|---|
| VCC | 5V |
| GND | GND |
| AO | A0 |

### KY-037 Sound Sensor

| KY-037 | Arduino Uno |
|---|---|
| VCC | 5V |
| GND | GND |
| AO | A1 |

### I2C LCD

| LCD | Arduino Uno |
|---|---|
| SDA | A4 |
| SCL | A5 |
| VCC | 5V |
| GND | GND |

### ESP8266

> ⚠️ **ESP8266 operates at 3.3V. Use an appropriate 3.3V regulator/level-shifting arrangement.**

| ESP8266 | Arduino / Supply |
|---|---|
| VCC | 3.3V |
| GND | GND |
| CH_PD / EN | 3.3V |
| TX | Arduino RX through voltage divider |
| RX | Arduino TX |

The Arduino code uses `SoftwareSerial` with:
- **Arduino pin 10:** RX
- **Arduino pin 11:** TX

## 🔄 System Workflow

```text
        ┌─────────────────┐
        │  MQ-2 Gas Sensor│
        └────────┬────────┘
                 │
        ┌────────▼────────┐
        │ KY-037 Sensor   │
        └────────┬────────┘
                 │
                 ▼
        ┌─────────────────┐
        │   Arduino Uno    │
        │ Read & Process   │
        └───────┬─────┬────┘
                │     │
        ┌───────▼─┐   │
        │ I2C LCD │   │ Serial
        │ Display │   │
        └─────────┘   ▼
                 ┌────────────┐
                 │  ESP8266   │
                 │ Wi-Fi      │
                 └─────┬──────┘
                       │
                       ▼
                ┌─────────────┐
                │ ThingSpeak  │
                │ Cloud       │
                └──────┬──────┘
                       │
                       ▼
                ┌─────────────┐
                │ Graphs /    │
                │ Hotspot Map │
                └─────────────┘
```

## ⚙️ How It Works

1. The gas and sound sensors produce analog signals.
2. The Arduino Uno reads the sensor values through `A0` and `A1`.
3. The raw gas-sensor value is converted into an approximate air-quality index.
4. The sound-sensor value is converted into an approximate noise value.
5. The LCD displays the current readings.
6. The ESP8266 connects to Wi-Fi using AT commands.
7. The Arduino sends the processed values to the ESP8266.
8. The ESP8266 sends the values to ThingSpeak using an HTTP request.
9. Cloud data can be visualized over time and, with GPS/location data, used for hotspot mapping.

## 📈 Sensor Calibration & Accuracy

The sensor values in this project should be treated as **indicative measurements**, not professional environmental measurements.

### MQ-2

The MQ-2 produces an analog value affected by gas concentration and sensor heating. Exact PPM measurement requires calibration using the sensor's resistance curve (`Rs/R0`) and appropriate load-resistor values.

The current Arduino implementation uses a simple mapping:

```text
Raw MQ-2 value: 0–1023
Approximate AQ index: 0–500
```

For more accurate measurements, the sensor should be calibrated against known gas concentrations.

### KY-037

The KY-037 provides microphone/amplifier amplitude rather than a calibrated dB measurement.

The current implementation approximately maps:

```text
Raw sound value: 0–1023
Approximate noise range: 30–120 dB
```

For better results, readings should be calibrated against a reasonably accurate sound meter at a fixed distance.

> **Important:** These sensors are suitable for a student/hobby prototype and trend monitoring. Calibrated instruments should be used for legal, regulatory, or official measurements.

## ☁️ ThingSpeak Integration

The Arduino code is designed to upload two values to ThingSpeak:

- **Field 1:** Air-quality value
- **Field 2:** Noise value

The upload interval in the provided implementation is **20 seconds**.

Before uploading/running the code, configure:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
const char* THINGSPEAK_APIKEY = "YOUR_THINGSPEAK_WRITE_KEY";
```

**Do not commit real Wi-Fi passwords or API keys to GitHub.**

## 💻 Software Requirements

- Arduino IDE
- Arduino Uno R3
- ESP8266 with AT firmware
- Required Arduino libraries:
  - `Wire`
  - `LiquidCrystal_I2C`
  - `SoftwareSerial`
- A ThingSpeak channel for cloud data

## 🚀 Getting Started

### 1. Clone the repository

```bash
git clone <YOUR_REPOSITORY_URL>
cd <YOUR_REPOSITORY_FOLDER>
```

### 2. Open the Arduino project

Open the `.ino` file in Arduino IDE.

### 3. Install required libraries

Install:

- **LiquidCrystal_I2C**
- Libraries included with the Arduino IDE such as `Wire` and `SoftwareSerial`

### 4. Configure Wi-Fi and ThingSpeak

Replace the placeholder credentials in the Arduino code:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
const char* THINGSPEAK_APIKEY = "YOUR_THINGSPEAK_WRITE_KEY";
```

### 5. Select the board and port

In Arduino IDE:

```text
Tools → Board → Arduino Uno
Tools → Port → Select your Arduino COM port
```

### 6. Upload the code

Upload the sketch to the Arduino Uno and open the Serial Monitor at the configured baud rate to observe debugging information.

## 🖥️ Example LCD Output

```text
AQ:225
Noise:65dB
```

The values are examples and depend on sensor readings and calibration.

## 📊 Results

The project presentation reports that the prototype was able to detect ambient noise from approximately **50 dB** to above **90 dB**, with higher readings associated with traffic/honking conditions.

The documented system also describes continuous exposure above **85 dB for more than 5 minutes** as a trigger for GPS logging in the hotspot-mapping workflow.

Data transmitted through ESP8266 to ThingSpeak can be visualized as time-series graphs and, when GPS coordinates are incorporated, as points or heatmaps representing pollution/noise hotspots.

## ⚠️ Limitations

- MQ-series gas sensors require proper warm-up and calibration.
- The MQ-2-to-AQI mapping in the current code is a simplified linear approximation.
- The KY-037 is not a calibrated dB meter.
- The current prototype does not by itself provide legally certified environmental measurements.
- Accurate hotspot mapping requires reliable GPS/location data.
- ESP8266 communication depends on suitable 3.3V power and serial-level handling.
- Wi-Fi availability is required for cloud transmission.

## 🔮 Future Scope

Potential improvements include:

- 📱 A dedicated mobile application
- 🌡️ Temperature and humidity sensors
- 📍 Integrated GPS for automatic location tagging
- 🤖 Machine-learning-based pollution prediction
- ☁️ More advanced cloud analytics
- 🔋 Improved battery and power management
- 📊 More accurate sensor calibration
- 🗺️ Interactive real-time pollution/noise heatmaps

## 🗺️ Circuit Diagram

The project circuit was designed using **Cirkit Designer**.

[Open the Circuit Designer project](https://app.cirkitdesigner.com/project/63704ef4-e460-44e3-9cec-53fbdcca568c)

## 👥 Project Team

- Aman
- Shriank
- Jay
- Pritam

**Institution:** Lovely Professional University

## 📚 Project Documentation

The repository/project documentation is based on the project's technical documentation and presentation covering the system architecture, methodology, sensor calibration, ESP8266 communication, ThingSpeak integration, results, limitations, and future scope.

---

### ⭐ Project Goal

> Build an affordable, portable, and accessible system for real-time monitoring of air quality and environmental noise, with Wi-Fi-enabled data transmission and the potential to identify localized pollution hotspots.
