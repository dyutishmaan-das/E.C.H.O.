/*
 * =========================================================================
 * 🦅 PROJECT E.C.H.O. | Acoustic Diagnostic Firmware (v1.1)
 * =========================================================================
 * 🛡️ Hardware: STM32 Blue Pill (STM32F103C8T6)
 * 🔬 Function: Standalone Mic/Preamp validator on STM32
 * 💻 Logic: 12-bit ADC High-Speed Pulse Check
 * =========================================================================
 */

// BLUE PILL PINOUT (Standard Mapping)
const int MIC_ANALOG_PIN = PA0; // High Speed ADC1_IN0
const int BUZZER_PIN = PA5;     // PWM Capable pin
const int YELLOW_LED_1_PIN = PA1;
const int YELLOW_LED_2_PIN = PA2;
const int RED_LED_PIN = PA3;
const int BLUE_LED_PIN = PA4;
const int ONBOARD_LED = PC13; // Active LOW for Blue Pill

// Diagnostic Config
const int BUFFER_SIZE = 500;
int samples[BUFFER_SIZE];

void setup() {
  Serial.begin(115200);
  delay(1500); // Required for USB CDC to stabilize on STM32

  Serial.println(
      "\n[SYSTEM] PROJECT E.C.H.O. / STM32-BLUEPILL DIAGNOSTIC READY");

  // Set ADC Resolution to 12-bit (0-4095) for professional analysis
  analogReadResolution(12);

  pinMode(MIC_ANALOG_PIN, INPUT_ANALOG);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(YELLOW_LED_1_PIN, OUTPUT);
  pinMode(YELLOW_LED_2_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT);

  // Quick Hardware Self-Test
  digitalWrite(ONBOARD_LED, LOW); // ON
  digitalWrite(YELLOW_LED_1_PIN, HIGH);
  delay(100);
  digitalWrite(YELLOW_LED_2_PIN, HIGH);
  delay(100);
  digitalWrite(RED_LED_PIN, HIGH);
  delay(100);
  digitalWrite(BLUE_LED_PIN, HIGH);
  delay(500);

  digitalWrite(YELLOW_LED_1_PIN, LOW);
  digitalWrite(YELLOW_LED_2_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(BLUE_LED_PIN, LOW);
  digitalWrite(ONBOARD_LED, HIGH); // OFF

  Serial.println("Monitoring Signal at PA0 (ADC12_IN0)...");
  Serial.println("Resolution: 12-bit (0 to 4095)");
}

void loop() {
  int rawMin = 4096;
  int rawMax = -1;
  long sum = 0;

  // Capture high-speed samples
  for (int i = 0; i < BUFFER_SIZE; i++) {
    int val = analogRead(MIC_ANALOG_PIN);
    samples[i] = val;

    if (val < rawMin)
      rawMin = val;
    if (val > rawMax)
      rawMax = val;
    sum += val;

    // Serial Plotter output for waveform viewing
    // Serial.println(val);

    delayMicroseconds(100); // Fast capture loop
  }

  float avgRaw = (float)sum / BUFFER_SIZE;
  int peakToPeak = rawMax - rawMin;

  // Visual feedback logic (Scaled for 12-bit range 0-4095)
  if (peakToPeak > 1200) {
    // Very loud - Reds/Blues
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(BLUE_LED_PIN, HIGH);
    digitalWrite(YELLOW_LED_1_PIN, LOW);
    digitalWrite(YELLOW_LED_2_PIN, LOW);
    digitalWrite(ONBOARD_LED, LOW); // Flicker onboard for clip warning
  } else if (peakToPeak > 200) {
    // Moderate - Yellows
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_1_PIN, HIGH);
    digitalWrite(YELLOW_LED_2_PIN, HIGH);
    digitalWrite(ONBOARD_LED, HIGH);
  } else {
    // Quiet / Offset Check
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_1_PIN, (millis() % 2000 < 100) ? HIGH : LOW);
    digitalWrite(YELLOW_LED_2_PIN, LOW);
    digitalWrite(ONBOARD_LED, HIGH);
  }

  // JSON Signal Report (Formatted for E.C.H.O. Serial Processor)
  Serial.print("{\"node\":\"ECHO-STM32-DIAG\",\"min\":");
  Serial.print(rawMin);
  Serial.print(",\"max\":");
  Serial.print(rawMax);
  Serial.print(",\"swing\":");
  Serial.print(peakToPeak);
  Serial.print(",\"dc\":");
  Serial.print(avgRaw);
  Serial.println("}");

  // Tone Warning if signal is too intense (Clipping Protection)
  if (peakToPeak > 3800) {
    tone(BUZZER_PIN, 2000, 10);
    Serial.println("ALERT: ADC CLIPPING! Reduce gain on preamp.");
  }

  delay(20);
}
