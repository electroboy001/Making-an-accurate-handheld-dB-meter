/*
 * Ultra-Stable Handheld dBA Meter
 * DFRobot SEN0232 + Arduino + SSD1306 OLED
 *
 * Features:
 * - Real-time dBA reading with large font
 * - Visual bar graph
 * - Min/Max tracking with reset
 * - Running average for ultra-stable readings
 * - Dynamic Vcc compensation (battery safe)
 * - Sound level descriptor
 *
 * Wiring:
 *   SEN0232 A (Green)  -> A0
 *   SEN0232 + (Red)    -> 5V
 *   SEN0232 - (Black)  -> GND
 *   OLED SDA           -> A4
 *   OLED SCL           -> A5
 *   OLED VCC           -> 5V
 *   OLED GND           -> GND
 *
 * Author: Sagar Saini
 * License: MIT
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---- Display Configuration ----
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---- Sensor Configuration ----
#define SENSOR_PIN A0
#define SENSITIVITY 50.0   // dBA per Volt (sensor characteristic)

// ---- Averaging Configuration ----
#define NUM_SAMPLES    10   // Circular buffer size
#define SAMPLE_DELAY   125  // ms (matches sensor's 125ms response time)

// ---- Variables ----
float samples[NUM_SAMPLES];
int sampleIndex = 0;
bool bufferFull = false;
float minDb = 999.0;
float maxDb = 0.0;
unsigned long lastSampleTime = 0;

// =============================================
// Accurate Vcc Reading (essential for battery)
// Reads internal 1.1V bandgap against AVcc
// Returns actual Vcc in millivolts
// =============================================
long readVcc() {
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    // ATmega328P (Arduino Uno/Nano)
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif

  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));

  uint8_t low  = ADCL;
  uint8_t high = ADCH;
  long result = (high << 8) | low;

  // 1.1V * 1023 * 1000 = 1125300
  result = 1125300L / result;
  return result;  // Vcc in millivolts
}

// =============================================
// Read Sound Level with Averaging
// =============================================
float readSoundLevel() {
  float refVolt = float(readVcc()) / 1000.0;
  float voltage = (analogRead(SENSOR_PIN) / 1024.0) * refVolt;
  float db = voltage * SENSITIVITY;

  // Clamp to sensor's valid range
  db = constrain(db, 30.0, 130.0);

  // Store in circular buffer
  samples[sampleIndex] = db;
  sampleIndex = (sampleIndex + 1) % NUM_SAMPLES;
  if (sampleIndex == 0) bufferFull = true;

  // Compute running average
  float sum = 0;
  int count = bufferFull ? NUM_SAMPLES : sampleIndex;
  for (int i = 0; i < count; i++) {
    sum += samples[i];
  }
  return sum / (float)count;
}

// =============================================
// Sound Level Descriptor
// =============================================
const char* getLevelDescription(float db) {
  if (db < 40)  return "Quiet";
  if (db < 55)  return "Moderate";
  if (db < 70)  return "Loud";
  if (db < 85)  return "Very Loud";
  if (db < 100) return "Dangerous";
  return "EXTREME!";
}

// =============================================
// Draw Visual Bar Graph
// =============================================
void drawBarGraph(float db) {
  int barWidth = map(constrain((int)db, 30, 130), 30, 130, 0, 120);

  // Outline
  display.drawRect(3, 42, 122, 10, SSD1306_WHITE);

  // Filled portion
  if (barWidth > 0) {
    display.fillRect(4, 43, barWidth, 8, SSD1306_WHITE);
  }

  // Scale labels
  display.setTextSize(1);
  display.setCursor(0, 54);
  display.print(F("30"));
  display.setCursor(55, 54);
  display.print(F("80"));
  display.setCursor(108, 54);
  display.print(F("130"));
}

// =============================================
// Update OLED Display
// =============================================
void updateDisplay(float db) {
  display.clearDisplay();

  // ---- Header Row ----
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 3);
  display.print(F("dBA METER"));

  // Level descriptor (right-aligned)
  const char* desc = getLevelDescription(db);
  int descWidth = strlen(desc) * 6;
 // display.setCursor(SCREEN_WIDTH - descWidth, 0);
 // display.print(desc);

  // Separator
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // ---- Main Reading (Large) ----
  display.setTextSize(2);
  display.setCursor(40, 23);
  if (db < 100.0) {
    display.print(db, 1);
  } else {
    display.print(db, 0);
  }

  // Unit
  display.setTextSize(1);

  // Min/Max (small, below main reading)
  display.setCursor(100, 23);
  display.print(F("H:"));
  display.print((int)maxDb);
  display.setCursor(100, 32);
  display.print(F("L:"));
  display.print((int)minDb);

  // ---- Bar Graph ----
  drawBarGraph(db);

  display.display();
}

// =============================================
// Setup
// =============================================
void setup() {
  Serial.begin(115200);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 OLED not found!"));
    for (;;);
  }

  // Splash screen
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(45, 3);
  display.println(F("dBA"));
  display.setCursor(35, 25);
  display.println(F("METER"));
  display.setTextSize(1);
  display.setCursor(20, 50);
  display.print(F("SEN0232 + SSD1306"));
  display.display();
  delay(2000);

  // Initialize buffer
  for (int i = 0; i < NUM_SAMPLES; i++) {
    samples[i] = 0;
  }

  // Serial header
  Serial.println(F("========================="));
  Serial.println(F(" Handheld dBA Meter"));
  Serial.println(F(" SEN0232 + SSD1306 OLED"));
  Serial.println(F("========================="));
  Serial.println();
  Serial.println(F("dBA\tMin\tMax\tVcc(mV)\tLevel"));
  Serial.println(F("---\t---\t---\t-------\t-----"));
}

// =============================================
// Main Loop
// =============================================
void loop() {
  unsigned long now = millis();

  if (now - lastSampleTime >= SAMPLE_DELAY) {
    lastSampleTime = now;

    float db = readSoundLevel();

    // Track min/max (ignore startup readings)
    if (bufferFull || sampleIndex > 3) {
      if (db > maxDb) maxDb = db;
      if (db < minDb) minDb = db;
    }

    // Update OLED
    updateDisplay(db);

    // Serial output
    Serial.print(db, 1);
    Serial.print(F("\t"));
    Serial.print(minDb, 1);
    Serial.print(F("\t"));
    Serial.print(maxDb, 1);
    Serial.print(F("\t"));
    Serial.print(readVcc());
    Serial.print(F("\t"));
    Serial.println(getLevelDescription(db));
  }
}
