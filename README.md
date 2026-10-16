# ESP32 3D Printer Enclosure & Chamber Temperature Controller

A complete IoT and firmware solution to monitor and regulate temperatures inside a 3D printer heated chamber/enclosure using an **ESP32**, **Solid State Relay (SSR)**, and **3x DS18B20 digital temperature sensors**.

---

## 🚀 Features

- **3x DS18B20 Temperature Sensors on 1-Wire (GPIO 4):**
  - **Sensor 1:** Control Zone 1 (Chamber temperature).
  - **Sensor 2:** Control Zone 2 (Bed / Enclosure temperature).
  - **Sensor 3:** Dedicated live heater monitor (displays real-time heater temperature).
- **Dual-Zone Thermostat Logic:**
  - Automatic heating regulation between adjustable limits (**MIN 45.0 °C** and **MAX 60.0 °C**).
  - Turns SSR ON when both sensors are below MIN.
  - Turns SSR OFF if either sensor reaches MAX.
- **Safety First:**
  - SSR (GPIO 18) strictly forced **LOW (0V / OFF)** on boot.
  - Emergency shutoff threshold (**110.0 °C**) to prevent thermal runaway.
- **Countdown Timer with Auto-Shutoff:**
  - Configurable hours & minutes with automatic shutoff at `00:00:00`.
- **IoT Cloud & Mobile Dashboard:**
  - Full remote control via **Blynk IoT** (mobile app & web console).
- **Interactive Serial Monitor Interface (115200 baud):**
  - Live data output every 2 seconds.
  - Manual control commands (`1`/`ON`, `0`/`OFF`, `START <h> <m>`, `STOP`, `STATUS`).

---

## 📁 Repository Structure

```
.
├── esp32_blynk_controller/            # Main production firmware with Blynk IoT
│   ├── esp32_blynk_controller.ino
│   └── BLYNK_SETUP_GUIDE.md
├── esp32_heater_controller/           # Standalone local web server dashboard option
│   ├── esp32_heater_controller.ino
│   └── README.md
├── esp32_blynk_telegram_controller/   # Optional Blynk + Telegram Bot integration
│   └── esp32_blynk_telegram_controller.ino
├── hardware_diagnostic/               # Hardware diagnostic tool for sensors and pins
│   └── hardware_diagnostic.ino
├── find_sensor_addresses/             # Utility to scan 64-bit ROM addresses of DS18B20s
│   └── find_sensor_addresses.ino
├── relay_test/                        # ESP32 SSR / relay hardware test sketch
│   └── relay_test.ino
└── arduino_relay_test/                # Arduino Uno relay verification sketch
    └── arduino_relay_test.ino
```

---

## 🔌 Hardware Pinout

| Component | ESP32 Pin | Details |
| :--- | :--- | :--- |
| **Solid State Relay (SSR)** | **GPIO 18** | Active HIGH (3.3V = ON, 0V = OFF). Heater control. |
| **DS18B20 Sensors (x3)** | **GPIO 4** | 1-Wire bus with 4.7kΩ pull-up resistor to 3.3V. |
| **Status LED** | **GPIO 2** | Built-in LED mirrors SSR state. |

---

## ⚙️ Blynk Virtual Pins

| Virtual Pin | Type | Function |
| :--- | :--- | :--- |
| **V0** | Double | Sensor 1 Temp (°C) |
| **V1** | Double | Sensor 2 Temp (°C) |
| **V2** | Double | Sensor 3 Heater Live Temp (°C) |
| **V3** | Integer | Heater Switch (ON / OFF) |
| **V4** | Double | MIN Temperature Limit |
| **V6** | Integer | Timer Hours |
| **V7** | Integer | Timer Minutes |
| **V8** | Integer | Timer Start / Stop |
| **V9** | String | Remaining Time Countdown (`HH:MM:SS`) |
| **V10** | Double | MAX Temperature Limit |

---

## 🛠️ Getting Started

1. Open **`esp32_blynk_controller/esp32_blynk_controller.ino`** in Arduino IDE.
2. Ensure you have installed the required libraries:
   - `Blynk`
   - `OneWire`
   - `DallasTemperature`
3. Set your board to **ESP32 Dev Module**.
4. Configure your WiFi credentials if needed in lines 37–38.
5. Click **Upload** and open Serial Monitor at **115200 baud**.
