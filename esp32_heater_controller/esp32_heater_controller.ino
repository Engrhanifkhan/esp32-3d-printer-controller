/*
 * =========================================================================
 *   ESP32 3D PRINTER CONTROLLER - COMPLETE WEB DASHBOARD (PHONE & PC)
 * =========================================================================
 *  - 100% CONTROLLED VIA WEBSITE (Chrome / Safari on Phone or PC)
 *  - ZERO APP SETUP! Just open the website in your browser!
 *  - MIN & MAX Temperature Band Control:
 *      * Default: MIN = 40 °C, MAX = 45 °C
 *      * Temp < MIN (40 °C)  -> SSR turns ON (Heats up)
 *      * Temp >= MAX (45 °C) -> SSR turns OFF (Stops heating)
 *      * Easily adjust/increase MIN and MAX limits anytime on the website!
 *  - 3x DS18B20 Live Temperatures on 1 Pin (GPIO 4) with °C units
 *  - Custom Timer: Set 1 hour, 2 minutes, or ANY custom time!
 *      * Automatic shutoff when timer reaches 00:00:00
 *  - Solid State Relay (SSR) on GPIO 18 (Strictly OFF on boot)
 *  - Connects to WiFi "NCAI" (Password: 2026@ncai)
 *  - mDNS URL: http://printer.local
 * =========================================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ==================== PINS ====================
#define ONE_WIRE_BUS_PIN     4   // All 3 DS18B20 sensors on GPIO 4
#define SSR_HEATER_PIN      18   // Solid State Relay on GPIO 18
#define STATUS_LED_PIN       2   // ESP32 onboard LED

// ==================== SAFETY LIMITS ====================
#define MAX_SAFE_TEMP       110.0 // Hard emergency cutoff temp

// ==================== WIFI SETTINGS ====================
const char* ssid = "NCAI";
const char* pass = "2026@ncai";

// Backup Access Point (if NCAI is unreachable)
const char* ap_ssid = "ESP32-3DPrinter";
const char* ap_pass = "12345678";

// ==================== OBJECTS & STATE ====================
OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature sensors(&oneWire);
WebServer server(80);

// Live Temperatures
float temp1 = 0.0; // Chamber
float temp2 = 0.0; // Bed
float temp3 = 0.0; // Ambient
int sensorCount = 0;

// MIN & MAX Temperature Limits (Adjustable on Website)
float minTemp = 45.0; // Default: 45 °C
float maxTemp = 60.0; // Default: 60 °C
int controlSensor = 1; // 1 = Chamber, 2 = Bed

// Relay & Heater State
bool ssrState = false;
bool heaterEnabled = false;

// Timer Settings
bool timerActive = false;
int timerHours = 0;
int timerMinutes = 2; // Default 2 minutes
unsigned long timerTotalSeconds = 120;
unsigned long timerRemainingSeconds = 120;
unsigned long lastTimerTick = 0;

// Timing intervals
unsigned long lastSensorRead = 0;
unsigned long lastSerialPrint = 0;

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
}

// ==================== MODERN RESPONSIVE WEB DASHBOARD ====================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>3D Printer Controller</title>
  <style>
    :root {
      --primary: #0284c7;
      --bg: #f8fafc;
      --card: #ffffff;
      --text: #0f172a;
      --muted: #64748b;
      --danger: #dc2626;
      --success: #16a34a;
      --border: #e2e8f0;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background: var(--bg); color: var(--text); padding: 16px; min-height: 100vh; }
    .wrap { max-width: 520px; margin: 0 auto; }
    
    header { text-align: center; margin-bottom: 16px; }
    header h1 { font-size: 1.45rem; color: var(--primary); }
    header p { font-size: 0.85rem; color: var(--muted); margin-top: 4px; }
    
    .card { background: var(--card); border-radius: 12px; padding: 16px; margin-bottom: 14px; border: 1px solid var(--border); box-shadow: 0 2px 6px rgba(0,0,0,0.04); }
    .card h2 { font-size: 0.95rem; text-transform: uppercase; color: var(--muted); margin-bottom: 12px; letter-spacing: 0.05em; display: flex; justify-content: space-between; }
    
    /* Temperatures */
    .temps { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; }
    .temp-card { background: #f1f5f9; border-radius: 8px; padding: 10px 4px; text-align: center; border: 1px solid var(--border); }
    .temp-lbl { font-size: 0.75rem; color: var(--muted); font-weight: 600; }
    .temp-val { font-size: 1.5rem; font-weight: 800; color: #0284c7; margin: 4px 0; }
    
    /* SSR Status Box */
    .status-box { display: flex; align-items: center; justify-content: space-between; padding: 12px 16px; border-radius: 8px; margin-bottom: 14px; font-weight: 700; font-size: 1.05rem; }
    .status-on { background: #fee2e2; color: var(--danger); border: 1px solid #f87171; }
    .status-off { background: #f1f5f9; color: var(--muted); border: 1px solid var(--border); }
    
    /* Min/Max Controls */
    .limits-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 12px; }
    .limit-box { background: #f8fafc; border: 1px solid var(--border); border-radius: 8px; padding: 10px; text-align: center; }
    .limit-lbl { font-size: 0.8rem; color: var(--muted); font-weight: 600; margin-bottom: 6px; }
    .limit-input-wrap { display: flex; align-items: center; justify-content: center; gap: 4px; }
    .limit-btn { width: 32px; height: 32px; border: none; background: #e2e8f0; font-size: 1.2rem; font-weight: bold; border-radius: 6px; cursor: pointer; }
    .limit-val { font-size: 1.2rem; font-weight: 700; width: 60px; text-align: center; border: 1px solid #cbd5e1; border-radius: 6px; padding: 4px; }
    
    .row { display: flex; align-items: center; justify-content: space-between; margin-bottom: 12px; font-size: 0.9rem; }
    select { padding: 8px 10px; border: 1px solid #cbd5e1; border-radius: 6px; font-size: 0.95rem; background: #fff; }
    
    /* Timer */
    .timer-screen { font-size: 2.4rem; font-weight: 800; text-align: center; color: var(--primary); font-variant-numeric: tabular-nums; margin: 8px 0; }
    .quick-btns { display: flex; gap: 6px; margin-bottom: 12px; }
    .quick-btns button { flex: 1; padding: 8px 4px; font-size: 0.85rem; background: #e2e8f0; color: var(--text); border: none; border-radius: 6px; font-weight: 600; cursor: pointer; }
    
    .btn-row { display: flex; gap: 8px; margin-top: 10px; }
    button.btn { flex: 1; padding: 12px; border: none; border-radius: 8px; font-weight: 700; font-size: 0.95rem; cursor: pointer; transition: 0.15s; }
    .btn-green { background: var(--success); color: #fff; }
    .btn-red { background: var(--danger); color: #fff; }
    .btn-blue { background: var(--primary); color: #fff; }
    .btn-gray { background: #64748b; color: #fff; }
  </style>
</head>
<body>
  <div class="wrap">
    <header>
      <h1>3D Printer Control Center</h1>
      <p id="ip-info">Connected to WiFi: NCAI</p>
    </header>

    <!-- 1. LIVE TEMPERATURE READOUTS -->
    <div class="card">
      <h2>Live Temperatures (1-Wire Pin 4)</h2>
      <div class="temps">
        <div class="temp-card">
          <div class="temp-lbl">Sensor 1 (Chamber)</div>
          <div class="temp-val" id="t1">--</div>
          <small style="color:var(--muted);">&deg;C</small>
        </div>
        <div class="temp-card">
          <div class="temp-lbl">Sensor 2 (Bed)</div>
          <div class="temp-val" id="t2">--</div>
          <small style="color:var(--muted);">&deg;C</small>
        </div>
        <div class="temp-card">
          <div class="temp-lbl">Sensor 3 (Ambient)</div>
          <div class="temp-val" id="t3">--</div>
          <small style="color:var(--muted);">&deg;C</small>
        </div>
      </div>
    </div>

    <!-- 2. MIN & MAX TEMPERATURE CONTROLS -->
    <div class="card">
      <h2>Temperature Band Control</h2>
      <div class="limits-grid">
        <div class="limit-box">
          <div class="limit-lbl">MIN Temp (Heat ON)</div>
          <div class="limit-input-wrap">
            <button class="limit-btn" onclick="stepMin(-1)">-</button>
            <input type="number" id="min-in" class="limit-val" value="45" step="1">
            <button class="limit-btn" onclick="stepMin(1)">+</button>
          </div>
          <small style="color:var(--muted);">&deg;C</small>
        </div>
        <div class="limit-box">
          <div class="limit-lbl">MAX Temp (Heat OFF)</div>
          <div class="limit-input-wrap">
            <button class="limit-btn" onclick="stepMax(-1)">-</button>
            <input type="number" id="max-in" class="limit-val" value="60" step="1">
            <button class="limit-btn" onclick="stepMax(1)">+</button>
          </div>
          <small style="color:var(--muted);">&deg;C</small>
        </div>
      </div>

      <div class="row">
        <label>Control Sensor:</label>
        <select id="sensor-sel" onchange="updateLimits()">
          <option value="1">Sensor 1 (Chamber)</option>
          <option value="2">Sensor 2 (Bed)</option>
        </select>
      </div>

      <button class="btn btn-blue" style="width:100%;" onclick="updateLimits()">Apply Temperature Limits</button>
    </div>

    <!-- 3. RELAY STATUS & MANUAL SWITCH -->
    <div class="card">
      <h2>Solid State Relay (SSR Pin 18)</h2>
      <div class="status-box status-off" id="relay-status">
        <span id="relay-text">HEATER OFF</span>
        <span id="mode-text" style="font-size:0.8rem; font-weight:normal;">Idle</span>
      </div>

      <div class="btn-row">
        <button class="btn btn-green" onclick="toggleHeater(1)">Heater ON</button>
        <button class="btn btn-red" onclick="toggleHeater(0)">Heater OFF</button>
      </div>
    </div>

    <!-- 4. CUSTOM TIMER -->
    <div class="card">
      <h2>Heating Timer (Auto-Cutoff)</h2>
      <div class="timer-screen" id="timer-display">00:00:00</div>

      <div class="quick-btns">
        <button onclick="setQuickTime(0, 2)">2 min</button>
        <button onclick="setQuickTime(0, 15)">15 min</button>
        <button onclick="setQuickTime(0, 30)">30 min</button>
        <button onclick="setQuickTime(1, 0)">1 Hour</button>
        <button onclick="setQuickTime(2, 0)">2 Hours</button>
      </div>

      <div class="row">
        <label>Set Custom Duration:</label>
        <div>
          <input type="number" id="h-in" value="1" min="0" max="24" style="width:65px; padding:6px; border:1px solid #cbd5e1; border-radius:6px; text-align:center;"> hr
          <input type="number" id="m-in" value="0" min="0" max="59" style="width:65px; padding:6px; border:1px solid #cbd5e1; border-radius:6px; text-align:center; margin-left:4px;"> min
        </div>
      </div>

      <div class="btn-row">
        <button class="btn btn-blue" onclick="startTimer()">Start Timer</button>
        <button class="btn btn-gray" onclick="stopTimer()">Reset Timer</button>
      </div>
    </div>
  </div>

  <script>
    function update() {
      fetch('/api/status')
        .then(r => r.json())
        .then(d => {
          document.getElementById('t1').innerText = (d.t1 <= -50) ? 'ERR' : d.t1.toFixed(1);
          document.getElementById('t2').innerText = (d.t2 <= -50) ? 'ERR' : d.t2.toFixed(1);
          document.getElementById('t3').innerText = (d.t3 <= -50) ? 'ERR' : d.t3.toFixed(1);

          const rBox = document.getElementById('relay-status');
          const rTxt = document.getElementById('relay-text');
          if (d.ssrOn) {
            rBox.className = 'status-box status-on';
            rTxt.innerText = 'HEATER ON (HEATING)';
          } else {
            rBox.className = 'status-box status-off';
            rTxt.innerText = 'HEATER OFF';
          }

          document.getElementById('mode-text').innerText = d.heaterEnabled ? 'Thermostat Running' : 'Idle';
          document.getElementById('timer-display').innerText = d.timerStr;
        });
    }

    function stepMin(val) {
      const el = document.getElementById('min-in');
      el.value = Math.max(20, Math.min(100, parseFloat(el.value) + val));
      updateLimits();
    }

    function stepMax(val) {
      const el = document.getElementById('max-in');
      el.value = Math.max(20, Math.min(100, parseFloat(el.value) + val));
      updateLimits();
    }

    function updateLimits() {
      const mn = document.getElementById('min-in').value;
      const mx = document.getElementById('max-in').value;
      const sns = document.getElementById('sensor-sel').value;
      fetch(`/api/limits?min=${mn}&max=${mx}&sensor=${sns}`);
    }

    function toggleHeater(state) {
      fetch(`/api/heater?enable=${state}`);
    }

    function setQuickTime(h, m) {
      document.getElementById('h-in').value = h;
      document.getElementById('m-in').value = m;
      startTimer();
    }

    function startTimer() {
      const h = document.getElementById('h-in').value;
      const m = document.getElementById('m-in').value;
      updateLimits();
      fetch(`/api/timer?action=start&h=${h}&m=${m}`);
    }

    function stopTimer() {
      fetch('/api/timer?action=stop');
    }

    window.onload = () => {
      setInterval(update, 1500);
      update();
    };
  </script>
</body>
</html>
)rawliteral";

// ==================== THERMOSTAT (MIN / MAX) ====================

void readSensors() {
  sensors.requestTemperatures();
  float t1 = sensors.getTempCByIndex(0);
  float t2 = sensors.getTempCByIndex(1);
  float t3 = sensors.getTempCByIndex(2);

  temp1 = (t1 == DEVICE_DISCONNECTED_C) ? -127.0 : t1;
  temp2 = (t2 == DEVICE_DISCONNECTED_C) ? -127.0 : t2;
  temp3 = (t3 == DEVICE_DISCONNECTED_C) ? -127.0 : t3;
}

void regulateHeater() {
  // 1. Safety Over-temp Emergency Cutoff (> 110 C)
  if (temp1 > MAX_SAFE_TEMP || temp2 > MAX_SAFE_TEMP || temp3 > MAX_SAFE_TEMP) {
    setSSR(false);
    heaterEnabled = false;
    timerActive = false;
    Serial.println("\n[ALERT] Over-temp limit (110 C) exceeded! SSR Emergency Shutoff.");
    return;
  }

  // 2. MIN / MAX Temperature Band Regulation
  float activeTemp = (controlSensor == 1) ? temp1 : temp2;

  if (activeTemp > -100.0 && (heaterEnabled || timerActive)) {
    // Below MIN (e.g. 40 C) -> Turn ON SSR to heat up!
    if (activeTemp < minTemp) {
      if (!ssrState) {
        setSSR(true);
        Serial.printf("[THERMOSTAT] Temp (%.1f C) < MIN (%.1f C) -> SSR turned ON!\n", activeTemp, minTemp);
      }
    }
    // At or above MAX (e.g. 45 C) -> Turn OFF SSR to stop heating!
    else if (activeTemp >= maxTemp) {
      if (ssrState) {
        setSSR(false);
        Serial.printf("[THERMOSTAT] Temp (%.1f C) >= MAX (%.1f C) -> SSR turned OFF!\n", activeTemp, maxTemp);
      }
    }
  } else if (!heaterEnabled && !timerActive) {
    if (ssrState) setSSR(false); // System stopped -> Keep SSR OFF
  }
}

void handleTimerTick() {
  if (timerActive && timerRemainingSeconds > 0) {
    if (millis() - lastTimerTick >= 1000) {
      lastTimerTick = millis();
      timerRemainingSeconds--;
      if (timerRemainingSeconds == 0) {
        timerActive = false;
        heaterEnabled = false;
        setSSR(false);
        Serial.println("\n[TIMER FINISHED] Heating duration elapsed. SSR turned OFF automatically!");
      }
    }
  }
}

// ==================== HTTP API HANDLERS ====================

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleStatusApi() {
  String json = "{";
  json += "\"t1\":" + String(temp1, 1) + ",";
  json += "\"t2\":" + String(temp2, 1) + ",";
  json += "\"t3\":" + String(temp3, 1) + ",";
  json += "\"ssrOn\":" + String(ssrState ? "true" : "false") + ",";
  json += "\"heaterEnabled\":" + String(heaterEnabled ? "true" : "false") + ",";
  json += "\"minTemp\":" + String(minTemp, 1) + ",";
  json += "\"maxTemp\":" + String(maxTemp, 1) + ",";
  json += "\"timerActive\":" + String(timerActive ? "true" : "false") + ",";
  json += "\"timerStr\":\"" + (timerActive ? formatTime(timerRemainingSeconds) : "00:00:00") + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleLimitsApi() {
  if (server.hasArg("min")) minTemp = server.arg("min").toFloat();
  if (server.hasArg("max")) maxTemp = server.arg("max").toFloat();
  if (server.hasArg("sensor")) controlSensor = server.arg("sensor").toInt();
  Serial.printf("[WEBSITE] Limits updated: MIN=%.1f C | MAX=%.1f C\n", minTemp, maxTemp);
  regulateHeater();
  server.send(200, "text/plain", "OK");
}

void handleHeaterApi() {
  if (server.hasArg("enable")) {
    int en = server.arg("enable").toInt();
    heaterEnabled = (en == 1);
    if (heaterEnabled) {
      float activeTemp = (controlSensor == 1) ? temp1 : temp2;
      if (activeTemp < maxTemp) setSSR(true);
    } else {
      setSSR(false);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleTimerApi() {
  if (server.hasArg("action")) {
    String act = server.arg("action");
    if (act == "start") {
      int h = server.arg("h").toInt();
      int m = server.arg("m").toInt();

      unsigned long sec = ((unsigned long)h * 3600) + ((unsigned long)m * 60);
      if (sec == 0) sec = 120; // Default 2 mins

      timerHours = h;
      timerMinutes = m;
      timerTotalSeconds = sec;
      timerRemainingSeconds = sec;
      timerActive = true;
      heaterEnabled = true;
      lastTimerTick = millis();

      float activeTemp = (controlSensor == 1) ? temp1 : temp2;
      if (activeTemp < maxTemp) setSSR(true);

      Serial.printf("\n[WEBSITE TIMER] Started: %d hr %d min | Limits: %.1f C - %.1f C\n", h, m, minTemp, maxTemp);
    } else {
      timerActive = false;
      timerRemainingSeconds = 0;
      heaterEnabled = false;
      setSSR(false);
      Serial.println("\n[WEBSITE TIMER] Stopped. SSR turned OFF.");
    }
  }
  server.send(200, "text/plain", "OK");
}

// ==================== SETUP & LOOP ====================

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=======================================================");
  Serial.println("  ESP32 3D Printer Controller - Web Dashboard Starting ");
  Serial.println("=======================================================");

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
  sensorCount = sensors.getDeviceCount();
  Serial.printf("[HARDWARE] Found %d DS18B20 sensors on Pin 4.\n", sensorCount);

  // Step 3: Connect to WiFi "NCAI"
  Serial.printf("[WIFI] Connecting to '%s' ...\n", ssid);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, pass);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(">>> [WIFI] CONNECTED TO NCAI SUCCESSFULLY! <<<");
    Serial.print(">>> Open Website URL: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(">>> [WIFI] NCAI connection pending. Starting Hotspot... <<<");
  }

  // Backup Access Point (Always available!)
  WiFi.softAP(ap_ssid, ap_pass);
  Serial.printf(">>> Hotspot Backup: '%s' (Pass: %s) -> http://%s\n", ap_ssid, ap_pass, WiFi.softAPIP().toString().c_str());

  // Start mDNS (Open http://printer.local in browser!)
  if (MDNS.begin("printer")) {
    Serial.println(">>> mDNS Website URL: http://printer.local");
  }

  // Web Server Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatusApi);
  server.on("/api/limits", HTTP_GET, handleLimitsApi);
  server.on("/api/heater", HTTP_GET, handleHeaterApi);
  server.on("/api/timer", HTTP_GET, handleTimerApi);
  server.begin();

  Serial.println("\n--- Ready! Open http://printer.local in your browser! ---\n");
}

void loop() {
  server.handleClient();
  handleTimerTick();

  // Read sensors every 1s
  if (millis() - lastSensorRead >= 1000) {
    lastSensorRead = millis();
    readSensors();
    regulateHeater();
  }

  // Print status to Serial Monitor every 2s
  if (millis() - lastSerialPrint >= 2000) {
    lastSerialPrint = millis();
    Serial.print("[DATA] S1: ");
    if (temp1 <= -100.0) Serial.print("ERR");
    else Serial.printf("%.1f C", temp1);

    Serial.print(" | S2: ");
    if (temp2 <= -100.0) Serial.print("ERR");
    else Serial.printf("%.1f C", temp2);

    Serial.print(" | S3: ");
    if (temp3 <= -100.0) Serial.print("ERR");
    else Serial.printf("%.1f C", temp3);

    Serial.printf(" | Limits: [%.1f - %.1f C]", minTemp, maxTemp);
    Serial.printf(" | SSR: %s", ssrState ? "ON" : "OFF");
    Serial.printf(" | Timer: %s\n", timerActive ? formatTime(timerRemainingSeconds).c_str() : "IDLE");
  }
}
