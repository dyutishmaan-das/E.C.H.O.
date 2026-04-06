/*
 * =========================================================================
 * 🦅 PROJECT E.C.H.O. | DEBUG MODE - Acoustic Validator (v9.7)
 * =========================================================================
 */

#include "echo_acoustic_model.h"
#include <Chirale_TensorFlowLite.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/schema/schema_generated.h>

// PINOUT (Using PA1 for better reliability)
const int MIC_ANALOG_PIN = PA0;   
const int RADAR_TRIGGER_PIN = PA1; 
const int RED_LED_PIN = PA3;
const int BLUE_LED_PIN = PA4;
const int ONBOARD_LED = PC13;   

// Memory configuration
const int NUMBER_OF_MELS = 24;
const int NUMBER_OF_FRAMES = 24;
const int TENSOR_ARENA_SIZE = (12 * 1024); // Micro-bump to 12KB

byte tensorArena[TENSOR_ARENA_SIZE];

namespace {
  const tflite::Model* gModel = nullptr;
  tflite::MicroInterpreter* gInterpreter = nullptr;
  TfLiteTensor* gInputTensor = nullptr;
  TfLiteTensor* gOutputTensor = nullptr;
  
  // STRIPPED OP RESOLVER (Now including MEAN and DENSE)
  tflite::MicroMutableOpResolver<10> gOpResolver;
}

void setup() {
  Serial.begin(115200);
  delay(1000); 
  Serial.println("\n\n--- EBHO INITIALIZING (v9.7) ---");

  pinMode(RADAR_TRIGGER_PIN, INPUT_PULLDOWN);
  pinMode(MIC_ANALOG_PIN, INPUT_ANALOG);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT);
  analogReadResolution(12);

  // Corrected OP registrations
  gOpResolver.AddConv2D();
  gOpResolver.AddMaxPool2D();
  gOpResolver.AddReshape();
  gOpResolver.AddFullyConnected(); // Required for Dense layer
  gOpResolver.AddMean();           // Required for GlobalAveragePooling
  gOpResolver.AddSoftmax();
  gOpResolver.AddQuantize();
  gOpResolver.AddDequantize();

  Serial.println("[AI] Loading TinyModel...");
  gModel = tflite::GetModel(echo_acoustic_model_tflite);
  
  static tflite::MicroInterpreter static_interpreter(gModel, gOpResolver, tensorArena, TENSOR_ARENA_SIZE);
  gInterpreter = &static_interpreter;
  
  if (gInterpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("[FATAL] Memory Allocation Failed!");
    while(1) { digitalWrite(ONBOARD_LED, !digitalRead(ONBOARD_LED)); delay(100); }
  }
  
  gInputTensor = gInterpreter->input(0);
  gOutputTensor = gInterpreter->output(0);
  
  Serial.println("[READY] ALL SYSTEMS NOMINAL! Touch 3.3V to PA1...");
}

void loop() {
  // Pulse LED slower to show standby
  digitalWrite(ONBOARD_LED, (millis() % 2000 < 50) ? LOW : HIGH);

  if (digitalRead(RADAR_TRIGGER_PIN) == LOW) {
    return; 
  }

  // --- TRIGGERED ---
  Serial.println("\n>>> [WAKEUP] Trigger Signal detected on PA1!");
  
  long rawMin = 4095;
  long rawMax = 0;

  Serial.print("Listening for signatures: ");
  for (int i = 0; i < (NUMBER_OF_MELS * NUMBER_OF_FRAMES); i++) {
    int raw = analogRead(MIC_ANALOG_PIN);
    if(raw < rawMin) rawMin = raw;
    if(raw > rawMax) rawMax = raw;

    float normalized = (float)raw / 4095.0f;
    const float scale = gInputTensor->params.scale;
    const int zp = gInputTensor->params.zero_point;
    gInputTensor->data.int8[i] = (int8_t)(normalized / scale + zp);
    
    if (i % 80 == 0) Serial.print(".");
    delayMicroseconds(50);
  }
  Serial.println(" Done.");

  Serial.println("Running AI Calculation...");
  if (gInterpreter->Invoke() == kTfLiteOk) {
    float score = (float)(gOutputTensor->data.int8[0] - gOutputTensor->params.zero_point) * gOutputTensor->params.scale;
    
    Serial.print("--- [STATUS] ---");
    Serial.print("\nDrone Certainty: "); Serial.print(score * 100); Serial.println("%");
    Serial.print("Mic Volume (Swing): "); Serial.println(rawMax - rawMin);

    if (score > 0.60f) {
      Serial.println("RESULT: POSITIVE (Drone Alert)");
      digitalWrite(RED_LED_PIN, HIGH);
      digitalWrite(BLUE_LED_PIN, HIGH);
      delay(1500);
    } else {
      Serial.println("RESULT: NEGATIVE (Ambient Noise)");
    }
  } else {
    Serial.println("[ERROR] Inference calculation failed!");
  }
  
  Serial.println("Relapsing to sleep...\n");
  delay(500);
}
