/*
 * =========================================================================
 * 🦅 PROJECT E.C.H.O. | NodeMCU Final Bulletproof Firmware (v6.2)
 * =========================================================================
 * 🛡️ Hardware: NodeMCU ESP8266 (v3)
 * 🔬 AI Engine: ArduTFLite + Chirale_TensorFlowLite (ESP8266 Compatible)
 * 💻 Interface: Structured JSON + Serial Simulation
 * =========================================================================
 * Updates in v6.2:
 * - SOS loop now runs infinitely on ENEMY without freezing.
 * - FRIENDLY mode correctly pulses both Yellow pair and Red/Blue pair.
 * - Added non-blocking paste-buffer for 5-second serial tests.
 * =========================================================================
 */

#include "echo_model.h"
#include <Chirale_TensorFlowLite.h>
#include <math.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/schema/schema_generated.h>

// Model Configuration
#define NUMBER_OF_INPUTS 192
#define NUMBER_OF_OUTPUTS 3
#define TENSOR_ARENA_SIZE (12 * 1024)

// HARDWARE PINOUT
const int RADAR_ANALOG_PIN = A0;
const int BUZZER_PIN = 14;      // D5
const int YELLOW_LED_1_PIN = 5; // D1
const int YELLOW_LED_2_PIN = 4; // D2
const int RED_LED_PIN = 12;     // D6
const int BLUE_LED_PIN = 13;    // D7
const float ENEMY_CONF_THRESHOLD = 90.0f;
const float ACTIVITY_SPAN_MIN = 0.20f;  // hard minimum for (max-min)
const float ACTIVITY_DIFF_MIN = 0.015f; // hard minimum for avg |x[i]-x[i-1]|
const int ENEMY_STREAK_REQUIRED = 3; // require N consecutive enemy predictions

// Tensor Arena pointer (allocated at runtime to keep .bss small on ESP8266)
byte *tensorArena = nullptr;

const char *labels[] = {"Bird", "Drone", "Human"};
int enemyStreak = 0;
float baselineSpan = 0.0f;
float baselineDiff = 0.0f;

enum AlertMode { MODE_IDLE = 0, MODE_FRIENDLY = 1, MODE_ENEMY = 2 };
AlertMode currentMode = MODE_IDLE;
bool isAlertLatched = false;

unsigned long patternLastMs = 0;
bool patternPhase = false;
unsigned long lastTestTime = 0; // Gives user time to paste next test string

// Custom SOS array from user
const int MORSE_STEPS = 18;
const unsigned int morseDurations[MORSE_STEPS] = {
    500, 100,  500, 100, 500, 500, 1000, 100, 1000,
    100, 1000, 500, 500, 100, 500, 100,  500, 2000};
const bool morseBuzzOn[MORSE_STEPS] = {true, false, true, false, true, false,
                                       true, false, true, false, true, false,
                                       true, false, true, false, true, false};
int morseStep = 0;
unsigned long morseStepStartMs = 0;

// Lightweight local TFLite wrapper
namespace {
const tflite::Model *gModel = nullptr;
tflite::MicroInterpreter *gInterpreter = nullptr;
TfLiteTensor *gInputTensor = nullptr;
TfLiteTensor *gOutputTensor = nullptr;
tflite::MicroMutableOpResolver<6> gOpResolver;
bool gOpsAdded = false;

void addRequiredOps() {
  if (gOpsAdded)
    return;
  gOpResolver.AddFullyConnected();
  gOpResolver.AddSoftmax();
  gOpResolver.AddReshape();
  gOpResolver.AddQuantize();
  gOpResolver.AddDequantize();
  gOpsAdded = true;
}
} // namespace

bool modelInit(const unsigned char *model, byte *arena, int arenaSize) {
  addRequiredOps();
  gModel = tflite::GetModel(model);
  if (gModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema version mismatch!");
    return false;
  }

  gInterpreter =
      new tflite::MicroInterpreter(gModel, gOpResolver, arena, arenaSize);
  if (!gInterpreter || gInterpreter->AllocateTensors() != kTfLiteOk) {
    return false;
  }

  gInputTensor = gInterpreter->input(0);
  gOutputTensor = gInterpreter->output(0);
  return gInputTensor != nullptr && gOutputTensor != nullptr;
}

bool modelSetInput(float inputValue, int index) {
  if (!gInputTensor || index < 0) {
    return false;
  }

  if (gInputTensor->type == kTfLiteFloat32) {
    if (index >= (gInputTensor->bytes / (int)sizeof(float)))
      return false;
    gInputTensor->data.f[index] = inputValue;
    return true;
  }

  if (gInputTensor->type == kTfLiteInt8) {
    if (index >= gInputTensor->bytes)
      return false;
    const float scale = gInputTensor->params.scale;
    const int zero_point = gInputTensor->params.zero_point;
    int q = (int)roundf(inputValue / scale) + zero_point;
    if (q < -128)
      q = -128;
    if (q > 127)
      q = 127;
    gInputTensor->data.int8[index] = (int8_t)q;
    return true;
  }

  return false;
}

