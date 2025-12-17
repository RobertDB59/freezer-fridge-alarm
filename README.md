<h1>Freezer/Fridge Temperature Alarm with ATtiny85</h1>
<h2>Project Overview</h2>
<p>A ultra-low-power temperature monitoring system for freezer and refrigerator with audible/visual alarms. Uses an ATtiny85 microcontroller with 2× DS18B20 temperature sensors, designed to operate for <strong>25+ years</strong> on a single 18650 battery (3500mAh) with 5-minute check intervals.</p>

<h2>Key Features</h2>
<ul>
  <li><strong>Dual Temperature Monitoring</strong>: Simultaneous freezer (-15°C alarm) and fridge (8°C alarm) monitoring</li>
  <li><strong>Ultra-Low Power</strong>: Consumes ~12µA average, enabling decade-long battery life</li>
  <li><strong>Immediate Response</strong>: 5-minute check intervals for rapid door-open detection</li>
  <li><strong>Multi-Alert System</strong>: Distinct audio patterns + LED visual indication for different alarms</li>
  <li><strong>Battery Monitoring</strong>: TVL803 monitors battery voltage (alerts at <3.0V)</li>
  <li><strong>Sleep-Optimized</strong>: 98.4% duty cycle in deep sleep mode</li>
</ul>

<h2>Performance Specifications</h2>
<table>
  <tr>
    <th>Parameter</th>
    <th>Value</th>
  </tr>
  <tr>
    <td>Current Consumption</td>
    <td>~12µA average</td>
  </tr>
  <tr>
    <td>Check Interval</td>
    <td>5 minutes (adjustable)</td>
  </tr>
  <tr>
    <td>Temperature Resolution</td>
    <td>±0.5°C (9-bit DS18B20 mode)</td>
  </tr>
  <tr>
    <td>Alarm Response</td>
    <td><5 minutes from temperature breach</td>
  </tr>
  <tr>
    <td>Battery Life</td>
    <td>25+ years theoretical (3500mAh 18650)</td>
  </tr>
</table>

<h3>Alarm Patterns</h3>
<ul>
  <li><strong>Freezer</strong>: 3× fast beeps (2700Hz) + LED flashes</li>
  <li><strong>Fridge</strong>: 2× slow beeps (2000Hz) + LED flashes</li>
  <li><strong>Battery</strong>: 1× long beep (800Hz) + LED flash</li>
</ul>

<h2>Hardware Components</h2>
<table>
  <tr>
    <th>Component</th>
    <th>Quantity</th>
    <th>Purpose</th>
  </tr>
  <tr>
    <td>ATtiny85</td>
    <td>1</td>
    <td>Main microcontroller</td>
  </tr>
  <tr>
    <td>DS18B20</td>
    <td>2</td>
    <td>Temperature sensors (freezer + fridge)</td>
  </tr>
  <tr>
    <td>SI2301 P-MOSFET</td>
    <td>1</td>
    <td>Sensor power control</td>
  </tr>
  <tr>
    <td>TVL803-30</td>
    <td>1</td>
    <td>3.0V battery monitor</td>
  </tr>
  <tr>
    <td>Passive Buzzer (12085)</td>
    <td>1</td>
    <td>Audible alarms</td>
  </tr>
  <tr>
    <td>LED (Red)</td>
    <td>1</td>
    <td>Visual alarm indication</td>
  </tr>
  <tr>
    <td>18650 Battery + TP4056</td>
    <td>1</td>
    <td>Power + charging</td>
  </tr>
  <tr>
    <td>Resistors</td>
    <td>4</td>
    <td>150, 4k7, 10k and 100kΩ</td>
  </tr>
  <tr>
    <td>1N4148 Diode</td>
    <td>1</td>
    <td>Buzzer back-EMF protection</td>
  </tr>
</table>

<h2>Circuit Design</h2>

<h3>Power Management</h3>
<ul>
  <li><strong>SI2301 MOSFET</strong> gates sensor power (PB0 controlled)</li>
  <li><strong>TVL803</strong> only powered during readings (connected to MOSFET drain)</li>
  <li><strong>9-bit DS18B20</strong> resolution minimizes active time (94ms vs 750ms)</li>
</ul>

<h3>Sensor Interface</h3>
<ul>
  <li><strong>Both DS18B20s on PB2</strong> with shared 4.7kΩ pull-up</li>
  <li><strong>Individual addressing</strong> via stored ROM addresses</li>
  <li><strong>Sequential reading</strong> for minimal power consumption</li>
</ul>

<h3>Alarm Output</h3>
<ul>
  <li><strong>Buzzer + LED parallel</strong> on PB1 (150Ω series with LED only)</li>
  <li><strong>1N4148 diode</strong> across buzzer for protection</li>
  <li><strong>PWM tones</strong> generated in software</li>
</ul>

<h2>Firmware Architecture</h2>

