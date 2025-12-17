# DS18B20 Address Reading Guide

<h2>Overview</h2>
<p>This guide explains how to read the unique 64-bit ROM addresses of your DS18B20 temperature sensors. Each sensor has a unique address that must be programmed into the alarm system firmware.</p>

<h2>Why Addresses Matter</h2>
<p>When multiple DS18B20 sensors share the same 1-Wire bus (same data pin), the microcontroller needs to know which sensor is which. The addresses allow us to:</p>
<ul>
  <li>Identify which sensor is in the freezer vs fridge</li>
  <li>Read temperatures from specific sensors individually</li>
  <li>Replace sensors without confusing the system</li>
</ul>

<h2>Required Equipment</h2>
<table>
  <tr>
    <th>Item</th>
    <th>Purpose</th>
  </tr>
  <tr>
    <td>Arduino Uno/Nano (or any Arduino)</td>
    <td>Temporary reader</td>
  </tr>
  <tr>
    <td>USB cable</td>
    <td>Power and programming</td>
  </tr>
  <tr>
    <td>Breadboard</td>
    <td>Temporary connections</td>
  </tr>
  <tr>
    <td>4.7kΩ resistor</td>
    <td>1-Wire pull-up resistor</td>
  </tr>
  <tr>
    <td>Jumper wires</td>
    <td>Connections</td>
  </tr>
  <tr>
    <td>Your DS18B20 sensors</td>
    <td>To read addresses from</td>
  </tr>
</table>

<h2>Wiring Diagram</h2>
<pre>
DS18B20 #1 (Freezer)          DS18B20 #2 (Fridge)
      │                             │
      ├─────────────┬───────────────┤
      │             │               │
    VCC (Red)     DATA (Yellow)   GND (Black)
      │             │               │
      │             │               │
    5V ────────────┴─────────────── GND
                    │
                  4.7kΩ
                    │
                  5V
</pre>

<h3>Connection Details:</h3>
<ul>
  <li><strong>All VCC pins</strong> → 5V (Arduino 5V pin)</li>
  <li><strong>All GND pins</strong> → GND (Arduino GND pin)</li>
  <li><strong>All DATA pins</strong> → Connected together</li>
  <li><strong>DATA bus</strong> → Arduino pin 2 (via 4.7kΩ pull-up to 5V)</li>
</ul>

<h2>Arduino Sketch for Reading Addresses</h2>
<pre><code>#include &lt;OneWire.h&gt;

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance
OneWire oneWire(ONE_WIRE_BUS);

void setup(void) {
  Serial.begin(9600);
  Serial.println("DS18B20 Address Reader");
  Serial.println("======================");
}

void loop(void) {
  byte addr[8];
  int sensorCount = 0;
  
  Serial.println("\nSearching for DS18B20 sensors...");
  
  oneWire.reset_search();
  delay(250);
  
  while (oneWire.search(addr)) {
    Serial.print("\nSensor ");
    Serial.print(++sensorCount);
    Serial.print(" Address: ");
    
    // Print address in hex format
    for (int i = 0; i < 8; i++) {
      if (addr[i] < 16) Serial.print("0");
      Serial.print(addr[i], HEX);
      if (i < 7) Serial.print(", ");
    }
    
    Serial.println();
    
    // Verify CRC (8th byte is CRC)
    if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
    }
    
    // Identify sensor type
    Serial.print("Sensor Type: ");
    switch (addr[0]) {
      case 0x10:
        Serial.println("DS18S20 or older DS1820");
        break;
      case 0x28:
        Serial.println("DS18B20");
        break;
      case 0x22:
        Serial.println("DS1822");
        break;
      default:
        Serial.println("Unknown device");
    }
    
    delay(2000);
  }
  
  Serial.println("\nNo more addresses.");
  Serial.println("Done!");
  
  while(1); // Stop here
}
</code></pre>

<h2>📋 Step-by-Step Process</h2>

<h3>Step 1: Connect Sensors</h3>
<ol>
  <li>Connect both DS18B20 sensors in parallel on the breadboard</li>
  <li>Connect DATA pins together to Arduino pin 2</li>
  <li>Add 4.7kΩ resistor between DATA and 5V</li>
  <li>Connect all VCC to 5V</li>
  <li>Connect all GND to GND</li>
</ol>

<h3>Step 2: Upload and Run Sketch</h3>
<ol>
  <li>Open Arduino IDE</li>
  <li>Install OneWire library (if not installed)</li>
  <li>Copy the sketch above</li>
  <li>Upload to Arduino</li>
  <li>Open Serial Monitor (9600 baud)</li>
</ol>

<h3>Step 3: Record Addresses</h3>
<p>The Serial Monitor will display something like:</p>
<pre>
Sensor 1 Address: 0x28, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0xCRC
Sensor Type: DS18B20

Sensor 2 Address: 0x28, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0xCRC  
Sensor Type: DS18B20
</pre>

<h2>Identifying Freezer vs Fridge Sensors</h2>

<h3>Method 1: Temperature Test</h3>
<pre><code>// Modified sketch to identify by temperature
#include &lt;OneWire.h&gt;
#include &lt;DallasTemperature.h&gt;

#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress freezerAddr, fridgeAddr;

