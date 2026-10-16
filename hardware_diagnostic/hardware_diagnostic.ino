/*
 * ESP32 HARDWARE DIAGNOSTIC: WIFI & RELAY TEST
 * --------------------------------------------
 * 1. Checks exact reason why WiFi fails (Wrong password, MAC filter, or router rejection)
 * 2. Prints your ESP32 MAC Address (needed if your office blocks unknown devices)
 * 3. Cycles Relay pin 18 ON (HIGH) and OFF (LOW) every 2 seconds to test relay hardware
 */

#include <WiFi.h>

#define RELAY_PIN 18
#define LED_PIN    2

const char* ssid = "NCAI";
const char* pass = "2026@ncai";

void printWiFiError(wl_status_t status) {
  switch (status) {
    case WL_NO_SSID_AVAIL:
      Serial.println(">>> ERROR: 'NCAI' SSID NOT FOUND! (Check router distance / 2.4GHz)");
      break;
    case WL_CONNECT_FAILED:
      Serial.println(">>> ERROR: WRONG PASSWORD or ROUTER REJECTED CONNECTION!");
      break;
    case WL_CONNECTION_LOST:
      Serial.println(">>> ERROR: Connection lost.");
      break;
    case WL_DISCONNECTED:
      Serial.println(">>> ERROR: Disconnected by router (Possible MAC Address Filtering or Enterprise Login required).");
      break;
    default:
      Serial.printf(">>> ERROR: Status Code %d\n", status);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=======================================================");
  Serial.println("        ESP32 HARDWARE & RELAY DIAGNOSTIC TOOL         ");
  Serial.println("=======================================================");

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // 1. Print ESP32 MAC Address
  WiFi.mode(WIFI_STA);
  Serial.print("[INFO] ESP32 MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("[NOTE] If NCAI has MAC filtering, give this MAC address to your IT admin.\n");

  // 2. Test WiFi Connection to NCAI
  Serial.printf("[WIFI TEST] Connecting to '%s' with password '%s' ...\n", ssid, pass);
  WiFi.begin(ssid, pass);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(">>> SUCCESS! WiFi Connected!");
    Serial.print(">>> IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.printf(">>> Signal Strength: %d dBm\n\n", WiFi.RSSI());
  } else {
    printWiFiError(WiFi.status());
    Serial.println();
  }

  Serial.println("=======================================================");
  Serial.println("  STARTING RELAY TEST (Cycles every 2 seconds)");
  Serial.println("  Watch the Relay LED and listen for clicks!");
  Serial.println("=======================================================\n");
}

int cycle = 0;
void loop() {
  cycle++;
  Serial.printf("--- Test Cycle #%d ---\n", cycle);

  // State 1: HIGH (3.3V)
  Serial.println("[RELAY] Pin 18 set to HIGH (3.3V) - LED ON");
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
  delay(2000);

  // State 2: LOW (0V)
  Serial.println("[RELAY] Pin 18 set to LOW (0V)   - LED OFF");
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  delay(2000);
}
