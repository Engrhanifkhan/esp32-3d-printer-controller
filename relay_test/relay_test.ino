/*
 * ==========================================================
 *        DEDICATED RELAY TEST WITH PERSISTENT 1/0 COMMANDS
 * ==========================================================
 * - Default: Cycles relay ON/OFF automatically every 2 seconds.
 * - Type '1' in Serial Monitor -> Relay stays ON permanently!
 * - Type '0' in Serial Monitor -> Relay stays OFF permanently!
 * - Type 'A' in Serial Monitor -> Resume automatic ON/OFF cycle.
 * - Type 'I' in Serial Monitor -> INVERT (switches between Active HIGH and Active LOW)
 *
 * Board Pin Mapping:
 * - ESP32:       Relay on Pin 18, Onboard LED on Pin 2
 * - Arduino Uno: Relay on Pin 7,  Onboard LED on Pin 13
 */

#if defined(ESP32)
  #define RELAY_PIN   18
  #define TEST_LED     2
  #define BAUD_RATE 115200
#else
  #define RELAY_PIN    7
  #define TEST_LED LED_BUILTIN
  #define BAUD_RATE 9600
#endif

bool manualMode = false;       // If true, manual 1/0 command locks the state
bool currentRelayState = false;// true = ON, false = OFF
bool activeLow = false;        // Set to true if relay turns ON with 0V (LOW)
unsigned long lastCycleTime = 0;

void applyRelayState(bool turnOn) {
  currentRelayState = turnOn;
  
  // If Active LOW: LOW(0V) is ON, HIGH(5V/3.3V) is OFF
  // If Active HIGH: HIGH is ON, LOW is OFF
  int pinSignal = turnOn ? (activeLow ? LOW : HIGH) : (activeLow ? HIGH : LOW);
  
  digitalWrite(RELAY_PIN, pinSignal);
  digitalWrite(TEST_LED, turnOn ? HIGH : LOW);

  Serial.print(">>> RELAY IS NOW: [ ");
  Serial.print(turnOn ? "ON" : "OFF");
  Serial.print(" ] | Output Voltage on Pin: ");
  Serial.println((pinSignal == HIGH) ? "HIGH" : "LOW (0V)");
}

void setup() {
  Serial.begin(BAUD_RATE);
  delay(1000);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(TEST_LED, OUTPUT);

  // Default initial state: OFF
  applyRelayState(false);

  Serial.println("\n==============================================");
  Serial.println("        DEDICATED RELAY HARDWARE TEST         ");
  Serial.println("==============================================");
  Serial.print("Board Detected: ");
  #if defined(ESP32)
    Serial.println("ESP32 (Relay Pin: 18)");
  #else
    Serial.println("Arduino Uno/Nano (Relay Pin: 7)");
  #endif
  Serial.println("\nCOMMANDS in Serial Monitor:");
  Serial.println("  '1' -> Force Relay ON (Stays ON permanently)");
  Serial.println("  '0' -> Force Relay OFF (Stays OFF permanently)");
  Serial.println("  'A' -> Return to Automatic ON/OFF cycle");
  Serial.println("  'I' -> Invert logic (try this if your relay is Active LOW!)");
  Serial.println("==============================================\n");
  Serial.println("[AUTO MODE] Cycling Relay ON for 2s, OFF for 2s by default...\n");
}

void loop() {
  // 1. Process Serial Commands
  if (Serial.available() > 0) {
    char c = Serial.read();
    
    // Ignore newline / carriage return characters
    if (c == '\r' || c == '\n') return;

    if (c == '1') {
      manualMode = true;
      Serial.println("\n[MANUAL LOCK] Command '1' received!");
      applyRelayState(true);
    } 
    else if (c == '0') {
      manualMode = true;
      Serial.println("\n[MANUAL LOCK] Command '0' received!");
      applyRelayState(false);
    } 
    else if (c == 'A' || c == 'a') {
      manualMode = false;
      Serial.println("\n[AUTO MODE] Resuming automatic ON/OFF cycling!");
    } 
    else if (c == 'I' || c == 'i') {
      activeLow = !activeLow;
      Serial.print("\n[LOGIC CHANGED] Relay is now set to: ");
      Serial.println(activeLow ? "ACTIVE LOW (0V / LOW triggers relay ON)" : "ACTIVE HIGH (HIGH triggers relay ON)");
      applyRelayState(currentRelayState);
    }
  }

  // 2. Default Automatic Cycle (runs only when not locked by 1 or 0)
  if (!manualMode) {
    if (millis() - lastCycleTime >= 2500) { // Toggle every 2.5 seconds
      lastCycleTime = millis();
      applyRelayState(!currentRelayState);
    }
  }
}
