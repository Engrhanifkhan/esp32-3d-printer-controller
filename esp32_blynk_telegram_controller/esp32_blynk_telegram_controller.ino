/*************************************************************
  ESP32 3D Printer Controller - DUAL BLYNK IOT + TELEGRAM BOT
  -------------------------------------------------------------
  Features:
   - Full Remote Access from ANYWHERE in the world!
   - DUAL INTERFACE:
       1) Blynk IoT App (Sliders, Buttons, Live Gauges)
       2) Telegram Bot (Chat from anywhere: /status, /start 1h, /stop)
   - Push notifications to your Telegram when heating finishes!
   - 3x DS18B20 Digital Sensors on 1 Pin (GPIO 4)
   - Solid State Relay (SSR) on GPIO 18 (Strictly OFF on boot)
   - Custom Timer: 1 hour or ANY random minutes/hours
   - Direct Fast Connection to WiFi "NCAI" (Password: 2026@ncai)
   - NO CAMERA (Completely removed)
 *************************************************************/

// ==================== BLYNK CREDENTIALS ====================
#define BLYNK_TEMPLATE_ID   "TMPL6dWwINGDY"
#define BLYNK_TEMPLATE_NAME "3D Printer Controller"
#define BLYNK_AUTH_TOKEN    "gaE53Ka7uNDb9wVZNMUXY-3gu-LUDwD-"

// ==================== TELEGRAM BOT CREDENTIALS ====================
// Replace with your Telegram Bot Token & Chat ID from @BotFather
#define BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define CHAT_ID   "YOUR_TELEGRAM_CHAT_ID"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp32.h>
#include <UniversalTelegramBot.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>

// ==================== PINS ====================
#define ONE_WIRE_BUS_PIN     4   // All 3 DS18B20 sensors on GPIO 4
#define SSR_HEATER_PIN      18   // SSR on GPIO 18
#define STATUS_LED_PIN       2   // Built-in ESP32 LED

// ==================== SAFETY LIMITS ====================
#define MAX_SAFE_TEMP       110.0 // Hard emergency shutoff in Celsius
#define SENSOR_DISCONNECT_ERR -50.0

// ==================== WIFI ====================
char ssid[] = "SWEET HOME";
char pass[] = "03062026";

// ==================== OBJECTS & STATE ====================
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature sensors(&oneWire);
BlynkTimer timer;

// Temperatures
float tempChamber = 0.0;
float tempBed     = 0.0;
float tempAmbient = 0.0;

// Thermostat & Relay Settings
bool heaterEnabled = false;
bool ssrState = false;
float targetTemp = 60.0;
float hysteresis = 1.0;
int controlSensor = 1; // 1 = Chamber, 2 = Bed

// Timer Settings
bool timerActive = false;
int timerHours = 1;
int timerMinutes = 0;
unsigned long timerTotalSeconds = 0;
unsigned long timerRemainingSeconds = 0;

// Telegram polling interval
unsigned long lastTelegramCheck = 0;
const unsigned long TELEGRAM_INTERVAL = 1000; // Check Telegram every 1 second

// Helper: Format seconds to HH:MM:SS
String formatTime(unsigned long totalSec) {
  unsigned int h = totalSec / 3600;
  unsigned int m = (totalSec % 3600) / 60;
  unsigned int s = totalSec % 60;
  char buf[12];
  snprintf(buf, sizeof(buf), "%02u:%02u:%02u", h, m, s);
  return String(buf);
}

void setSSR(bool turnOn) {
  ssrState = turnOn;
  digitalWrite(SSR_HEATER_PIN, turnOn ? HIGH : LOW);
  digitalWrite(STATUS_LED_PIN, turnOn ? HIGH : LOW);

  if (Blynk.connected()) {
    Blynk.virtualWrite(V5, turnOn ? 255 : 0);
  }
}

// ==================== TIMER CONTROLLER ====================