<h3>State Machine</h3>
<pre>
STATE_SLEEP → STATE_CHECK_TEMP → STATE_ALARM_ACTIVE
    ^                                   |
    └───────────────────────────────────┘
</pre>

<h3>Power Optimization</h3>
<ul>
  <li><strong>Watchdog Timer</strong>: 8-second intervals for timing</li>
  <li><strong>Power-Down Sleep</strong>: 0.1µA + WDT (6µA)</li>
  <li><strong>ADC Disabled</strong>: Saves 200µA</li>
  <li><strong>9-bit DS18B20</strong>: 94ms conversion vs 750ms</li>
</ul>

<h3>Memory Usage (ATtiny85)</h3>
<ul>
  <li><strong>Flash</strong>: ~2.5KB (31% of 8KB)</li>
  <li><strong>RAM</strong>: ~100 bytes (20% of 512B)</li>
  <li><strong>EEPROM</strong>: Optional for sensor addresses</li>
</ul>

<h2>File Structure</h2>
<pre>
freezer_fridge_alarm/
├── firmware/
│   └── freezerFridgeAlarm.ino    # Main firmware
├── hardware/
│   ├── schematic.pdf             # Circuit diagram
│   └── pcb/                      # PCB design files
├── docs/
│   └── addressesGuide.md         # Sensor address reading guide
└── README.md                     # This file
</pre>

<h2>Configuration</h2>
<p>Edit these defines in the firmware:</p>
<pre><code>#define FREEZER_ALARM    -15.0    // Freezer alarm temperature
#define FRIDGE_ALARM      8.0     // Fridge alarm temperature  
#define TEMP_HYSTERESIS   2.0     // Hysteresis to clear alarms
#define TEMP_CHECK_MIN    5       // Check interval (minutes)
</code></pre>

<h2>Setup Instructions</h2>
<ol>
  <li><strong>Read DS18B20 Addresses</strong> using a separate Arduino sketch</li>
  <li><strong>Update addresses</strong> in firmware variables <code>freezerAddr[]</code> and <code>fridgeAddr[]</code></li>
  <li><strong>Assemble circuit</strong> following schematic</li>
  <li><strong>Upload firmware</strong> using Arduino IDE with ATtiny85 core</li>
  <li><strong>Test</strong> with temperature changes to verify alarms</li>
</ol>

<h2>Power Consumption Analysis</h2>
<table>
  <tr>
    <th>State</th>
    <th>Current</th>
    <th>Duration</th>
    <th>Duty Cycle</th>
    <th>Avg Current</th>
  </tr>
  <tr>
    <td>Deep Sleep</td>
    <td>7µA</td>
    <td>298.5s</td>
    <td>99.5%</td>
    <td>6.97µA</td>
  </tr>
  <tr>
    <td>Temperature Reading</td>
    <td>7mA</td>
    <td>0.2s</td>
    <td>0.07%</td>
    <td>4.67µA</td>
  </tr>
  <tr>
    <td><strong>Total</strong></td>
    <td colspan="3"></td>
    <td><strong>~11.64µA</strong></td>
  </tr>
</table>

<p><strong>Battery Life</strong>: 3500mAh × 76% (self-discharge) / 0.01164mA ≈ 25.3 years</p>

<h2>Troubleshooting</h2>
<table>
  <tr>
    <th>Issue</th>
    <th>Possible Cause</th>
    <th>Solution</th>
  </tr>
  <tr>
    <td>No temperature readings</td>
    <td>Wrong sensor addresses</td>
    <td>Read and update addresses</td>
  </tr>
  <tr>
    <td>Short battery life</td>
    <td>MOSFET leakage</td>
    <td>Check SI2301 gate resistor</td>
  </tr>
  <tr>
    <td>No alarm sound</td>
    <td>Buzzer/LED wiring</td>
    <td>Verify parallel connection</td>
  </tr>
  <tr>
    <td>False battery alarms</td>
    <td>TVL803 pull-up</td>
    <td>Ensure connected to MOSFET drain</td>
  </tr>
</table>

<h2>Future Enhancements</h2>
<ol>
  <li><strong>PB4 Utilization</strong>: Free pin for additional sensor/display</li>
  <li><strong>Wireless Alert</strong>: Add RF module for remote notifications</li>
  <li><strong>Temperature Logging</strong>: EEPROM storage for temperature history</li>
  <li><strong>Calibration Mode</strong>: Software calibration for temperature offsets</li>
</ol>

<h2>License</h2>
<p>MIT License - see LICENSE file for details</p>

<h2>Acknowledgments</h2>
<ul>
  <li>DS18B20 9-bit optimization for power savings</li>
  <li>ATtiny85 sleep mode techniques</li>
  <li>TVL803 battery monitoring circuit</li>
</ul>

<hr>
<p><em>Designed for reliability in critical temperature monitoring applications</em></p>
