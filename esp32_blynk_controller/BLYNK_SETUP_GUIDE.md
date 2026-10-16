# Blynk IoT Setup Guide - Control & Monitor from Anywhere

This guide walks you through setting up the free **Blynk IoT** app on your phone (iOS / Android) and connecting your ESP32 so you can monitor all 3 DS18B20 temperature sensors, control the Solid State Relay (SSR), adjust the timer, and watch your 3D printer over 4G/5G or WiFi from anywhere in the world.

---

## Step 1: Create a Free Blynk Account
1. Go to [https://blynk.cloud/](https://blynk.cloud/) and create a free account.
2. Install the **Blynk IoT** app on your phone from Google Play Store or Apple App Store.

---

## Step 2: Create a Device Template on Blynk Web Console
1. In the left menu of [blynk.cloud](https://blynk.cloud/), click **Templates** > **+ New Template**.
2. Set:
   - **Name:** `3D Printer Controller`
   - **Hardware:** `ESP32`
   - **Connection Type:** `WiFi`
3. Click **Done**.
4. You will see a black box at the top right showing:
   ```cpp
   #define BLYNK_TEMPLATE_ID   "TMPLxxxxxx"
   #define BLYNK_TEMPLATE_NAME "3D Printer Controller"
   #define BLYNK_AUTH_TOKEN    "Your_Auth_Token"
   ```
   👉 Copy these 3 lines and paste them at the top of your [`esp32_blynk_controller.ino`](file:///d:/Esp%2032%20control%20unit%203d%20printer/esp32_blynk_controller/esp32_blynk_controller.ino).

---

## Step 3: Configure Datastreams (Virtual Pins)
In your template, click the **Datastreams** tab > **+ New Datastream** > **Virtual Pin**:

| Pin | Name | Data Type | Min | Max | Default |
|---|---|---|---|---|---|
| **V0** | Chamber Temp | Double | -50 | 125 | 0 |
| **V1** | Bed Temp | Double | -50 | 125 | 0 |
| **V2** | Ambient Temp | Double | -50 | 125 | 0 |
| **V3** | Heater Switch | Integer | 0 | 1 | 0 |
| **V4** | Target Temp | Double | 20 | 100 | 60 |
| **V5** | SSR State LED | Integer | 0 | 255 | 0 |
| **V6** | Timer Hours | Integer | 0 | 24 | 1 |
| **V7** | Timer Minutes | Integer | 0 | 59 | 0 |
| **V8** | Timer Start/Stop | Integer | 0 | 1 | 0 |
| **V9** | Timer Remaining | String | - | - | "00:00:00" |
| **V10** | Camera Stream | String | - | - | - |

Click **Save** in the top right.

---

## Step 4: Build Your Mobile App Dashboard
Open the **Blynk IoT** app on your phone, open your device, and add these widgets to your dashboard:

1. **3x Value Displays or Gauges:**
   - Link to `V0` (Chamber Temp °C)
   - Link to `V1` (Bed Temp °C)
   - Link to `V2` (Ambient Temp °C)
2. **Button / Switch Widget:**
   - Link to `V3` (Heater Master Switch / Thermostat Enable)
3. **Slider or Numeric Input Widget:**
   - Link to `V4` (Target Temp Setpoint)
4. **LED Widget:**
   - Link to `V5` (Lights up red when the SSR is physically ON)
5. **Slider / Numeric Input Widgets:**
   - Link to `V6` (Timer Hours: 0 to 24)
   - Link to `V7` (Timer Minutes: 0 to 59)
6. **Button Switch Widget:**
   - Link to `V8` (Timer Start / Stop: 1 starts timer and turns heater ON, 0 turns OFF)
7. **Value Display Widget:**
   - Link to `V9` (Countdown display: e.g. `01:00:00` -> `00:59:59` -> `FINISHED`)
8. **Video Stream Widget:**
   - Link to `V10` (Displays live camera feed from your ESP32-CAM)

---

## Step 5: How to View the ESP32-CAM Stream from Anywhere

When you are outside your home on mobile data (4G/5G), your phone cannot directly access internal local IP addresses like `192.168.1.xxx`. 

Here are the 2 simplest ways to view your ESP32-CAM live from anywhere:

### Method A: Free Tunneling via Pinggy / Ngrok (Easiest)
1. Run a lightweight tunnel on any computer / Raspberry Pi in your home network (or directly on a local server):
   ```bash
   ssh -p 443 -R0:192.168.1.CAM_IP:81 a.pinggy.io
   ```
2. Pinggy gives you a public HTTPS URL (e.g., `https://random-xyz.a.pinggy.link/stream`).
3. Paste that public URL into the Blynk **Video Stream Widget** (`V9`).
4. You can now watch the live video stream from anywhere in the world!

### Method B: Telegram Bot Snapshot on Demand (Alternative Zero-Config)
- If you prefer zero port-forwarding and zero router setup, your ESP32-CAM can be flashed with a **Telegram Bot** sketch.
- You can simply open your Telegram app from anywhere and type `/photo` or `/video` to instantly get high-resolution pictures of your 3D print sent to your phone!

---

## Step 6: Arduino IDE Libraries to Install
In Arduino IDE (**Sketch > Include Library > Manage Libraries**), install:
- **Blynk** (by Volodymyr Shymanskyy)
- **OneWire** (by Paul Stoffregen)
- **DallasTemperature** (by Miles Burton)
