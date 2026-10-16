/*
 * DS18B20 1-Wire Address Finder & Real-Time Tester for ESP32
 * ---------------------------------------------------------
 * Connect ALL 3 sensors to GPIO 4 (with one 4.7k resistor between 3.3V and GPIO 4).
 * Open Serial Monitor at 115200 baud to see the unique 64-bit address of each sensor.
 * Touch each sensor with your fingers to see which temperature rises!
 */

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 4 // Pin connected to all 3 DS18B20 data wires

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int deviceCount = 0;
DeviceAddress tempDeviceAddress;

void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    if (i < 7) Serial.print(" ");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n==========================================");
  Serial.println("  DS18B20 1-Wire Scanner (All on 1 Pin)");
  Serial.println("==========================================");

  sensors.begin();
  deviceCount = sensors.getDeviceCount();
  Serial.printf("Found %d sensors connected on GPIO %d!\n\n", deviceCount, ONE_WIRE_BUS);

  for (int i = 0; i < deviceCount; i++) {
    if (sensors.getAddress(tempDeviceAddress, i)) {
      Serial.printf("Sensor #%d Address: { ", i + 1);
      for (uint8_t b = 0; b < 8; b++) {
        Serial.printf("0x%02X%s", tempDeviceAddress[b], (b < 7) ? ", " : " ");
      }
      Serial.println("}");
    }
  }

  Serial.println("\n--- Real-Time Temperature Monitor Starting ---");
  Serial.println("Touch one sensor with your fingers to see its temperature rise!\n");
}

void loop() {
  sensors.requestTemperatures();

  Serial.print("[Live Temperatures on 1 Pin] ");
  for (int i = 0; i < deviceCount; i++) {
    float tempC = sensors.getTempCByIndex(i);
    Serial.printf(" | Sensor %d: %.2f °C", i + 1, tempC);
  }
  Serial.println();

  delay(1500);
}
