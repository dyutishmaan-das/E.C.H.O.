/*
 * =========================================================================
 * 🦅 PROJECT E.C.H.O. | Acoustic Secondary Validator - Gated Logic (v7.0)
 * =========================================================================
 * 🛡️ Hardware: STM32 Blue Pill (STM32F103C8T6)
 * 🔬 Function: Secondary Drone Confirmation (Slave Node)
 * 📡 Power: Runs ONLY when Radar Master (ESP8266) pulls Trigger HIGH.
 * =========================================================================
 */

// BLUE PILL PINOUT
const int MIC_ANALOG_PIN = PA0;   // Mic Preamplifier Input (ADC1_IN0)
const int RADAR_TRIGGER_PIN = PB1; // Wake-up Input (from NodeMCU D8)
const int BUZZER_PIN = PA5;      
const int YELLOW_LED_1_PIN = PA1;
const int YELLOW_LED_2_PIN = PA2;
const int RED_LED_PIN = PA3;
const int BLUE_LED_PIN = PA4;
const int ONBOARD_LED = PC13;   

// Dynamic Sampling Config
const int BUFFER_SIZE = 500;
int samples[BUFFER_SIZE];

void setup() {
  Serial.begin(115200);
  delay(1500); // USB CDC Safety Delay for STM32
  
  Serial.println("\n[SYSTEM] E.C.H.O. ACOUSTIC GATED NODE ONLINE");
  Serial.println("[MODE] Secondary Validator (Slave)");
  Serial.println("[PIN] Watching Radar Trigger on PB1...");
  
  analogReadResolution(12); // Boost to 12-bit (0-4095)
  
  pinMode(RADAR_TRIGGER_PIN, INPUT_PULLDOWN); // Ensure idle if wire disconnected
  pinMode(MIC_ANALOG_PIN, INPUT_ANALOG);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(YELLOW_LED_1_PIN, OUTPUT);
  pinMode(YELLOW_LED_2_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT);

  // Initial Hardware Blink
  setAllLeds(true, true, true, true);
  delay(500);
  setAllLeds(false, false, false, false);
}

void setAllLeds(bool y1, bool y2, bool r, bool b) {
  digitalWrite(YELLOW_LED_1_PIN, y1 ? HIGH : LOW);
  digitalWrite(YELLOW_LED_2_PIN, y2 ? HIGH : LOW);
  digitalWrite(RED_LED_PIN, r ? HIGH : LOW);
  digitalWrite(BLUE_LED_PIN, b ? HIGH : LOW);
}

void loop() {
  // --- MASTER CONTROL CHECK ---
  bool isActivated = digitalRead(RADAR_TRIGGER_PIN);
  
  if (!isActivated) {
    // IDLE MODE: Low frequency standby
    digitalWrite(ONBOARD_LED, HIGH); // OFF
    digitalWrite(YELLOW_LED_1_PIN, (millis() % 2000 < 100) ? HIGH : LOW); // Heartbeat
    digitalWrite(YELLOW_LED_2_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);
    delay(50); // Loop quickly but do nothing intensive
    return; 
  }

  // --- WAKE UP MODE: Radar has identified a suspect target ---
  digitalWrite(ONBOARD_LED, LOW); // ON (Activity indicator)
  digitalWrite(YELLOW_LED_2_PIN, HIGH);
  
  int rawMin = 4096;
  int rawMax = -1;
  long sum = 0;
  
  // High-speed acoustic capture for confirmation
  for (int i = 0; i < BUFFER_SIZE; i++) {
    int val = analogRead(MIC_ANALOG_PIN);
    if (val < rawMin) rawMin = val;
    if (val > rawMax) rawMax = val;
    sum += val;
    delayMicroseconds(100); 
  }
  
  float avgRaw = (float)sum / BUFFER_SIZE;
  int peakToPeak = rawMax - rawMin;
  
  // DRONE ACOUSTIC MATCHING LOGIC (Simulated with Frequency/Swing)
  // Replace this block with model.runInference() when model.h is ready for MCU
  bool isConfirmed = (peakToPeak > 350); // Significant acoustic signature detected

  if (isConfirmed) {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(BLUE_LED_PIN, HIGH);
    Serial.println(">>> ACOUSTIC CONFIRMATION: Target Signature Matches Drone.");
  } else {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);
    Serial.println(">>> ACOUSTIC REJECTION: No drone noise profile detected.");
  }

  // Structured Reporting for Telemetry
  Serial.printf("{\"node\":\"ECHO-ACOUSTIC\",\"trigger\":%s,\"swing\":%d,\"confirmed\":%s}\n", 
                isActivated ? "ON" : "OFF", peakToPeak, isConfirmed ? "TRUE" : "FALSE");

  delay(20);
}
