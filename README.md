# Project E.C.H.O. 🦅
*(Edge Computing Hardware for Overhead-observation)*

## Problem Statement 🚨
Traditional perimeter defense systems struggle to identify small, rogue UAVs (drones) in poor visibility, often confusing them with biological clutter like birds. There is a critical need for a low-cost, decentralized edge-computing node that uses embedded Machine Learning (TinyML) on microcontrollers (NodeMCU ESP8266) to continuously scan radar frequencies, instantly distinguish the micro-Doppler signature of drone propellers from wildlife, and alert command centers in any weather condition.

---

## The "True TinyML" Simulated Radar Solution 🧠
As opposed to heavy computing models using Computer Vision (which fail in the dark or fog), this project relies entirely on **Radio Frequency (RF)** radar analysis. Because our hardware is the highly-constrained **NodeMCU ESP8266**, we are building a 1D Convolutional Neural Network via **Edge Impulse**.
   
Currently, we are functioning on **Phase 1** (Simulated Radar). Rather than hooking up physical Doppler radar hardware immediately, we use Python mathematics to generate the electrical signal an ESP8266 analog pin would theoretically receive.

---

## 📦 Datasets
> **Note:** Datasets are NOT included in this repository to keep it lightweight. Download them from the links below and place them in the `radar_dataset/` folder.

### 1. Simulated Radar Data (Generated Locally)
Run `generate_radar_data.py` to mathematically generate 60 CSV files simulating the Micro-Doppler signatures of Empty Air, Birds, and Drones.

### 2. Real-World 77GHz FMCW Radar Data (Recommended)
For production-grade AI training, we use the open-source **"Radar measurements on drones, birds and humans with a 77GHz FMCW sensor"** dataset by Alexander Karlsson (2021).
- **Download Link:** [https://zenodo.org/records/5845259](https://zenodo.org/records/5845259)
- **DOI:** 10.5281/zenodo.5845259
- **Contents:** 75,868 radar samples of birds, humans, and 6 types of drones
- **Sensor:** 77 GHz FMCW radar with mechanically scanning antenna
- **Format:** `.npy` (NumPy arrays)

After downloading, place the `.npy` file inside `radar_dataset/` and run `convert_npy.py` to convert it into Edge Impulse-compatible CSV files.

---

## 📁 Repository Contents
| File | Description |
|------|-------------|
| `generate_radar_data.py` | Physics engine that mathematically simulates radar ADC readings for Empty Air, Birds, and Drones. |
| `convert_npy.py` | Converts the Zenodo 77GHz `.npy` dataset into labeled `.csv` files for Edge Impulse. |
| `radar_dataset/` | Output folder for generated/converted CSV files. *(Not tracked by Git — download or generate locally)* |
| `.agents/workflows/uav_detection_workflow.md` | Team workflow document with 3-member division of labor and 6-phase execution plan. |

---

## 🚀 Next Steps (The Workflow Plan)
* **Team 1 (Embedded & AI)** will upload the dataset to Edge Impulse, train the C++ AI library, and write the complete ESP8266 AI Inference and Wi-Fi embedded firmware.
* **Team 2 (Backend Cloud)** will build a secure MQTT/HTTP receiver server to log incoming drone alerts from the ESP8266 to a database.
* **Team 3 (Command Frontend & Presentation)** will build the defense operator's web dashboard and compile the Final Project Documentation and PowerPoint Presentation.

---

*Watch the skies.* 🛸