bool modelRunInference() {
  return gInterpreter && gInterpreter->Invoke() == kTfLiteOk;
}

float modelGetOutput(int index) {
  if (!gOutputTensor || index < 0) {
    return -1.0f;
  }

  if (gOutputTensor->type == kTfLiteFloat32) {
    if (index >= (gOutputTensor->bytes / (int)sizeof(float)))
      return -1.0f;
    return gOutputTensor->data.f[index];
  }

  if (gOutputTensor->type == kTfLiteInt8) {
    if (index >= gOutputTensor->bytes)
      return -1.0f;
    const float scale = gOutputTensor->params.scale;
    const int zero_point = gOutputTensor->params.zero_point;
    const int q = (int)gOutputTensor->data.int8[index];
    return (float)(q - zero_point) * scale;
  }

  return -1.0f;
}

void resetMorse() {
  morseStep = 0;
  morseStepStartMs = millis();
  digitalWrite(BUZZER_PIN, LOW);
}

void updateMorseSOS() {
  const unsigned long now = millis();
  if (now - morseStepStartMs >= morseDurations[morseStep]) {
    morseStep = (morseStep + 1) % MORSE_STEPS;
    morseStepStartMs = now;
  }
  digitalWrite(BUZZER_PIN, morseBuzzOn[morseStep] ? HIGH : LOW);
}

void setAllLeds(bool y1, bool y2, bool r, bool b) {
  digitalWrite(YELLOW_LED_1_PIN, y1 ? HIGH : LOW);
  digitalWrite(YELLOW_LED_2_PIN, y2 ? HIGH : LOW);
  digitalWrite(RED_LED_PIN, r ? HIGH : LOW);
  digitalWrite(BLUE_LED_PIN, b ? HIGH : LOW);
}

