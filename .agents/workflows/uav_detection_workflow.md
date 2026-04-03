---
description: UAV Detection System Architecture and Workflow (Team Division)
---

# Defensive UAV Detection System (ESP01/STM32/ESP8266)

This document outlines the workflow and division of responsibilities for a 3-person team building a UAV/Drone detection system using microcontrollers and Machine Learning (TinyML). Due to the lack of physical radar hardware, this workflow mainly follows the **Simulated Radar (TinyML)** approach running locally on the STM32.

## 👥 Team Division

### Team 1: Embedded AI & Hardware Node (Us)
Responsible for: The physical sensor node simulation, making it "smart" with Machine Learning, and programming the microcontrollers to process the radar array and send an alert trigger.

### Team 2: Backend Cloud & Security Architecture (Member 2)
Responsible for: The backend server that catches alerts from the ESP8266, logging detections into a database securely, and handling automated alert scripts (SMS/Email/Telegram).

### Team 3: Command Center Frontend & Project Documentation (Member 3)
Responsible for: Building the user interface (web dashboard) for the defense operator, showing live feed status, logging unauthorized UAV detections, and leading the creation of the final project documentation and presentation (PPT).

## 🚀 Workflow Steps

### Phase 1: Data Simulation & ML Training (Team 1)
1. Write a Python script to mathematically generate CSV files containing "fake" 1D radar data (Empty Air, Birds, Drone Propellers).
2. Create an account on [Edge Impulse](https://edgeimpulse.com).
3. Upload the generated CSV data to Edge Impulse via the Data Forwarder or Uploader.
4. Design the Data Impulse (Time Series Data -> Spectrogram -> 1D Convolutional Neural Network).
5. Train the AI model to classify the three states.
6. Export the trained model as an optimized Arduino (C++) library.

### Phase 2: Microcontroller Firmware (Team 1 & 2)
1. **STM32 Blue Pill**: Write C++ code that includes the Edge Impulse library. The code will feed the model a hardcoded array of simulated drone data. When the ML model outputs a high confidence level for `Drone`, it triggers an alert over UART (Serial) to the ESP8266.
2. **ESP8266/ESP-01 Offline Storage (Store-and-Forward)**: Program the ESP to use its internal flash memory (LittleFS) or an SD Card module. If the Wi-Fi connection drops, it must locally log the drone detection timestamp and confidence level.
3. **ESP8266/ESP-01 Online Sync & Email Alert**: When Wi-Fi reconnects, the ESP must read the offline logs, push the backlog to the cloud MQTT/HTTP server to sync the database, and trigger an automated Email Alert (using an SMTP library or Webhook).

### Phase 3: Cloud Infrastructure (Team 2)
1. Set up an MQTT Broker (e.g., Mosquitto) or a REST API backend (e.g., Node.js Express).
2. Listen for incoming alerts over the network from the ESP8266.
3. Store the alert payload (Timestamp, Node ID, Confidence Level) into a database (e.g., InfluxDB, Firebase, or MongoDB).
4. Set up an automated server script to push urgent notifications to the commander's devices when a threshold is breached.

### Phase 4: Frontend Dashboard (Team 3)
1. Create a modern web dashboard framework.
2. Connect the dashboard to the backend API or WebSocket feed.
3. Display real-time alerts and visually differentiate between a "Threat" (Drone) and "Safe" (Bird/Empty).
4. Implement an interactive map view indicating the status and location of the radar node.

### Phase 5: Full System Integration Test
1. Team 1 runs the STM32 code (which processes the simulated rogue drone array).
2. STM32 triggers the ESP8266 via Serial.
3. ESP8266 sends the MQTT payload via Wi-Fi.
4. Team 2's backend catches the payload and securely logs it.
5. Team 3's dashboard instantly flashes a red UAV alert with spatial data for the operator.

### Phase 6: Project Documentation & Presentation (Team 3 Lead)
1. **System Architecture Document:** Create a comprehensive report detailing the hardware choices, Edge Impulse Machine Learning pipeline, and network architecture. Include screenshots of the simulated ML training graphs.
2. **Setup Instructions (README):** Write clear instructions on how to run the STM32 firmware, start the backend server, and launch the frontend dashboard.
3. **PowerPoint Presentation (PPT):** Design a professional presentation covering:
   - Problem Statement & Objective (UAV threat in defense)
   - The "Simulated Radar + TinyML" Solution Architecture
   - Hardware vs. Simulation constraints (Why we used simulated data)
   - Results & Live/Recorded Demo
4. **Coordinate Final Review:** Gather technical info, schematic diagrams, and code snippets from Teams 1 and 2 to finalize the documentation and PPT for final submission.