void startHeatingTimer(int hours, int minutes) {
  unsigned long totalSec = ((unsigned long)hours * 3600) + ((unsigned long)minutes * 60);
  if (totalSec == 0) return;

  timerHours = hours;
  timerMinutes = minutes;
  timerTotalSeconds = totalSec;
  timerRemainingSeconds = totalSec;
  timerActive = true;
  heaterEnabled = true;

  // Sync to Blynk
  if (Blynk.connected()) {
    Blynk.virtualWrite(V6, timerHours);
    Blynk.virtualWrite(V7, timerMinutes);
    Blynk.virtualWrite(V8, 1);
    Blynk.virtualWrite(V3, 1);
    Blynk.virtualWrite(V9, formatTime(timerRemainingSeconds));
  }

  Serial.printf("\n[TIMER] Started for %d hr %d min | Target: %.1f C\n", hours, minutes, targetTemp);
}

void stopHeatingTimer() {
  timerActive = false;
  timerRemainingSeconds = 0;
  heaterEnabled = false;
  setSSR(false);

  // Sync to Blynk
  if (Blynk.connected()) {
    Blynk.virtualWrite(V8, 0);
    Blynk.virtualWrite(V3, 0);
    Blynk.virtualWrite(V9, "00:00:00");
  }

  Serial.println("\n[TIMER] Stopped. SSR turned OFF.");
}

// ==================== BLYNK APP HANDLERS ====================

BLYNK_WRITE(V3) {
  heaterEnabled = (param.asInt() == 1);
  if (!heaterEnabled) setSSR(false);
  Serial.printf("[BLYNK] Heater Master: %s\n", heaterEnabled ? "ON" : "OFF");
}

BLYNK_WRITE(V4) {
  targetTemp = param.asFloat();
  Serial.printf("[BLYNK] Target Temp: %.1f C\n", targetTemp);
}

BLYNK_WRITE(V6) {
  timerHours = param.asInt();
}

BLYNK_WRITE(V7) {
  timerMinutes = param.asInt();
}

BLYNK_WRITE(V8) {
  if (param.asInt() == 1) {
    startHeatingTimer(timerHours, timerMinutes);
  } else {
    stopHeatingTimer();
  }
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V3, V4, V6, V7, V8);
  Serial.println("[BLYNK] Synced settings from cloud!");
}

// ==================== TELEGRAM BOT COMMAND HANDLER ====================

void handleTelegramMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    text.trim();

    Serial.printf("[TELEGRAM] Message from %s: '%s'\n", chat_id.c_str(), text.c_str());

    if (text == "/start" || text == "/help") {
      String welcome = "🤖 *3D Printer Controller Bot*\n\n";
      welcome += "Available Commands:\n";
      welcome += "📊 /status - View live temperatures & relay state\n";
      welcome += "⏱️ /timer 1h - Start heating for 1 hour\n";
      welcome += "⏱️ /timer 45m - Start heating for 45 minutes\n";
      welcome += "🌡️ /set 60 - Set target temperature to 60°C\n";
      welcome += "🔴 /stop - Turn off heater immediately\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
    else if (text == "/status") {
      String stat = "📊 *Current Printer Status*\n\n";
      stat += "🌡️ *Chamber:* " + (tempChamber <= SENSOR_DISCONNECT_ERR ? "ERR" : String(tempChamber, 1) + " °C") + "\n";
      stat += "🌡️ *Bed:* "     + (tempBed <= SENSOR_DISCONNECT_ERR ? "ERR" : String(tempBed, 1) + " °C") + "\n";
      stat += "🌡️ *Ambient:* " + (tempAmbient <= SENSOR_DISCONNECT_ERR ? "ERR" : String(tempAmbient, 1) + " °C") + "\n";
      stat += "🎯 *Target:* "  + String(targetTemp, 1) + " °C\n";
      stat += "⚡ *Heater SSR:* " + String(ssrState ? "🔥 ON (HEATING)" : "❄️ OFF") + "\n";
      stat += "⏱️ *Timer:* " + (timerActive ? formatTime(timerRemainingSeconds) : "IDLE") + "\n";
      bot.sendMessage(chat_id, stat, "Markdown");
    }
    else if (text.startsWith("/timer ")) {
      String arg = text.substring(7);
      arg.trim();
      arg.toLowerCase();

      int h = 0, m = 0;
      if (arg.endsWith("h")) {
        h = arg.substring(0, arg.length() - 1).toInt();
      } else if (arg.endsWith("m")) {
        m = arg.substring(0, arg.length() - 1).toInt();
      } else {
        m = arg.toInt(); // Default to minutes
      }

      if (h > 0 || m > 0) {
        startHeatingTimer(h, m);
        String reply = "✅ *Timer Started!*\n";
        reply += "Duration: " + String(h) + " hr " + String(m) + " min\n";
        reply += "Target Temp: " + String(targetTemp, 1) + " °C\n";
        reply += "Heater SSR is now ACTIVE.";
        bot.sendMessage(chat_id, reply, "Markdown");
      } else {
        bot.sendMessage(chat_id, "⚠️ Invalid format. Use: `/timer 1h` or `/timer 45m`", "Markdown");
      }
    }
    else if (text.startsWith("/set ")) {
      float newTgt = text.substring(5).toFloat();
      if (newTgt > 0 && newTgt <= MAX_SAFE_TEMP) {
        targetTemp = newTgt;
        if (Blynk.connected()) Blynk.virtualWrite(V4, targetTemp);
        bot.sendMessage(chat_id, "🎯 Target temperature updated to: *" + String(targetTemp, 1) + " °C*", "Markdown");
      } else {
        bot.sendMessage(chat_id, "⚠️ Temperature must be between 20°C and 110°C.", "Markdown");
      }
    }
    else if (text == "/stop" || text == "/off") {
      stopHeatingTimer();
      bot.sendMessage(chat_id, "🛑 *Heater turned OFF immediately.*", "Markdown");
    }
    else {
      bot.sendMessage(chat_id, "❓ Unknown command. Type /help to see commands.", "Markdown");
    }
  }
}

