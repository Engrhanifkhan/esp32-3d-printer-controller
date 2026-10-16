/*************************************************************
  ESP32 3D Printer Controller - Restored Standard Blynk WiFi
  -------------------------------------------------------------
  Features:
   - Uses the proven official Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass)
   - Configured for: "CISNR" (Password: 2026@cisnr)
     (Also includes "NCAI" / "2026@ncai" ready to uncomment)
   - SENSOR 3: Dedicated Live Heater Temperature (V2) - Monitor Only
   - SENSORS 1 & 2: Dual Zone Control (MIN 45 °C - MAX 60 °C)
   - Timer in Hours & Minutes with automatic shutoff at 00:00:00
   - Solid State Relay (SSR) on GPIO 18 (Strictly OFF on boot)
   - Serial commands: 1/ON, 0/OFF, START <h> <m>, STOP, STATUS
 *************************************************************/

// Blynk template credentials
#define BLYNK_TEMPLATE_ID   "TMPL6dWwINGDY"
#define BLYNK_TEMPLATE_NAME "3D Printer Controller"
#define BLYNK_AUTH_TOKEN    "gaE53Ka7uNDb9wVZNMUXY-3gu-LUDwD-"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ==================== PINS ====================
#define ONE_WIRE_BUS_PIN     4    // GPIO 4 for all 3 DS18B20 sensors (requires 4.7k pull-up to 3.3V)
#define SSR_HEATER_PIN      18    // GPIO 18 connected to SSR Input (+)
#define STATUS_LED_PIN       2    // Built-in ESP32 LED

// ==================== HARD SAFETY LIMIT ====================
#define MAX_SAFE_TEMP       110.0 // Hard emergency shutoff in Celsius

// ==================== WIFI CREDENTIALS (NCAI) ====================
char ssid[] = "CISNR";
char pass[] = "2026@cisnr";

// ==================== OBJECTS & STATE ====================
OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature sensors(&oneWire);
BlynkTimer timer;

// Temperatures
float temp1 = 0.0; // Sensor 1 (Control Zone 1 / Chamber)
float temp2 = 0.0; // Sensor 2 (Control Zone 2 / Bed)
float temp3 = 0.0; // Sensor 3 (HEATER TEMPERATURE - Live Monitor Only)
int sensorCount = 0;

// MIN & MAX Limits (Default: 40 C to 45 C)
float minTemp = 40.0;
float maxTemp = 45.0;

// Relay & Heater State
bool ssrState = false;
bool heaterEnabled = false;

// Timer Settings
bool timerActive = false;
int timerHours = 0;
int timerMinutes = 2;
unsigned long timerTotalSeconds = 120;
unsigned long timerRemainingSeconds = 120;

// Helper: Format seconds to HH:MM:SS
String formatTime(unsigned long totalSec) {
  unsigned int h = totalSec / 3600;
  unsigned int m = (totalSec % 3600) / 60;
  unsigned int s = totalSec % 60;
  char buf[12];
  snprintf(buf, sizeof(buf), "%02u:%02u:%02u", h, m, s);
  return String(buf);
}

// Low-level hardware SSR control
void setSSR(bool turnOn) {
  ssrState = turnOn;
  digitalWrite(SSR_HEATER_PIN, turnOn ? HIGH : LOW);
  digitalWrite(STATUS_LED_PIN, turnOn ? HIGH : LOW);

  Serial.printf("[SSR] Pin 18 set to %s (%s)\n", 
                turnOn ? "ON" : "OFF", 
                turnOn ? "3.3V HIGH" : "0V LOW");
}

// ==================== TIMER CONTROLLER ====================