void runActiveIndicators() {
  const unsigned long now = millis();
  
  if (isAlertLatched || currentMode == MODE_ENEMY) {
    if (now - patternLastMs >= 200) {
      patternLastMs = now;
      patternPhase = !patternPhase;
    }
    // MODE_ENEMY: yellow off, red/blue alternate, SOS buzzer.
    setAllLeds(false, false, patternPhase, !patternPhase);
    updateMorseSOS();
    return;
  }

  if (currentMode == MODE_IDLE) {
    if (now - patternLastMs >= 500) { // Slow blink for idle
      patternLastMs = now;
      patternPhase = !patternPhase;
    }
    // IDLE: Yellow LEDs alternate slowly, Red/Blue OFF
    setAllLeds(patternPhase, !patternPhase, false, false);
    resetMorse();
    return;
  }

  if (currentMode == MODE_FRIENDLY) {
    if (now - patternLastMs >= 200) { // Fast blink for friendly
      patternLastMs = now;
      patternPhase = !patternPhase;
    }
    // FRIENDLY: Both yellow pair and red/blue pair alternate as requested.
    setAllLeds(patternPhase, !patternPhase, !patternPhase, patternPhase);
    resetMorse();
    return;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50); // Important: Prevents Serial.read from freezing the loop
  
  pinMode(RADAR_ANALOG_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(YELLOW_LED_1_PIN, OUTPUT);
  pinMode(YELLOW_LED_2_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  setAllLeds(false, false, false, false);
  digitalWrite(BUZZER_PIN, LOW); // OFF

  Serial.printf("\nFree heap before arena alloc: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Requesting tensor arena: %u bytes\n", (unsigned)TENSOR_ARENA_SIZE);

  tensorArena = (byte *)malloc(TENSOR_ARENA_SIZE);
  if (!tensorArena) {
    Serial.println("ERROR: Tensor arena allocation failed.");
    while (1) delay(100);
  }

  // Initialize the AI engine
  if (!modelInit(echo_model_tflite, tensorArena, TENSOR_ARENA_SIZE)) {
    Serial.println("ERROR: AI Brain Initialization Failed.");
    while (1) delay(100);
  }

  // Startup background calibration
  float spanAccum = 0.0f;
  float diffAccum = 0.0f;
  const int calibFrames = 20;
  for (int f = 0; f < calibFrames; f++) {
    float cMin = 99999.0f;
    float cMax = -99999.0f;
    float prev = 0.0f;
    bool hasPrev = false;
    float dSum = 0.0f;

    for (int i = 0; i < NUMBER_OF_INPUTS; i++) {
      const float s = (float)analogRead(RADAR_ANALOG_PIN) / 143.212067f;
      if (s < cMin) cMin = s;
      if (s > cMax) cMax = s;
      if (hasPrev) dSum += fabsf(s - prev);
      prev = s;
      hasPrev = true;
      delayMicroseconds(50);
    }
    spanAccum += (cMax - cMin);
    diffAccum += dSum / (float)(NUMBER_OF_INPUTS - 1);
  }
  baselineSpan = spanAccum / (float)calibFrames;
  baselineDiff = diffAccum / (float)calibFrames;
  Serial.printf("Baseline calibrated: span=%.4f diff=%.4f\n", baselineSpan, baselineDiff);

  Serial.println("\nPROJECT E.C.H.O. V6.2 ONLINE");
  Serial.println("ENEMY DETECTION SYSTEM READY. Type 'CLEAR' to clear alarms.");
}

void loop() {
  runActiveIndicators();

  bool testDataReceived = false;
  float sample = 0.0f;
  float frameMin = 99999.0f;
  float frameMax = -99999.0f;
  float frameSum = 0.0f;
  float frameDiffSum = 0.0f;
  float prevSample = 0.0f;
  bool hasPrevSample = false;

  // Non-blocking serial parsing
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.equalsIgnoreCase("CLEAR") || input.equalsIgnoreCase("ACK")) {
      isAlertLatched = false;
      enemyStreak = 0;
      currentMode = MODE_IDLE;
      Serial.println("\n*** ALARM CLEARED *** System restored to IDLE state.");
      return; 
    } 
    else if (input.indexOf(',') > 0) {
      Serial.println("Data Packet Found. Processing Simulation...");
      
      int startIdx = 0;
      int nextCom;
      for (int i = 0; i < NUMBER_OF_INPUTS; i++) {
        nextCom = input.indexOf(',', startIdx);
        String valStr = (nextCom == -1) ? input.substring(startIdx) : input.substring(startIdx, nextCom);
        sample = valStr.toFloat();
        
        if (sample < frameMin) frameMin = sample;
        if (sample > frameMax) frameMax = sample;
        frameSum += sample;
        if (hasPrevSample) frameDiffSum += fabsf(sample - prevSample);
        prevSample = sample;
        hasPrevSample = true;
        
        modelSetInput(sample, i);
        startIdx = nextCom + 1;
      }
      testDataReceived = true;
      lastTestTime = millis(); // <--- Update the paste timer!
    } else {
      return; 
    }
  }

  if (isAlertLatched) {
    delay(10); 
    return;
  }

  // GIVE THE USER A 5-SECOND WINDOW TO PASTE THE NEXT STRING WITHOUT RESETTING THE STREAK
  if (!testDataReceived && (millis() - lastTestTime < 5000)) {
    delay(10); // Keep LED rendering smooth
    return;
  }

  // Live Radar Sampling
  if (!testDataReceived) {
    for (int i = 0; i < NUMBER_OF_INPUTS; i++) {
      sample = (float)analogRead(RADAR_ANALOG_PIN) / 143.212067f;
      if (sample < frameMin) frameMin = sample;
      if (sample > frameMax) frameMax = sample;
      frameSum += sample;
      if (hasPrevSample) frameDiffSum += fabsf(sample - prevSample);
      prevSample = sample;
      hasPrevSample = true;
      
      if (!modelSetInput(sample, i)) {
        Serial.println("ERROR: Failed to set input.");
        delay(100);
        return;
      }
      delayMicroseconds(50);
    }
  }

  const float frameSpan = frameMax - frameMin;
  const float frameDiff = frameDiffSum / (float)(NUMBER_OF_INPUTS - 1);
  const float spanGate = fmaxf(ACTIVITY_SPAN_MIN, baselineSpan * 2.5f);
  const float diffGate = fmaxf(ACTIVITY_DIFF_MIN, baselineDiff * 2.5f);

  if (!testDataReceived && frameSpan < spanGate && frameDiff < diffGate) {
    enemyStreak = 0;
    currentMode = MODE_IDLE;
    Serial.printf("{\"node\":\"ECHO-01\",\"type\":\"None\",\"conf\":0.00,\"status\":\"NORMAL\",\"span\":%.3f,\"diff\":%.4f}\n", frameSpan, frameDiff);
    delay(50);
    return;
  }

  if (!modelRunInference()) {
    Serial.println("Classification Error.");
    delay(100);
    return;
  }

  int prediction = 0;
  float maxProb = -1.0;
  float outputs[NUMBER_OF_OUTPUTS];

  for (int i = 0; i < NUMBER_OF_OUTPUTS; i++) {
    outputs[i] = modelGetOutput(i);
    if (outputs[i] > maxProb) {
      maxProb = outputs[i];
      prediction = i;
    }
  }

  const float confidence = maxProb * 100.0f;
  const bool isDrone = (prediction == 1);
  const bool rawEnemy = isDrone && (confidence >= ENEMY_CONF_THRESHOLD);
  
  if (rawEnemy) {
    enemyStreak++;
  } else {
    enemyStreak = 0;
  }
  
  const bool isEnemy = enemyStreak >= ENEMY_STREAK_REQUIRED;
  const bool isFriendlyDrone = isDrone && !isEnemy;

  if (isEnemy) {
    currentMode = MODE_ENEMY;
    isAlertLatched = true;
  } else if (isFriendlyDrone) {
    currentMode = MODE_FRIENDLY;
  } else {
    currentMode = MODE_IDLE;
  }

  Serial.printf(
      "{\"node\":\"ECHO-01\",\"type\":\"%s\",\"conf\":%.2f,\"status\":\"%s\",\"span\":%.3f,\"diff\":%.4f,\"streak\":%d}\n",
      labels[prediction], confidence, isEnemy ? "ENEMY" : "NORMAL", frameSpan, frameDiff, enemyStreak);
}
