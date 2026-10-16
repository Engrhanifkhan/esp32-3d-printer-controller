# ESP32 3D Printer Control Unit

A complete IoT controller for 3D printer heated enclosures, beds, or drying chambers with:
- **3x DS18B20 Digital Temperature Sensors** (e.g. Chamber, Bed, Ambient/Electronics)
- **Solid State Relay (SSR)** control for safe AC or DC heating elements
- **Responsive Web Dashboard** (usable from phone, tablet, or PC browser without cloud dependency)
- **Adjustable Auto-off Timer** (quick buttons + custom countdown to automatically cut heater power)
- **ESP32-CAM Live Stream Window** (view real-time 3D prints side-by-side with temperatures)

---

## 1. Hardware Wiring Guide

### A. 3x DS18B20 Temperature Sensors (1-Wire Bus)
All 3 DS18B20 sensors connect in parallel to the exact same 3 wires:
| DS18B20 Wire Color / Pin | ESP32 Pin | Note |
|---|---|---|
| **VCC (Red)** | `3.3V` | Powers the temperature sensors |
| **GND (Black)** | `GND` | Ground reference |
| **DATA (Yellow / Blue)** | `GPIO 4` | Shared 1-Wire signal bus |

> ⚠️ **CRITICAL: 4.7kΩ Pull-Up Resistor**
> Place one **4.7kΩ resistor** between `DATA (GPIO 4)` and `3.3V`. Without this resistor, the sensors will read `-127°C` (Disconnected).

```
  ESP32 3.3V  -----------------+------------+------------+
                               |            |            |
                             [4.7kΩ]      [VCC]        [VCC]
                               |         Sensor 1     Sensor 2 ...
  ESP32 GPIO 4 ----------------+------------+------------+
                               |          [DATA]       [DATA]
  ESP32 GND   -----------------+------------+------------+
                                          [GND]        [GND]
```

---

### B. Solid State Relay (SSR)
| SSR Terminal | Connection |
|---|---|
| **Input (+)** / Terminal 3 | `GPIO 18` |
| **Input (-)** / Terminal 4 | `GND` |
| **Output / Load (Terminals 1 & 2)** | In series with your Heater power line (AC Live or DC +) |

*Recommended SSR models:*
- AC Heaters (Mains 110V/220V): **Fotek SSR-25DA**, **Crydom**, or similar DC-AC relay.
- DC Heaters (12V/24V): **SSR-25DD** (DC-DC) or high-current MOSFET module.

---

### C. Future ESP32-CAM Module
To add live video streaming:
1. Flash your ESP32-CAM with the standard Arduino **CameraWebServer** example (`File > Examples > ESP32 > Camera > CameraWebServer`).
2. Connect the ESP32-CAM to the same local WiFi network.
3. Check the ESP32-CAM Serial Monitor for its IP address (e.g., `http://192.168.1.150`).
4. In the main control dashboard, paste the stream URL: `http://<ESP32_CAM_IP>:81/stream` and click **Connect**.
5. The live video will immediately stream inside the dashboard.

---

## 2. Software & Libraries (Arduino IDE)

1. Open **Arduino IDE**.
2. Go to **Tools > Manage Libraries...** (or `Ctrl + Shift + I`).
3. Search and install:
   - **OneWire** (by *Paul Stoffregen*)
   - **DallasTemperature** (by *Miles Burton*)
4. Under **Tools > Board**, select **ESP32 Dev Module** (or your specific ESP32 variant).
5. Open `esp32_heater_controller.ino`.
6. Update your WiFi credentials:
   ```cpp
   const char* ssid     = "YOUR_WIFI_NAME";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
7. Click **Upload**.

---

## 3. How to Use the Dashboard

1. Open the Arduino Serial Monitor at `115200 baud` after uploading.
2. Note the IP address displayed:
   ```text
   WiFi Connected!
   Web Dashboard IP Address: http://192.168.1.120
   ```
3. Open any web browser on your phone or PC (connected to the same WiFi) and navigate to `http://192.168.1.120`.
4. If your home WiFi is not reachable, the ESP32 will automatically create its own WiFi network:
   - **SSID:** `ESP32-3DPrinter-Control`
   - **Password:** `12345678`
   - **IP:** `http://192.168.4.1`

### Dashboard Capabilities:
- **Temperature Display:** Shows 3 temperatures (Chamber, Bed, Ambient).
- **Thermostat:** Choose which sensor regulates the heater, set target temperature & hysteresis band.
- **Timer:** Select +15m, +30m, +1hr, +2hr or enter custom minutes. When the timer elapses, the heater is automatically switched off.
- **Emergency Stop:** Instantly cuts SSR power.
- **Camera Feed:** Integrates live camera stream directly.