void startHeatingTimer(int hours, int minutes) {
  unsigned long totalSec = ((unsigned long)hours * 3600) + ((unsigned long)minutes * 60);
  
  if (totalSec == 0) {
    totalSec = 120;
    hours = 0;
    minutes = 2;
  }

  timerHours = hours;
  timerMinutes = minutes;
  timerTotalSeconds = totalSec;
  timerRemainingSeconds = totalSec;
  timerActive = true;
  heaterEnabled = true;

  if (temp1 < maxTemp && temp2 < maxTemp) {
    setSSR(true);
  }

  if (Blynk.connected()) {
    Blynk.virtualWrite(V6, timerHours);
    Blynk.virtualWrite(V7, timerMinutes);
    Blynk.virtualWrite(V8, 1);
    Blynk.virtualWrite(V3, 1);
    Blynk.virtualWrite(V9, formatTime(timerRemainingSeconds));
  }

  Serial.printf("\n>>> [TIMER STARTED] %d hr %d min | Limits: MIN=%.1f C / MAX=%.1f C <<<\n", hours, minutes, minTemp, maxTemp);
}

void stopHeatingTimer() {
  timerActive = false;
  timerRemainingSeconds = 0;
  heaterEnabled = false;
  setSSR(false);

  if (Blynk.connected()) {
    Blynk.virtualWrite(V8, 0);
    Blynk.virtualWrite(V3, 0);
    Blynk.virtualWrite(V9, "00:00:00");
  }

  Serial.println("\n>>> [HEATER STOPPED] SSR turned OFF (Pin 18 = 0V). <<<");
}

// ==================== BLYNK APP & WEB HANDLERS ====================

BLYNK_WRITE(V3) {
  int val = param.asInt();
  heaterEnabled = (val == 1);
  if (heaterEnabled) {
    if (temp1 < maxTemp && temp2 < maxTemp) setSSR(true);
    Serial.println("[APP] Heater Switch -> ON!");
  } else {
    stopHeatingTimer();
    setSSR(false);
    Serial.println("[APP] Heater Switch -> OFF!");
  }
}

BLYNK_WRITE(V4) {
  minTemp = param.asFloat();
  Serial.printf("[APP] MIN Temp Limit Set to: %.1f C\n", minTemp);
}

BLYNK_WRITE(V10) {
  maxTemp = param.asFloat();
  Serial.printf("[APP] MAX Temp Limit Set to: %.1f C\n", maxTemp);
}

BLYNK_WRITE(V6) {
  timerHours = param.asInt();
}

BLYNK_WRITE(V7) {
  timerMinutes = param.asInt();
}

BLYNK_WRITE(V8) {
  int startBtn = param.asInt();
  if (startBtn == 1) {
    if (timerHours == 0 && timerMinutes == 0) timerMinutes = 2;
    startHeatingTimer(timerHours, timerMinutes);
  } else {
    stopHeatingTimer();
  }
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V3, V4, V6, V7, V8, V10);
  Serial.println("[BLYNK] Connected to cloud and synchronized!");
}

// ==================== SERIAL COMMAND HANDLER ====================

void processSerialCommand() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() == 0) return;

    cmd.toUpperCase();

    // 1 or ON -> Force SSR ON
    if (cmd.equals("1") || cmd.equals("ON")) {
      heaterEnabled = true;
      setSSR(true);
      Serial.println("\n>>> [SERIAL] SSR FORCED ON (Pin 18 = 3.3V) <<<");
    }
    // 0 or OFF -> Force SSR OFF
    else if (cmd.equals("0") || cmd.equals("OFF")) {
      stopHeatingTimer();
      setSSR(false);
      Serial.println("\n>>> [SERIAL] SSR FORCED OFF (Pin 18 = 0V) <<<");
    }
    // START <hours> <minutes>
    else if (cmd.startsWith("START")) {
      int firstSpace = cmd.indexOf(' ');
      int h = 0, m = 2;
      if (firstSpace > 0) {
        String rest = cmd.substring(firstSpace + 1);
        rest.trim();
        int secondSpace = rest.indexOf(' ');
        if (secondSpace > 0) {
          h = rest.substring(0, secondSpace).toInt();
          m = rest.substring(secondSpace + 1).toInt();
        } else {
          int totalM = rest.toInt();
          h = totalM / 60;
          m = totalM % 60;
        }
      }
      startHeatingTimer(h, m);
    }
    // STOP
    else if (cmd.equals("STOP")) {
      stopHeatingTimer();
    }
    // STATUS
    else if (cmd.equals("STATUS")) {
      Serial.println("\n--- STATUS ---");
      Serial.printf("S1: %.1f C | S2: %.1f C | S3(Heater): %.1f C\n", temp1, temp2, temp3);
      Serial.printf("Limits: [%.1f - %.1f C] | SSR: %s\n", minTemp, maxTemp, ssrState ? "ON" : "OFF");
      Serial.printf("WiFi: %s | Blynk: %s\n", (WiFi.status() == WL_CONNECTED) ? WiFi.SSID().c_str() : "OFFLINE", Blynk.connected() ? "ONLINE" : "OFFLINE");
      Serial.println("--------------\n");
    }
  }
}

