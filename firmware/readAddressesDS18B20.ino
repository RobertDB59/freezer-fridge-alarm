/*
=============================================
DS18B20 ADDRESS READER
=============================================
Purpose: Read and display 64-bit ROM addresses
         of DS18B20 temperature sensors
         
Connections:
- DS18B20 DATA → Arduino pin 2
- DS18B20 VCC  → Arduino 5V
- DS18B20 GND  → Arduino GND
- 4.7kΩ resistor between DATA and 5V

Upload this to any Arduino board (Uno, Nano, etc.)
Open Serial Monitor at 9600 baud
=============================================
*/

#include <OneWire.h>

// ===========================================
// CONFIGURATION
// ===========================================
#define ONE_WIRE_BUS 2      // Data pin connected to Arduino pin 2
#define SERIAL_BAUD  9600   // Serial monitor baud rate

// Create OneWire instance
OneWire oneWire(ONE_WIRE_BUS);

// ===========================================
// SETUP - RUNS ONCE
// ===========================================
void setup() {
  // Initialize serial communication
  Serial.begin(SERIAL_BAUD);
  
  // Wait for serial port to connect (for boards with native USB)
  while (!Serial) {
    delay(10);
  }
  
  Serial.println();
  Serial.println("=========================================");
  Serial.println("      DS18B20 ADDRESS READER");
  Serial.println("=========================================");
  Serial.println();
  Serial.println("Instructions:");
  Serial.println("1. Connect DS18B20 sensors to pin 2");
  Serial.println("2. Use 4.7kΩ pull-up between DATA and 5V");
  Serial.println("3. All sensors must share DATA, VCC, GND");
  Serial.println("4. Open Serial Monitor (9600 baud)");
  Serial.println("=========================================");
  Serial.println();
  
  delay(2000); // Give user time to read instructions
}

// ===========================================
// MAIN LOOP
// ===========================================
void loop() {
  byte addr[8];          // Array to hold 8-byte address
  int sensorCount = 0;   // Count of found sensors
  
  Serial.println();
  Serial.println(" Searching for DS18B20 sensors...");
  Serial.println("-----------------------------------------");
  
  // Reset the 1-Wire bus search
  oneWire.reset_search();
  delay(250); // Short delay for stability
  
  // Search for all devices on the bus
  while (oneWire.search(addr)) {
    sensorCount++;
    
    Serial.print(" Sensor #");
    Serial.print(sensorCount);
    Serial.println(" Found!");
    Serial.println("-----------------------------------------");
    
    // Print the 8-byte address in HEX format
    Serial.print("Address (HEX): {");
    for (int i = 0; i < 8; i++) {
      // Print with leading zero if needed
      if (addr[i] < 16) Serial.print("0");
      Serial.print(addr[i], HEX);
      
      if (i < 7) {
        Serial.print(", ");
      }
    }
    Serial.println("}");
    
    // Print the 8-byte address in DECIMAL format
    Serial.print("Address (DEC): {");
    for (int i = 0; i < 8; i++) {
      Serial.print(addr[i]);
      if (i < 7) {
        Serial.print(", ");
      }
    }
    Serial.println("}");
    
    // Verify CRC (8th byte is CRC)
    Serial.print("CRC Check: ");
    if (OneWire::crc8(addr, 7) == addr[7]) {
      Serial.println(" PASSED");
    } else {
      Serial.println(" FAILED - Check wiring!");
    }
    
    // Identify sensor type based on family code
    Serial.print("Sensor Type: ");
    switch (addr[0]) {
      case 0x10:
        Serial.println("DS18S20 or older DS1820");
        break;
      case 0x28:
        Serial.println(" DS18B20 (correct for this project)");
        break;
      case 0x22:
        Serial.println("DS1822");
        break;
      default:
        Serial.print("Unknown device (Family Code: 0x");
        if (addr[0] < 16) Serial.print("0");
        Serial.print(addr[0], HEX);
        Serial.println(")");
    }
    
    // Calculate unique 48-bit serial number
    Serial.print("Serial Number: ");
    for (int i = 1; i <= 6; i++) {
      if (addr[i] < 16) Serial.print("0");
      Serial.print(addr[i], HEX);
    }
    Serial.println();
    
    Serial.println("-----------------------------------------");
    
    delay(1000); // Pause between sensors
  }
  
  // Display search results summary
  Serial.println();
  Serial.println("=========================================");
  Serial.print(" Search Complete: ");
  Serial.print(sensorCount);
  Serial.println(" sensor(s) found");
  Serial.println("=========================================");
  
  if (sensorCount == 0) {
    Serial.println(" No sensors found!");
    Serial.println();
    Serial.println("Troubleshooting:");
    Serial.println("1. Check DATA pin connection (Pin 2)");
    Serial.println("2. Verify 4.7kΩ pull-up resistor");
    Serial.println("3. Ensure sensors have power (VCC to 5V)");
    Serial.println("4. Check all GND connections");
    Serial.println("5. Try connecting sensors one at a time");
  }
  
  Serial.println();
  Serial.println(" Restarting search in 10 seconds...");
  Serial.println("=========================================");
  
  delay(10000); // Wait 10 seconds before next search
}

// ===========================================
// HELPER FUNCTION: PRINT FORMATTED ADDRESS
// ===========================================
void printFormattedAddress(byte* addr) {
  Serial.println();
  Serial.println(" Copy this for your ATtiny85 code:");
  Serial.println("-----------------------------------------");
  
  // Format for C/C++ array (freezer)
  Serial.println("Freezer sensor address:");
  Serial.print("uint8_t freezerAddr[8] = {");
  for (int i = 0; i < 8; i++) {
    Serial.print("0x");
    if (addr[i] < 16) Serial.print("0");
    Serial.print(addr[i], HEX);
    if (i < 7) Serial.print(", ");
  }
  Serial.println("};");
  
  Serial.println();
  Serial.println("Fridge sensor address (if second sensor):");
  Serial.println("uint8_t fridgeAddr[8]  = {0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};");
  Serial.println("// ^ Replace with actual address");
  Serial.println("-----------------------------------------");
}