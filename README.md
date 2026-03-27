# Project E.C.H.O.
*(Edge Computing Hardware for Overhead-observation)*

## Problem Statement
Traditional perimeter defense systems struggle to identify small, rogue UAVs (drones) in poor visibility, often confusing them with biological clutter like birds. There is a critical need for a low-cost, decentralized edge-computing node that uses embedded Machine Learning (TinyML) on microcontrollers (STM32/ESP8266) to continuously scan radar frequencies, instantly distinguish the micro-Doppler signature of drone propellers from wildlife, and alert command centers in any weather condition.

---

## The "True TinyML" Simulated Radar Solution
As opposed to heavy computing models using Computer Vision (which fail in the dark or fog), this project relies entirely on **Radio Frequency (RF)** radar analysis. Because our hardware is the tiny, $2 **STM32 Blue Pill**, we are building a 1D Convolutional Neural Network via **Edge Impulse**.

Currently, we are functioning on **Phase 1** (Simulated Radar). Rather than hooking up physical Doppler radar hardware immediately, we use Python mathematics to generate the electrical signal an STM32 analog pin would theoretically receive.

---

## Repository Contents
1. `generate_radar_data.py`: The physics engine. Run this Python script to mathematically spit out a 60-file CSV dataset mimicking the analog wave readings for *Empty Air*, *Birds*, and *Drones*. This dataset is designed precisely for the Edge Impulse studio.
2. `radar_dataset/`: The output folder created by the python script. This holds the `.csv` files required to train our Machine Learning model.
3. `.agents/workflows/uav_detection_workflow.md`: A highly detailed markdown document explaining the **3-Team Member Division of Labor** and the 6-phase project execution plan.

---

## Next Steps (The Workflow Plan)
* **Team 1 (Embedded & AI)** will upload this dataset to Edge Impulse, train the C++ AI library, and write the STM32 + ESP8266 communication firmware.
* **Team 2 (Backend Cloud)** will build a secure MQTT/HTTP receiver server to log incoming drone alerts from the ESP8266 to a database.
* **Team 3 (Command Frontend & Presentation)** will build the defense operator's web dashboard and compile the Final Project Documentation and PowerPoint Presentation.

*Watch the skies.*