// ==================== DUAL SENSOR CONTROL & LIVE HEATER READ ====================

void readSensorsAndThermostat() {
  sensors.requestTemperatures();

  float t1 = sensors.getTempCByIndex(0);
  float t2 = sensors.getTempCByIndex(1);
  float t3 = sensors.getTempCByIndex(2);

  temp1 = (t1 == DEVICE_DISCONNECTED_C) ? -127.0 : t1; // Sensor 1
  temp2 = (t2 == DEVICE_DISCONNECTED_C) ? -127.0 : t2; // Sensor 2
  temp3 = (t3 == DEVICE_DISCONNECTED_C) ? -127.0 : t3; // Sensor 3 (Heater Monitor Only)

  // Stream live temperatures to Blynk
  if (Blynk.connected()) {
    if (temp1 > -100.0) Blynk.virtualWrite(V0, temp1);
    if (temp2 > -100.0) Blynk.virtualWrite(V1, temp2);
    if (temp3 > -100.0) Blynk.virtualWrite(V2, temp3);
  }

  // Emergency safety cutoff (> 110 C)
  if (temp1 > MAX_SAFE_TEMP || temp2 > MAX_SAFE_TEMP || temp3 > MAX_SAFE_TEMP) {
    setSSR(false);
    timerActive = false;
    heaterEnabled = false;
    if (Blynk.connected()) {
      Blynk.virtualWrite(V3, 0);
      Blynk.virtualWrite(V8, 0);
    }
    Serial.println("\n[EMERGENCY] Safe limit (110 C) exceeded! SSR shut down.");
    return;
  }

  // Dual sensor control (Controlled by S1 and S2, S3 is live monitor only)
  if (heaterEnabled || timerActive) {
    if (temp1 >= maxTemp || temp2 >= maxTemp) {
      if (ssrState) {
        setSSR(false);
        Serial.printf("[THERMOSTAT] Reached MAX limit (%.1f C) -> SSR turned OFF!\n", maxTemp);
      }
    } else if (temp1 > -100.0 && temp2 > -100.0 && temp1 < minTemp && temp2 < minTemp) {
      if (!ssrState) {
        setSSR(true);
        Serial.printf("[THERMOSTAT] Both sensors < MIN limit (%.1f C) -> SSR turned ON!\n", minTemp);
      }
    }
  } else {
    if (ssrState) setSSR(false);
  }
}

// 1-Second Timer Tick
void timerCountdownTick() {
  if (timerActive && timerRemainingSeconds > 0) {
    timerRemainingSeconds--;

    if (Blynk.connected()) {
      Blynk.virtualWrite(V9, formatTime(timerRemainingSeconds));
    }

    if (timerRemainingSeconds == 0) {
      timerActive = false;
      heaterEnabled = false;
      setSSR(false); // AUTO-SHUTOFF!

      if (Blynk.connected()) {
        Blynk.virtualWrite(V8, 0);
        Blynk.virtualWrite(V3, 0);
        Blynk.virtualWrite(V9, "FINISHED");
      }

      Serial.println("\n=======================================================");
      Serial.println(">>> [TIMER FINISHED] Duration elapsed!                 <<<");
      Serial.println(">>> [AUTO-OFF] SSR turned OFF automatically (0V LOW)!  <<<");
      Serial.println("=======================================================\n");
    }
  }
}

