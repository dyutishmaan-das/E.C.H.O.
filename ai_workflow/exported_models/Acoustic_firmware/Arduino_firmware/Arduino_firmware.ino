/*
 * =========================================================================
 * 🦅 PROJECT E.C.H.O. | Acoustic Slave Validator - TinyML (v7.5)
 * =========================================================================
 * 🛡️ Hardware: STM32 Blue Pill (STM32F103C8T6)
 * 🔬 Engine: ArduTFLite / Chirale_TensorFlowLite (Quantized Int8)
 * 📡 Control: Wakes up on HIGH signal from Radar Master (PB1)
 * =========================================================================
 */

#include "echo_acoustic_model.h"
#include <Chirale_TensorFlowLite.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/schema/schema_generated.h>

// BLUE PILL PINOUT
const int MIC_ANALOG_PIN = PA0;   
const int RADAR_TRIGGER_PIN = PB1; 
const int YELLOW_LED_1_PIN = PA1;
const int RED_LED_PIN = PA3;
const int BLUE_LED_PIN = PA4;
const int ONBOARD_LED = PC13;   

// TinyML Model Parameters (Matching Optimized Notebook)
const int NUMBER_OF_MELS = 32;
const int NUMBER_OF_FRAMES = 32;
const int TENSOR_ARENA_SIZE = (10 * 1024); // 10KB Arena

byte *tensorArena = nullptr;

namespace {
const tflite::Model *gModel = nullptr;
tflite::MicroInterpreter *gInterpreter = nullptr;
TfLiteTensor *gInputTensor = nullptr;
TfLiteTensor *gOutputTensor = nullptr;
tflite::MicroMutableOpResolver<7> gOpResolver;

void setupTFLite() {
  gOpResolver.AddConv2D();
  gOpResolver.AddMaxPool2D();
  gOpResolver.AddAveragePool2D();
  gOpResolver.AddFullyConnected();
  gOpResolver.AddSoftmax();
  gOpResolver.AddQuantize();
  gOpResolver.AddDequantize();
}
}

bool initModel() {
  setupTFLite();
  gModel = tflite::GetModel(echo_acoustic_model_tflite);
  if (gModel->version() != TFLITE_SCHEMA_VERSION) return false;
  gInterpreter = new tflite::MicroInterpreter(gModel, gOpResolver, tensorArena, TENSOR_ARENA_SIZE);
  if (gInterpreter->AllocateTensors() != kTfLiteOk) return false;
  gInputTensor = gInterpreter->input(0);
  gOutputTensor = gInterpreter->output(0);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1500); 
  
  pinMode(RADAR_TRIGGER_PIN, INPUT_PULLDOWN);
  pinMode(MIC_ANALOG_PIN, INPUT_ANALOG);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT);
  
  analogReadResolution(12);

  tensorArena = (byte *)malloc(TENSOR_ARENA_SIZE);
  if (!initModel()) {
    while (1) { Serial.println("TFLITE FAIL"); delay(1000); }
  }
  
  Serial.println("\n[SYSTEM] E.C.H.O. ACOUSTIC SLAVE V7.5 (TINYML) READY");
}

void loop() {
  if (digitalRead(RADAR_TRIGGER_PIN) == LOW) {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);
    delay(50);
    return; 
  }

  // Analyzing...
  Serial.println("[WAKE] Analyzing Sound Spectrum...");
  
  // High-speed capture of 32x32 spectrum (Simulated extraction for MCU)
  // In a real scenario, you'd use an FFT library here.
  for (int i = 0; i < (NUMBER_OF_MELS * NUMBER_OF_FRAMES); i++) {
    int raw = analogRead(MIC_ANALOG_PIN);
    float normalized = (float)raw / 4095.0f; // 0.0 to 1.0
    
    // Convert to INT8 Quantized Input
    const float scale = gInputTensor->params.scale;
    const int zp = gInputTensor->params.zero_point;
    int8_t quantized = (int8_t)(normalized / scale + zp);
    gInputTensor->data.int8[i] = quantized;
    
    delayMicroseconds(50);
  }

  if (gInterpreter->Invoke() == kTfLiteOk) {
    float droneScore = (float)(gOutputTensor->data.int8[0] - gOutputTensor->params.zero_point) * gOutputTensor->params.scale;
    
    bool isConfirmed = (droneScore > 0.7f); // 70% Confidence

    if (isConfirmed) {
      digitalWrite(RED_LED_PIN, HIGH);
      digitalWrite(BLUE_LED_PIN, HIGH);
      Serial.println(">>> [CONFIRMED] Acoustic Drone Signature Detected.");
    } else {
      digitalWrite(RED_LED_PIN, LOW);
      digitalWrite(BLUE_LED_PIN, LOW);
      Serial.println(">>> [REJECTED] Signal does not match drone profile.");
    }
  }

  delay(10);
}
