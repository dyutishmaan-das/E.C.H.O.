/*
 * Project E.C.H.O.
 * NodeMCU radar classification and local alert firmware.
 *
 * Hardware:
 * - Board: NodeMCU ESP8266 (v3)
 * - Radar input: A0
 * - Alert outputs: Buzzer (D5), LED (D4, active LOW)
 *
 * Runtime modes:
 * - Live mode: samples radar signal from A0
 * - Test mode: paste 1280 comma-separated values into Serial Monitor
 */

#include <ESP8266WiFi.h>
#include <EloquentTinyML.h>
#include "echo_model.h"

// Model configuration
#define NUMBER_OF_INPUTS 1280
#define NUMBER_OF_OUTPUTS 3
#define TENSOR_ARENA_SIZE 16*1024

// Pin map
const int RADAR_ANALOG_PIN = A0;
const int BUZZER_PIN = D5;
const int ALERT_LED_PIN = D4;

// Input buffer for one inference window
float input_vector[NUMBER_OF_INPUTS];

// TinyML runtime
Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml;
const char* labels[] = {"Bird", "Drone", "Human"};

void setup() {
    Serial.begin(115200);
    pinMode(RADAR_ANALOG_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(ALERT_LED_PIN, OUTPUT);

    digitalWrite(ALERT_LED_PIN, HIGH); // LED OFF
    digitalWrite(BUZZER_PIN, LOW);     // Buzzer OFF

    if (!ml.begin(echo_model_tflite)) {
        Serial.println("Error: model init failed.");
        while (1) delay(100);
    }

    Serial.println("E.C.H.O. firmware ready.");
    Serial.println("Live mode: sampling radar from A0.");
    Serial.println("Test mode: paste 1280 comma-separated values into Serial.");
}

void loop() {
    bool testDataReceived = false;

    // If serial data is available, use it as one full test frame.
    if (Serial.available()) {
        Serial.println("Reading test frame from serial...");
        for (int i = 0; i < NUMBER_OF_INPUTS; i++) {
            input_vector[i] = Serial.parseFloat();
        }
        while (Serial.available()) Serial.read();
        testDataReceived = true;
    }

    // Otherwise, sample directly from the radar input.
    if (!testDataReceived) {
        for (int i = 0; i < NUMBER_OF_INPUTS; i++) {
            float raw = analogRead(RADAR_ANALOG_PIN);
            input_vector[i] = (float) raw / 143.2784;
            delayMicroseconds(50);
        }
    }

    int prediction = ml.predict(input_vector);
    float confidence = ml.getProbability();

    Serial.printf("[%s] Prediction: %s (%.1f%%)\n",
                  testDataReceived ? "TEST" : "RADAR",
                  labels[prediction],
                  confidence * 100.0f);

    if (prediction == 1) { // 1 = Drone
        triggerAlert(5);
    }

    if (testDataReceived) {
        delay(1000);
    }
}

void triggerAlert(int flashes) {
    Serial.println("Drone detected. Triggering local alarm.");

    for (int i = 0; i < flashes; i++) {
        digitalWrite(ALERT_LED_PIN, LOW);  // LED ON
        digitalWrite(BUZZER_PIN, HIGH);    // Buzzer ON
        delay(100);

        digitalWrite(ALERT_LED_PIN, HIGH); // LED OFF
        digitalWrite(BUZZER_PIN, LOW);     // Buzzer OFF
        delay(100);
    }
}