// ==================== SENSORS & THERMOSTAT ====================

void readSensorsAndThermostat() {
  sensors.requestTemperatures();

  float t1 = sensors.getTempCByIndex(0);
  float t2 = sensors.getTempCByIndex(1);
  float t3 = sensors.getTempCByIndex(2);

  tempChamber = (t1 == DEVICE_DISCONNECTED_C) ? SENSOR_DISCONNECT_ERR : t1;
  tempBed     = (t2 == DEVICE_DISCONNECTED_C) ? SENSOR_DISCONNECT_ERR : t2;
  tempAmbient = (t3 == DEVICE_DISCONNECTED_C) ? SENSOR_DISCONNECT_ERR : t3;

  // Send to Blynk
  if (Blynk.connected()) {
    if (tempChamber > SENSOR_DISCONNECT_ERR) Blynk.virtualWrite(V0, tempChamber);
    if (tempBed > SENSOR_DISCONNECT_ERR)     Blynk.virtualWrite(V1, tempBed);
    if (tempAmbient > SENSOR_DISCONNECT_ERR) Blynk.virtualWrite(V2, tempAmbient);
  }

  float activeTemp = (controlSensor == 1) ? tempChamber : tempBed;

  // 1. SAFETY: Emergency Over-temp
  if (tempChamber > MAX_SAFE_TEMP || tempBed > MAX_SAFE_TEMP || tempAmbient > MAX_SAFE_TEMP) {
    setSSR(false);
    heaterEnabled = false;
    timerActive = false;
    if (Blynk.connected()) {
      Blynk.virtualWrite(V3, 0);
      Blynk.virtualWrite(V8, 0);
    }
    bot.sendMessage(CHAT_ID, "🚨 *CRITICAL ALERT:* Over-temperature limit exceeded! Heater shut down immediately.", "Markdown");
    Serial.println("\n[EMERGENCY] Over-temp! SSR emergency shut off.");
    return;
  }

  // 2. SAFETY: Sensor Disconnect
  if (activeTemp <= SENSOR_DISCONNECT_ERR) {
    setSSR(false);
    return;
  }

  // 3. THERMOSTAT REGULATION
  if (heaterEnabled) {
    if (activeTemp < (targetTemp - hysteresis)) {
      setSSR(true);
    } else if (activeTemp >= (targetTemp + hysteresis)) {
      setSSR(false);
    }
  } else {
    setSSR(false);
  }
}

