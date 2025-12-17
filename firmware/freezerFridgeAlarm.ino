// ============================================================
// FREEZER/FRIDGE ALARM WITH BUZZER + LED IN PARALLEL
// ATtiny85 + 2× DS18B20 (same pin, 9-bit) + Buzzer/LED + TVL803
// Optimized for 5-minute checks with 25+ year battery life
// ============================================================

#include <avr/sleep.h>
#include <avr/wdt.h>

// ============================================================
// CONFIGURATION
// ============================================================
#define FREEZER_ALARM    -15.0    // Alarm if warmer than -15°C
#define FRIDGE_ALARM      8.0     // Alarm if warmer than 8°C
#define TEMP_HYSTERESIS   2.0     // Must cool this much below to clear
#define TEMP_CHECK_MIN    5       // Check temperature every 5 minutes

// Pin definitions (ATtiny85)
#define POWER_PIN     0    // PB0 - MOSFET control (SI2301)
#define BUZZER_PIN    1    // PB1 - Passive buzzer + LED in parallel
#define DS18B20_PIN   2    // PB2 - Both DS18B20 DATA pins
#define BATT_PIN      3    // PB3 - TVL803 battery monitor
// PB4 is free for future use

// SENSOR ADDRESSES - REPLACE WITH YOUR ACTUAL ADDRESSES!
// Read using separate sketch, format: {0x28, XX, XX, XX, XX, XX, XX, CRC}
uint8_t freezerAddr[8] = {0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // FREEZER
uint8_t fridgeAddr[8]  = {0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // FRIDGE

// Buzzer settings
#define BUZZ_FREEZER_FREQ  2700    // 2.7kHz for freezer alarm
#define BUZZ_FRIDGE_FREQ   2000    // 2.0kHz for fridge alarm
#define BUZZ_BATT_FREQ     800     // 800Hz for battery alarm
#define BUZZ_SHORT_MS      100     // Short beep duration
#define BUZZ_LONG_MS       500     // Long beep duration

// Timing (Watchdog ticks: 1 tick = 8 seconds)
#define TICKS_5_MIN    38      // 5 minutes = 38 ticks (304s)
#define TICKS_1_MIN    8       // 1 minute = 8 ticks (64s)
#define TICKS_30_SEC   4       // 30 seconds = 4 ticks (32s)

// DS18B20 9-bit resolution timing (94ms conversion)
#define DS18B20_CONVERSION_MS  94  // 9-bit conversion time

// ============================================================
// GLOBAL STATE VARIABLES
// ============================================================
enum AlarmState {
  ALARM_NONE,
  ALARM_FREEZER,
  ALARM_FRIDGE,
  ALARM_BATT
};

enum SystemState {
  STATE_SLEEP,
  STATE_CHECK_TEMP,
  STATE_ALARM_ACTIVE
};

volatile AlarmState currentAlarm = ALARM_NONE;
volatile SystemState systemState = STATE_SLEEP;
volatile uint32_t wdtTicks = 0;
volatile uint32_t lastTempCheck = 0;
volatile uint32_t lastBeepTime = 0;
volatile float freezerTemp = -50.0;
volatile float fridgeTemp = 25.0;
volatile bool batteryLow = false;

// ============================================================
// WATCHDOG TIMER SETUP
// ============================================================
void setupWatchdog(uint8_t prescaler) {
  // 0=16ms, 1=32ms, 2=64ms, 3=128ms, 4=250ms, 5=500ms
  // 6=1s, 7=2s, 8=4s, 9=8s
  MCUSR &= ~(1 << WDRF);
  WDTCR |= (1 << WDCE) | (1 << WDE);
  WDTCR = prescaler & 0x07;
  if (prescaler & 0x08) WDTCR |= (1 << WDP3);
  WDTCR |= (1 << WDIE);
}

ISR(WDT_vect) {
  wdtTicks++;
}

// ============================================================
// BUZZER CONTROL (with LED in parallel)
// ============================================================
void buzzerTone(uint16_t frequency, uint16_t duration) {
  // Simple PWM tone - LED flashes with buzzer
  uint32_t period = 1000000UL / frequency;  // Period in microseconds
  uint32_t cycles = (duration * 1000UL) / period;
  
  DDRB |= (1 << BUZZER_PIN);  // Set as output
  
  for (uint32_t i = 0; i < cycles; i++) {
    PORTB |= (1 << BUZZER_PIN);   // HIGH = Buzzer sounds + LED lights
    delayMicroseconds(period / 2);
    PORTB &= ~(1 << BUZZER_PIN);  // LOW = Both off
    delayMicroseconds(period / 2);
  }
  
  DDRB &= ~(1 << BUZZER_PIN);  // Back to input to save power
  PORTB &= ~(1 << BUZZER_PIN); // Ensure low
}

void handleAlarmBuzzer() {
  // Check if it's time to beep (every minute during alarm)
  if (wdtTicks - lastBeepTime < TICKS_1_MIN) {
    return;
  }
  
  switch(currentAlarm) {
    case ALARM_FREEZER:
      // Freezer alarm: 3 quick beeps (LED flashes 3 times)
      for (int i = 0; i < 3; i++) {
        buzzerTone(BUZZ_FREEZER_FREQ, BUZZ_SHORT_MS);
        if (i < 2) delay(BUZZ_SHORT_MS);
      }
      break;
      
    case ALARM_FRIDGE:
      // Fridge alarm: 2 slower beeps (LED flashes 2 times)
      for (int i = 0; i < 2; i++) {
        buzzerTone(BUZZ_FRIDGE_FREQ, BUZZ_SHORT_MS * 2);
        if (i < 1) delay(BUZZ_SHORT_MS * 2);
      }
      break;
      
    case ALARM_BATT:
      // Battery alarm: 1 long beep (LED lights for 500ms)
      buzzerTone(BUZZ_BATT_FREQ, BUZZ_LONG_MS);
      break;
      
    default:
      break;
  }
  
  lastBeepTime = wdtTicks;
}

// ============================================================
// OPTIMIZED 1-WIRE FUNCTIONS (minimal, fast)
// ============================================================
void oneWireLow(uint8_t pin, uint8_t us) {
  DDRB |= (1 << pin);
  PORTB &= ~(1 << pin);
  while (us--) _delay_us(1);
}

void oneWireHigh(uint8_t pin) {
  DDRB &= ~(1 << pin);
  PORTB |= (1 << pin);
  _delay_us(5);
}

uint8_t oneWireRead(uint8_t pin) {
  uint8_t b = 0;
  for (uint8_t i = 0; i < 8; i++) {
    oneWireLow(pin, 2);
    DDRB &= ~(1 << pin);
    _delay_us(8);
    if (PINB & (1 << pin)) b |= (1 << i);
    _delay_us(50);
  }
  return b;
}

void oneWireWrite(uint8_t pin, uint8_t b) {
  for (uint8_t i = 0; i < 8; i++) {
    if (b & 1) {
      oneWireLow(pin, 5);
      oneWireHigh(pin);
      _delay_us(55);
    } else {
      oneWireLow(pin, 60);
      oneWireHigh(pin);
    }
    b >>= 1;
  }
}

// ============================================================
// DS18B20 FUNCTIONS WITH 9-BIT RESOLUTION
// ============================================================
float readDS18B20ByAddr(uint8_t pin, uint8_t* addr) {
  // Reset
  oneWireLow(pin, 480);
  oneWireHigh(pin);
  _delay_us(70);
  _delay_us(410);
  
  // Match ROM command (address specific sensor)
  oneWireWrite(pin, 0x55);
  for (uint8_t i = 0; i < 8; i++) {
    oneWireWrite(pin, addr[i]);
  }
  
  // Set configuration to 9-bit resolution (R1=0, R0=0)
  oneWireWrite(pin, 0x4E); // Write scratchpad
  oneWireWrite(pin, 0x00); // TH register (unused)
  oneWireWrite(pin, 0x00); // TL register (unused)
  oneWireWrite(pin, 0x1F); // Configuration: 9-bit (R1=0, R0=0)
  
  // Start conversion with 9-bit resolution
  oneWireWrite(pin, 0x44);
  _delay_ms(DS18B20_CONVERSION_MS); // Only 94ms for 9-bit!
  
  // Reset again
  oneWireLow(pin, 480);
  oneWireHigh(pin);
  _delay_us(70);
  _delay_us(410);
  
  // Match ROM again
  oneWireWrite(pin, 0x55);
  for (uint8_t i = 0; i < 8; i++) {
    oneWireWrite(pin, addr[i]);
  }
  
  // Read scratchpad
  oneWireWrite(pin, 0xBE);
  
  uint8_t lsb = oneWireRead(pin);
  uint8_t msb = oneWireRead(pin);
  
  // Read and ignore remaining bytes (we only need first 2)
  for (uint8_t i = 0; i < 7; i++) oneWireRead(pin);
  
  // Convert to temperature (9-bit: shift right 3, 0.5°C resolution)
  int16_t temp16 = (msb << 8) | lsb;
  return temp16 / 16.0; // Still divide by 16 for proper scaling
}

// ============================================================
// POWER CONTROL
// ============================================================
void powerUpSensors() {
  // PB0 LOW turns on SI2301 P-MOSFET
  DDRB |= (1 << POWER_PIN);
  PORTB &= ~(1 << POWER_PIN);  // LOW = MOSFET ON
  _delay_ms(5);  // Short stabilization for 9-bit mode
}

void powerDownSensors() {
  // PB0 HIGH turns off SI2301
  PORTB |= (1 << POWER_PIN);   // HIGH = MOSFET OFF
  _delay_us(5);
  DDRB &= ~(1 << POWER_PIN);   // Input (high-Z)
  PORTB &= ~(1 << POWER_PIN);  // No pull-up
}

// ============================================================
// BATTERY CHECK (TVL803)
// ============================================================
void checkBattery() {
  // TVL803: LOW when battery < 3.0V
  DDRB &= ~(1 << BATT_PIN);
  PORTB |= (1 << BATT_PIN);  // Enable internal pull-up
  
  _delay_us(5);  // Minimal stabilization
  
  batteryLow = ((PINB & (1 << BATT_PIN)) == 0);
  
  PORTB &= ~(1 << BATT_PIN);  // Disable pull-up
}

// ============================================================
// TEMPERATURE CHECKING (OPTIMIZED)
// ============================================================
void checkTemperatures() {
  powerUpSensors();
  
  // Read both temperatures with 9-bit resolution
  // Sequential reading takes ~200ms total instead of 1500ms
  freezerTemp = readDS18B20ByAddr(DS18B20_PIN, freezerAddr);
  fridgeTemp = readDS18B20ByAddr(DS18B20_PIN, fridgeAddr);
  
  // Check battery
  checkBattery();
  
  powerDownSensors();
  
  // Update alarm state with hysteresis
  static bool freezerWasAlarming = false;
  static bool fridgeWasAlarming = false;
  static uint8_t batteryLowCount = 0;
  
  // Freezer alarm (highest priority)
  if (freezerTemp >= FREEZER_ALARM) {
    currentAlarm = ALARM_FREEZER;
    freezerWasAlarming = true;
  } else if (freezerTemp < (FREEZER_ALARM - TEMP_HYSTERESIS) && freezerWasAlarming) {
    if (currentAlarm == ALARM_FREEZER) {
      currentAlarm = ALARM_NONE;
    }
    freezerWasAlarming = false;
  }
  
  // Fridge alarm (only if freezer not alarming)
  if (currentAlarm == ALARM_NONE && fridgeTemp >= FRIDGE_ALARM) {
    currentAlarm = ALARM_FRIDGE;
    fridgeWasAlarming = true;
  } else if (fridgeTemp < (FRIDGE_ALARM - TEMP_HYSTERESIS) && fridgeWasAlarming) {
    if (currentAlarm == ALARM_FRIDGE) {
      currentAlarm = ALARM_NONE;
    }
    fridgeWasAlarming = false;
  }
  
  // Battery alarm with hysteresis (only if no temperature alarms)
  if (currentAlarm == ALARM_NONE) {
    if (batteryLow) {
      batteryLowCount++;
      if (batteryLowCount >= 3) {  // Low for 3 consecutive checks (15 min)
        currentAlarm = ALARM_BATT;
      }
    } else {
      batteryLowCount = 0;
      if (currentAlarm == ALARM_BATT) {
        currentAlarm = ALARM_NONE;
      }
    }
  }
  
  lastTempCheck = wdtTicks;
}

// ============================================================
// DELAY FUNCTIONS (OPTIMIZED)
// ============================================================
void delay(uint16_t ms) {
  for (uint16_t i = 0; i < ms; i++) {
    _delay_ms(1);
  }
}

void delayMicroseconds(uint16_t us) {
  for (uint16_t i = 0; i < us; i++) {
    _delay_us(1);
  }
}

// ============================================================
// SETUP (OPTIMIZED FOR POWER SAVING)
// ============================================================
void setup() {
  // All pins start as inputs (lowest power)
  DDRB = 0x00;
  PORTB = 0x00;
  
  // Disable ADC to save power (critical!)
  ADCSRA = 0;
  
  // Disable analog comparator
  ACSR |= (1 << ACD);
  
  // Setup watchdog for 8 seconds
  setupWatchdog(9);
  
  // Set sleep mode to power-down
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  
  // Enable interrupts
  sei();
  
  // Initial state
  systemState = STATE_SLEEP;
  currentAlarm = ALARM_NONE;
  
  // Initial temperature check
  checkTemperatures();
}

// ============================================================
// MAIN LOOP - STATE MACHINE
// ============================================================
void loop() {
  // Check if it's time for temperature check (every 5 minutes)
  if (wdtTicks - lastTempCheck >= TICKS_5_MIN) {
    systemState = STATE_CHECK_TEMP;
  }
  
  // Handle current state
  switch(systemState) {
    case STATE_SLEEP:
      if (currentAlarm == ALARM_NONE) {
        // Deep sleep if no alarm
        sleep_enable();
        sleep_cpu();
        sleep_disable();
      } else {
        // Alarm active, stay awake
        systemState = STATE_ALARM_ACTIVE;
      }
      break;
      
    case STATE_CHECK_TEMP:
      checkTemperatures();
      
      if (currentAlarm != ALARM_NONE) {
        systemState = STATE_ALARM_ACTIVE;
      } else {
        systemState = STATE_SLEEP;
      }
      break;
      
    case STATE_ALARM_ACTIVE:
      // Handle buzzer (LED flashes automatically)
      handleAlarmBuzzer();
      
      // Check temperatures more frequently during alarm (every minute)
      if (wdtTicks - lastTempCheck >= TICKS_1_MIN) {
        systemState = STATE_CHECK_TEMP;
      }
      
      // Brief sleep even during alarm to save power
      // WDT will wake us every 8 seconds for LED updates
      sleep_enable();
      sleep_cpu();
      sleep_disable();
      break;
  }
}