# Project E.C.H.O.
*(Edge Computing Hardware for Overhead-observation)*

## Problem Statement
Traditional perimeter defense systems struggle to identify small, rogue UAVs (drones) in poor visibility, often confusing them with biological clutter like birds. There is a critical need for a low-cost, decentralized edge-computing node that uses embedded Machine Learning (TinyML) on microcontrollers (NodeMCU ESP8266) to continuously scan radar frequencies, instantly distinguish the micro-Doppler signature of drone propellers from wildlife, and alert command centers in any weather condition.

---

## The E.C.H.O. Solution
Unlike heavy computing models using Computer Vision (which fail in the dark or fog), this project relies entirely on **Radio Frequency (RF)** radar analysis. 

We utilize a low-cost **CDM324 24GHz CW Doppler Radar** module connected to a **NodeMCU ESP8266**. The ESP8266 runs a highly optimized 1D Convolutional Neural Network (1D-CNN) built from scratch in TensorFlow and deployed directly using `Chirale_TensorFlowLite`. 

### Key Features
*   **Hardware-Scaled Training Pipeline:** We utilize a massive 77GHz FMCW research dataset (Alexander Karlsson, Zenodo) and mathematically scale the frequency signatures down by a factor of `0.311` in our custom Python training script to perfectly simulate the output of our 24GHz CDM324 hardware.
*   **Local AI Inference (Edge Computing):** The ESP8266 classifies raw radar ADCs into `Drone`, `Bird`, or `Human` completely offline in milliseconds.
*   **Digital Handshake (IFF):** We distinguish enemy drones from friendly drones using Wi-Fi scanning. Friendly drones carry a lightweight 1.5g ESP-01 module broadcasting a hidden beacon SSID. If the ground node detects a drone but finds the beacon, it suppresses the alarm.
*   **Multi-Layer Fallback:** The Node.js Backend cross-references detections against active Admin Flight Plans to further prevent friendly-fire.
*   **Command Dashboard:** A sleek React web interface for administrators provides a live Leaflet.js radar map, detection logs, and real-time WebSocket alerts.

---

## 📦 Datasets
> **Note:** Datasets are NOT included in this repository to keep it lightweight. Download them and place them in the `radar_dataset/` folder.

For production-grade AI training, we use the open-source **"Radar measurements on drones, birds and humans with a 77GHz FMCW sensor"** dataset by Alexander Karlsson (2021).
- **Download Link:** [https://zenodo.org/records/5845259](https://zenodo.org/records/5845259)
- **Format:** `.npy` (NumPy arrays)

---

## 📁 Repository Structure
*   `ai_workflow/scripts/train_echo_ai.py`: The custom Python pipeline that loads the Xenodo dataset, scales it for 24GHz hardware, builds the 1D-CNN, trains it, and exports it.
*   `ai_workflow/exported_models/`: Contains the generated `.h5` model, the `.tflite` model, and the critical `echo_model.h` C-byte array.
*   `ai_workflow/exported_models/node_echo_alert_firmware/`: The final Arduino IDE sketch containing the v6.2 firmware (AI Inference, IFF Wi-Fi Logic, LEDs, Buzzer, and JSON string generation).
*   `echo_system_architecture.md`: Comprehensive breakdown of the frontend, backend, firmware, and MongoDB schemas. 

---

## 🛡️ Team Structure
*   **Team 1 (Embedded AI)**: Train the 1D-CNN, scale for 24GHz, write the ESP8266 C++ inference firmware, implement LED/Buzzer indicators, and handle IFF scanning.
*   **Team 2 (Backend Cloud)**: Build the Node.js/Express REST API, manage JWT/M2F Authentication, Flight Plans, and automated Twilio/Nodemailer alerts. 
*   **Team 3 (Command Frontend)**: Build the defense operator's web dashboard (Map, Logs, Stats) and compile the Final Project Documentation.
*   **Team 4 (Patent Research)**: Conduct prior art searches, patent comparisons, and establish novelty justification.

---

*Watch the skies.*

---

## STM32 Setup Guide
For full Blue Pill (`STM32F103C8`) installation and upload instructions (Preferences URL, Boards Manager install, `Tools` selections, FTDI bootloader flash, USB upload flow, and troubleshooting), see:

- [STM32_BLUEPILL_SETUP.md](STM32_BLUEPILL_SETUP.md)