// Countdown Tick (Runs every 1s)
void timerCountdownTick() {
  if (timerActive && timerRemainingSeconds > 0) {
    timerRemainingSeconds--;
    if (Blynk.connected()) {
      Blynk.virtualWrite(V9, formatTime(timerRemainingSeconds));
    }

    if (timerRemainingSeconds == 0) {
      timerActive = false;
      heaterEnabled = false;
      setSSR(false);

      if (Blynk.connected()) {
        Blynk.virtualWrite(V8, 0);
        Blynk.virtualWrite(V3, 0);
        Blynk.virtualWrite(V9, "FINISHED");
      }

      // SEND NOTIFICATION TO TELEGRAM!
      bot.sendMessage(CHAT_ID, "🔔 *Heating Timer Finished!* Heater SSR turned OFF automatically.", "Markdown");
      Serial.println("\n[TIMER FINISHED] Heating duration elapsed. SSR turned OFF.");
    }
  }
}

// Print status to Serial Monitor every 2s
void printStatus() {
  Serial.print("[DATA] ");
  Serial.print("Chamber: ");
  if (tempChamber <= SENSOR_DISCONNECT_ERR) Serial.print("ERR");
  else Serial.printf("%.1f C", tempChamber);

  Serial.print(" | Bed: ");
  if (tempBed <= SENSOR_DISCONNECT_ERR) Serial.print("ERR");
  else Serial.printf("%.1f C", tempBed);

  Serial.print(" | Ambient: ");
  if (tempAmbient <= SENSOR_DISCONNECT_ERR) Serial.print("ERR");
  else Serial.printf("%.1f C", tempAmbient);

  Serial.printf(" | Target: %.1f C", targetTemp);
  Serial.printf(" | SSR: %s", ssrState ? "ON" : "OFF");
  Serial.printf(" | Timer: %s", timerActive ? formatTime(timerRemainingSeconds).c_str() : "IDLE");
  Serial.printf(" | WiFi: %s", (WiFi.status() == WL_CONNECTED) ? "NCAI" : "OFFLINE");
  Serial.printf(" | Blynk: %s\n", Blynk.connected() ? "ONLINE" : "OFFLINE");
}

// ==================== SETUP & MAIN LOOP ====================

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n===========================================================");
  Serial.println("  ESP32 3D Printer Controller (Blynk IoT + Telegram Bot)");
  Serial.println("===========================================================");

  // STEP 1: FORCE PIN 18 TO LOW (0V) IMMEDIATELY! (Strictly OFF on boot)
  pinMode(SSR_HEATER_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(SSR_HEATER_PIN, LOW);
  digitalWrite(STATUS_LED_PIN, LOW);
  ssrState = false;
  heaterEnabled = false;
  Serial.println("[SSR] Pin 18 set strictly to LOW (0V). SSR is OFF.");

  // Step 2: Start 1-Wire sensors (All 3 on Pin 4)
  sensors.begin();
  int count = sensors.getDeviceCount();
  Serial.printf("[HARDWARE] Found %d DS18B20 sensors on Pin 4.\n", count);

  // Step 3: Connect to WiFi & Blynk
  Serial.printf("[WIFI] Connecting to '%s' ...\n", ssid);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(">>> [WIFI] Connected to NCAI! <<<");
    Serial.print("[WIFI] IP Address: ");
    Serial.println(WiFi.localIP());
  }

  // Setup Telegram SSL
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Use Telegram root CA cert

  // Step 4: Periodic Timers
  timer.setInterval(1000L, readSensorsAndThermostat); // Sensors & SSR every 1s
  timer.setInterval(1000L, timerCountdownTick);       // Timer countdown every 1s
  timer.setInterval(2000L, printStatus);              // Real-time data every 2s

  Serial.println("\n--- Ready! Both Blynk App and Telegram Bot are Active! ---\n");
}

void loop() {
  Blynk.run();
  timer.run();

  // Check Telegram messages every 1 second
  if (millis() - lastTelegramCheck > TELEGRAM_INTERVAL) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleTelegramMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTelegramCheck = millis();
  }
}
