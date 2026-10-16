/*
 * ==========================================================
 *        ARDUINO UNO / NANO DEDICATED RELAY TEST
 * ==========================================================
 * WIRING ON ARDUINO UNO:
 *  - Relay IN (Signal) -> Digital Pin 7
 *  - Relay VCC         -> Arduino 5V pin  (Crucial: 5V power!)
 *  - Relay GND         -> Arduino GND pin
 *
 * HOW IT WORKS:
 *  - By default: Cycles Relay ON for 2.5s and OFF for 2.5s.
 *  - Type '1' in Serial Monitor -> Force Relay ON permanently!
 *  - Type '0' in Serial Monitor -> Force Relay OFF permanently!
 *  - Type 'A' in Serial Monitor -> Return to Auto cycle.
 *  - Type 'I' in Serial Monitor -> INVERT (If relay turns ON with 0V / Active LOW).
 */

const int RELAY_PIN = 7;           // Arduino Digital Pin 7
const int TEST_LED  = LED_BUILTIN; // Pin 13 (Built-in Arduino LED)

bool manualMode = false;
bool currentRelayState = false;
bool activeLow = false;            // Flips to true if typed 'I'
unsigned long lastCycle = 0;

void setRelay(bool turnOn) {
  currentRelayState = turnOn;

  // Active LOW: 0V(LOW) = ON, 5V(HIGH) = OFF
  // Active HIGH: 5V(HIGH) = ON, 0V(LOW) = OFF
  int signal = turnOn ? (activeLow ? LOW : HIGH) : (activeLow ? HIGH : LOW);

  digitalWrite(RELAY_PIN, signal);
  digitalWrite(TEST_LED, turnOn ? HIGH : LOW);

  Serial.print(">>> RELAY IS: [ ");
  Serial.print(turnOn ? "ON" : "OFF");
  Serial.print(" ] | Pin 7 Voltage: ");
  Serial.println((signal == HIGH) ? "5V (HIGH)" : "0V (LOW)");
}

void setup() {
  Serial.begin(9600); // Standard Arduino Uno 9600 baud rate
  delay(1000);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(TEST_LED, OUTPUT);

  // Start with relay OFF
  setRelay(false);

  Serial.println("\n==============================================");
  Serial.println("     ARDUINO UNO DEDICATED RELAY TEST         ");
  Serial.println("==============================================");
  Serial.println("Relay Signal connected to: Digital Pin 7");
  Serial.println("Arduino 5V connected to:   Relay VCC");
  Serial.println("Arduino GND connected to:  Relay GND");
  Serial.println("----------------------------------------------");
  Serial.println("Commands in Serial Monitor (9600 baud):");
  Serial.println("  '1' -> Force Relay ON (Stays ON permanently)");
  Serial.println("  '0' -> Force Relay OFF (Stays OFF permanently)");
  Serial.println("  'A' -> Return to Automatic ON/OFF cycling");
  Serial.println("  'I' -> Invert logic (try this if relay is Active LOW!)");
  Serial.println("==============================================\n");
  Serial.println("[AUTO MODE] Cycling ON/OFF every 2.5 seconds...\n");
}

void loop() {
  // Read Serial Monitor commands
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') return; // Ignore Enter keys

    if (c == '1') {
      manualMode = true;
      Serial.println("\n[MANUAL] Command '1' -> Relay locked ON!");
      setRelay(true);
    } 
    else if (c == '0') {
      manualMode = true;
      Serial.println("\n[MANUAL] Command '0' -> Relay locked OFF!");
      setRelay(false);
    } 
    else if (c == 'A' || c == 'a') {
      manualMode = false;
      Serial.println("\n[AUTO] Resuming automatic cycle!");
    } 
    else if (c == 'I' || c == 'i') {
      activeLow = !activeLow;
      Serial.print("\n[INVERT] Relay logic flipped to: ");
      Serial.println(activeLow ? "ACTIVE LOW (0V triggers ON)" : "ACTIVE HIGH (5V triggers ON)");
      setRelay(currentRelayState);
    }
  }

  // Automatic cycle runs when not locked by 1 or 0
  if (!manualMode) {
    if (millis() - lastCycle >= 2500) {
      lastCycle = millis();
      setRelay(!currentRelayState);
    }
  }
}
