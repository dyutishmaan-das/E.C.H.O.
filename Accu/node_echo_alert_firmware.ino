/*
 * =========================================================================
 * 🦅 PROJECT E.C.H.O. | NodeMCU Final Bulletproof Firmware (v6.0)
 * =========================================================================
 * 🛡️ Hardware: NodeMCU ESP8266 (v3)
 * 🔬 AI Engine: ArduTFLite + Chirale_TensorFlowLite (ESP8266 Compatible)
 * 💻 Interface: Structured JSON + Serial Simulation
 * =========================================================================
 * This version uses ArduTFLite (a thin wrapper over Chirale_TensorFlowLite)
 * which provides direct TFLite Micro support for ESP8266.
 * EloquentTinyML is NOT used — it requires tflm_esp32 (ESP32 only).
 * =========================================================================
 */

#include <Chirale_TensorFlowLite.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include "echo_model.h"

// Model Configuration
#define NUMBER_OF_INPUTS 1280
#define NUMBER_OF_OUTPUTS 3
#define TENSOR_ARENA_SIZE (96 * 1024)

// HARDWARE PINOUT
const int RADAR_ANALOG_PIN = A0;
const int BUZZER_PIN = 14;     // D5
const int ALERT_LED_PIN = 2;   // D4

// Tensor Arena pointer (allocated at runtime to keep .bss small on ESP8266)
byte* tensorArena = nullptr;

const char* labels[] = {"Bird", "Drone", "Human"};

// Lightweight local TFLite wrapper to avoid AllOpsResolver RAM overhead on ESP8266.
namespace {
    const tflite::Model* gModel = nullptr;
    tflite::MicroInterpreter* gInterpreter = nullptr;
    TfLiteTensor* gInputTensor = nullptr;
    TfLiteTensor* gOutputTensor = nullptr;
    tflite::MicroMutableOpResolver<12> gOpResolver;
    bool gOpsAdded = false;

    void addRequiredOps() {
        if (gOpsAdded) return;
        gOpResolver.AddConv2D();
        gOpResolver.AddMaxPool2D();
        gOpResolver.AddFullyConnected();
        gOpResolver.AddSoftmax();
        gOpResolver.AddReshape();
        gOpResolver.AddQuantize();
        gOpResolver.AddDequantize();
        gOpResolver.AddAdd();
        gOpResolver.AddMul();
        gOpResolver.AddMean();
        gOpResolver.AddExpandDims();
        gOpsAdded = true;
    }
}

bool modelInit(const unsigned char* model, byte* arena, int arenaSize) {
    addRequiredOps();
    gModel = tflite::GetModel(model);
    if (gModel->version() != TFLITE_SCHEMA_VERSION) {
        Serial.println("Model schema version mismatch!");
        return false;
    }

    gInterpreter = new tflite::MicroInterpreter(gModel, gOpResolver, arena, arenaSize);
    if (!gInterpreter || gInterpreter->AllocateTensors() != kTfLiteOk) {
        return false;
    }

    gInputTensor = gInterpreter->input(0);
    gOutputTensor = gInterpreter->output(0);
    return gInputTensor != nullptr && gOutputTensor != nullptr;
}

bool modelSetInput(float inputValue, int index) {
    if (!gInputTensor || index < 0 || index >= (gInputTensor->bytes / (int)sizeof(float))) {
        return false;
    }
    gInputTensor->data.f[index] = inputValue;
    return true;
}

bool modelRunInference() {
    return gInterpreter && gInterpreter->Invoke() == kTfLiteOk;
}

float modelGetOutput(int index) {
    if (!gOutputTensor || index < 0 || index >= (gOutputTensor->bytes / (int)sizeof(float))) {
        return -1.0f;
    }
    return gOutputTensor->data.f[index];
}

void setup() {
    Serial.begin(115200);
    pinMode(RADAR_ANALOG_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(ALERT_LED_PIN, OUTPUT);

    digitalWrite(ALERT_LED_PIN, HIGH); // OFF
    digitalWrite(BUZZER_PIN, LOW);     // OFF

    Serial.printf("Free heap before arena alloc: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Requesting tensor arena: %u bytes\n", (unsigned)TENSOR_ARENA_SIZE);

    tensorArena = (byte*)malloc(TENSOR_ARENA_SIZE);
    if (!tensorArena) {
        Serial.println("ERROR: Tensor arena allocation failed.");
        while (1) delay(100);
    }

    // Initialize the AI engine using ArduTFLite
    if (!modelInit(echo_model_tflite, tensorArena, TENSOR_ARENA_SIZE)) {
        Serial.println("ERROR: AI Brain Initialization Failed.");
        while (1) delay(100);
    }

    Serial.println("\nPROJECT E.C.H.O. V6.0 ONLINE");
    Serial.println("ENEMY DETECTION SYSTEM READY.");
}

void loop() {
    bool testDataReceived = false;
    float sample = 0.0f;

    // Simulation Data Parsing (From test_drone.csv strings)
    if (Serial.available()) {
        Serial.println("Data Packet Found. Processing Simulation...");
        // Stream each value directly into the model input to avoid a large RAM buffer.
        for (int i = 0; i < NUMBER_OF_INPUTS; i++) {
            sample = Serial.parseFloat();
            if (!modelSetInput(sample, i)) {
                Serial.println("ERROR: Failed to set input.");
                while (Serial.available()) Serial.read();
                delay(1000);
                return;
            }
        }
        while (Serial.available()) Serial.read(); // Flush trailing buffer
        testDataReceived = true;
    }

    // Live Radar Sampling (If no serial data is arriving)
    if (!testDataReceived) {
        for (int i = 0; i < NUMBER_OF_INPUTS; i++) {
            sample = (float)analogRead(RADAR_ANALOG_PIN) / 143.2784;
            if (!modelSetInput(sample, i)) {
                Serial.println("ERROR: Failed to set input.");
                delay(1000);
                return;
            }
            delayMicroseconds(50);
        }
    }

    // --- AI Classification Phase ---
    // 1. Run inference
    if (!modelRunInference()) {
        Serial.println("Classification Error.");
        delay(1000);
        return;
    }

    // 2. Read outputs and find best class (manual argmax)
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

    // Structured JSON for Member 3's Frontend Map & Logs
    Serial.printf("{\"node\":\"ECHO-01\",\"type\":\"%s\",\"conf\":%.2f,\"status\":\"%s\"}\n",
                  labels[prediction],
                  maxProb * 100.0,
                  (prediction == 1) ? "ENEMY" : "NORMAL");

    // Trigger Alert Hardware
    if (prediction == 1) triggerAlert(5);

    if (testDataReceived) {
        delay(2000); // Wait for user to read result
        Serial.println("Ready for next sample.");
    }
}

void triggerAlert(int flashes) {
    for (int i = 0; i < flashes; i++) {
        digitalWrite(ALERT_LED_PIN, LOW);  // ON
        digitalWrite(BUZZER_PIN, HIGH);    // BEEP
        delay(100);
        digitalWrite(ALERT_LED_PIN, HIGH); // OFF
        digitalWrite(BUZZER_PIN, LOW);     // SILENT
        delay(100);
    }
}