// Print status to Serial Monitor every 2 seconds
void printRealtimeData() {
  Serial.print("[DATA] ");
  
  Serial.print("S1: ");
  if (temp1 <= -100.0) Serial.print("ERR");
  else Serial.printf("%.1f C", temp1);

  Serial.print(" | S2: ");
  if (temp2 <= -100.0) Serial.print("ERR");
  else Serial.printf("%.1f C", temp2);

  Serial.print(" | S3(Heater): ");
  if (temp3 <= -100.0) Serial.print("ERR");
  else Serial.printf("%.1f C", temp3);

  Serial.printf(" | Limits: [%.1f - %.1f C]", minTemp, maxTemp);
  Serial.printf(" | SSR: %s", ssrState ? "ON" : "OFF");
  Serial.printf(" | Pin 18: %s", (digitalRead(SSR_HEATER_PIN) == HIGH) ? "HIGH(3.3V)" : "LOW(0V)");

  if (timerActive) {
    Serial.printf(" | Timer: %s", formatTime(timerRemainingSeconds).c_str());
  } else {
    Serial.print(" | Timer: IDLE");
  }

  Serial.printf(" | WiFi: %s", (WiFi.status() == WL_CONNECTED) ? WiFi.SSID().c_str() : "OFFLINE");
  Serial.printf(" | Blynk: %s\n", Blynk.connected() ? "ONLINE" : "OFFLINE");
}

// ==================== SETUP & LOOP ====================

void setup() {
  Serial.begin(115200);
  delay(100); // Fast startup
  Serial.println("\n=======================================================");
  Serial.println("  ESP32 3D Printer Controller Starting (Fast Boot)...");
  Serial.println("=======================================================");

  // STEP 1: FORCE PIN 18 TO LOW (0V) IMMEDIATELY! (Stays OFF on boot)
  pinMode(SSR_HEATER_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(SSR_HEATER_PIN, LOW);
  digitalWrite(STATUS_LED_PIN, LOW);
  ssrState = false;
  heaterEnabled = false;
  Serial.println("[SSR] Pin 18 set strictly to LOW (0V). SSR is OFF.");

  // Step 2: Start 1-Wire sensors (All 3 on Pin 4)
  sensors.begin();
  sensorCount = sensors.getDeviceCount();
  Serial.printf("[HARDWARE] Found %d DS18B20 sensors on Pin 4.\n", sensorCount);

  // Step 3: Fast WiFi connection configuration
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);        // Disable radio sleep: instant fast handshake
  WiFi.setAutoReconnect(true); // Auto-reconnect in background if disconnected
  WiFi.persistent(true);       // Save fast association parameters in flash

  Serial.printf("[WIFI] Fast-connecting to '%s' ...\n", ssid);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(">>> [WIFI] CONNECTED SUCCESSFULLY! <<<");
    Serial.printf("[WIFI] Connected to: %s\n", WiFi.SSID().c_str());
    Serial.print("[WIFI] IP Address: ");
    Serial.println(WiFi.localIP());
  }

  // Step 4: Setup periodic timers
  timer.setInterval(1000L, readSensorsAndThermostat); // Sensors & SSR check every 1s
  timer.setInterval(1000L, timerCountdownTick);       // Timer countdown every 1s
  timer.setInterval(2000L, printRealtimeData);         // Real-time data every 2s

  Serial.println("\n--- System Ready! ---");
}

void loop() {
  Blynk.run();
  timer.run();
  processSerialCommand();
}