void setup(void) {
  Serial.begin(9600);
  sensors.begin();
  
  Serial.println("Finding sensors...");
  int deviceCount = sensors.getDeviceCount();
  Serial.print("Found ");
  Serial.print(deviceCount);
  Serial.println(" devices");
  
  // Get addresses
  if (!sensors.getAddress(freezerAddr, 0)) {
    Serial.println("Unable to find address for Device 0"); 
  }
  if (!sensors.getAddress(fridgeAddr, 1)) {
    Serial.println("Unable to find address for Device 1"); 
  }
  
  Serial.println("\nPut FIRST sensor in FREEZER, SECOND in FRIDGE");
  Serial.println("Then send any character to continue...");
  while(!Serial.available());
  Serial.read();
  
  sensors.requestTemperatures();
  
  float temp1 = sensors.getTempC(freezerAddr);
  float temp2 = sensors.getTempC(fridgeAddr);
  
  Serial.println("\nResults:");
  Serial.print("Sensor 1: ");
  printAddress(freezerAddr);
  Serial.print(" = ");
  Serial.print(temp1);
  Serial.println("°C");
  
  Serial.print("Sensor 2: ");
  printAddress(fridgeAddr);
  Serial.print(" = ");
  Serial.print(temp2);
  Serial.println("°C");
  
  if (temp1 < temp2) {
    Serial.println("\n✓ Sensor 1 is FREEZER sensor (colder)");
    Serial.println("  Sensor 2 is FRIDGE sensor (warmer)");
  } else {
    Serial.println("\n✓ Sensor 2 is FREEZER sensor (colder)");
    Serial.println("  Sensor 1 is FRIDGE sensor (warmer)");
  }
}

void printAddress(DeviceAddress addr) {
  for (uint8_t i = 0; i < 8; i++) {
    if (addr[i] < 16) Serial.print("0");
    Serial.print(addr[i], HEX);
    if (i < 7) Serial.print(", ");
  }
}

void loop(void) {
  // Nothing here
}
</code></pre>

<h3>Method 2: Physical Labeling</h3>
<ol>
  <li>Connect sensors ONE AT A TIME</li>
  <li>Run address reader with only one sensor connected</li>
  <li>Label the sensor with its address (use sticker)</li>
  <li>Repeat for second sensor</li>
</ol>

<h2>Updating the Alarm Firmware</h2>

<h3>Step 1: Find these lines in the main firmware:</h3>
<pre><code>// SENSOR ADDRESSES - REPLACE WITH YOUR ACTUAL ADDRESSES!
uint8_t freezerAddr[8] = {0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t fridgeAddr[8]  = {0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
</code></pre>

<h3>Step 2: Replace with your addresses:</h3>
<pre><code>// Freezer sensor (colder location)
uint8_t freezerAddr[8] = {0x28, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0xCRC};

// Fridge sensor (warmer location)  
uint8_t fridgeAddr[8]  = {0x28, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0xCRC};
</code></pre>

<p><strong>Note:</strong> Keep the <code>0x</code> prefix and commas exactly as shown!</p>

<h2>🔧 Testing the Configuration</h2>

<h3>After programming the ATtiny85:</h3>
<ol>
  <li>Connect sensors to the final circuit</li>
  <li>Power on the system</li>
  <li>Wait 5 minutes for first reading</li>
  <li>Verify correct temperatures are displayed/alarmed</li>
  <li>Test by warming each sensor with your fingers</li>
</ol>

<h2>Troubleshooting</h2>

<table>
  <tr>
    <th>Problem</th>
    <th>Solution</th>
  </tr>
  <tr>
    <td>No addresses found</td>
    <td>Check 4.7kΩ pull-up resistor is connected</td>
  </tr>
  <tr>
    <td>Only one address found</td>
    <td>Check both sensors are properly powered (VCC to 5V)</td>
  </tr>
  <tr>
    <td>CRC errors</td>
    <td>Check wiring, try shorter cables, add 100nF capacitor across VCC/GND</td>
  </tr>
  <tr>
    <td>Wrong temperatures</td>
    <td>You may have swapped freezer/fridge addresses</td>
  </tr>
</table>

<h2>Pro Tips</h2>
<ul>
  <li><strong>Label sensors physically</strong> with their addresses on stickers</li>
  <li><strong>Keep a backup</strong> of addresses in a text file</li>
  <li><strong>Test before final installation</strong> to avoid taking everything apart later</li>
  <li><strong>New sensors</strong> must have their addresses updated in the firmware</li>
</ul>

<h2>Address Format Explained</h2>
<p>DS18B20 64-bit address structure:</p>
<pre>
Byte 0: Family code (0x28 for DS18B20)
Byte 1-6: 48-bit serial number (unique)
Byte 7: CRC (Cyclic Redundancy Check)
</pre>

<p>Example: <code>{0x28, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0xCRC}</code></p>

<h2>Additional Resources</h2>
<ul>
  <li><a href="https://github.com/PaulStoffregen/OneWire">OneWire Library Documentation</a></li>
  <li><a href="https://www.maximintegrated.com/en/products/sensors/DS18B20.html">DS18B20 Datasheet</a></li>
  <li><a href="https://create.arduino.cc/projecthub/TheGadgetBoy/ds18b20-digital-temperature-sensor-and-arduino-9cc806">DS18B20 Tutorial</a></li>
</ul>

<hr>
<p><strong>Remember:</strong> You only need to do this once per sensor! After programming the addresses into the ATtiny85 firmware, the system will always know which sensor is which.</p>
