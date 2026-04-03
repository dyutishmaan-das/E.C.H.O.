---
description: UAV Detection System Architecture and Workflow (Team Division)
---

# Defensive UAV Detection System (ESP8266 + CDM324)

This document outlines the workflow and division of responsibilities for a 4-member team building a UAV/Drone detection system using microcontrollers and Machine Learning (TinyML). The system uses a 1D-CNN model trained on Zenodo 77GHz FMCW radar data, scaled for 24GHz CDM324 hardware, and deployed on a NodeMCU ESP8266.

## Team Division

### Team 1: Embedded AI and Hardware Node
Responsible for: The physical sensor node, AI model training and deployment, firmware for the ESP8266, and local alert hardware (LEDs, buzzer, OLED display).

### Team 2: Backend Cloud and Security Architecture
Responsible for: The backend server (Node.js + Express), MongoDB database, JWT authentication with role-based access control, M2F (Machine-to-Firewall) API key auth for ESP8266, flight plan management, and automated alert scripts (Email/SMS via Nodemailer/Twilio).

### Team 3: Command Center Frontend and Project Documentation
Responsible for: Building the web dashboard (Leaflet.js radar map, log table, Chart.js bar graph, alert overlay screen), login page with role-based UI, and leading the final project documentation and presentation.

### Team 4: Patent Research and Novelty Justification
Responsible for: Conducting prior art searches, comparing E.C.H.O. with existing patents and research papers, and justifying the project's novelty (low-cost edge AI detection vs. expensive military-grade systems).

---

## Friend or Foe (IFF) Identification

### How to determine if a detected drone is Enemy or Friendly

The system uses a **Digital Handshake** method built on the ESP8266's native WiFi capability.

**Step 1 — The Drone "Tag" (Broadcaster):**
- A tiny **ESP-01** module (approximately 1.5 grams) is attached to each friendly drone's frame.
- The ESP-01 is powered from the drone's existing 5V battery rail (flight controller power bus).
- The ESP-01 is programmed in **AP Mode** (Access Point). It does not connect to the internet. It simply broadcasts a hidden SSID such as `ECHO_FRIENDLY_01`.
- For commercial drones (e.g., DJI), the existing RemoteID broadcast over WiFi/Bluetooth can be sniffed instead.

**Step 2 — The Ground Node (Listener):**
- When the CDM324 radar detects a drone signature, the ESP8266 immediately runs `WiFi.scanNetworks()`.
- It compares discovered SSIDs against a whitelist of known friendly beacon names.
- If a match is found, the detection is marked as `FRIENDLY`.
- If no match is found, the detection is marked as `ENEMY`.

**Step 3 — Flight Plan Cross-Reference (Backend):**
- Even if the beacon scan fails (drone is out of WiFi range), the backend server performs a secondary check.
- The admin uploads a **Flight Plan** (see format below) that specifies time windows and geographic zones for authorized flights.
- If a detection falls within an active flight plan's time and zone, the backend overrides the status to `FRIENDLY`.

**Range Considerations:**
- Standard ESP-01 + NodeMCU: reliable up to approximately 100 meters altitude.
- With a high-gain SMA antenna (5dBi or 8dBi) on the ground node: approximately 300 to 500 meters.
- For ranges beyond 1km: swap ESP-01 tag for a LoRa beacon module (future scope).

---

## Admin Flight Plan Format

The admin must upload flight plans through the backend API in the following JSON format. This allows the system to automatically distinguish authorized (friendly) drone operations from intruders.

```json
{
  "plan_id": "FP-2026-04-03-001",
  "drone_callsign": "ALPHA-7",
  "operator": "Lt. Kumar",
  "start_time": "2026-04-03T06:00:00Z",
  "end_time": "2026-04-03T08:00:00Z",
  "zone": {
    "center_lat": 28.6139,
    "center_lng": 77.2090,
    "radius_m": 500
  },
  "beacon_ssid": "FRIENDLY_ECHO_ALPHA7",
  "status": "ACTIVE"
}
```

The backend checks incoming detections against all active flight plans. If the detection timestamp falls within `start_time` to `end_time` and the radar node's location is within the specified zone, the detection is flagged as `FRIENDLY` regardless of the beacon scan result.

---

## OLED Dashboard Display (Local Status Screen)

In addition to the web-based frontend dashboard (Team 3), the ESP8266 ground node drives a small **SSD1306 OLED display** (128x64 pixels, I2C interface on pins D3/D4) to show real-time status directly on the hardware unit.

**OLED Display Layout:**
```
+---------------------------+
| E.C.H.O. Node: ECHO-01   |
| Status: IDLE              |
|                           |
| Last: Bird  (88.1%)       |
| Enemies today: 0          |
+---------------------------+
```

When an enemy drone is detected:
```
+---------------------------+
| *** ENEMY DETECTED ***    |
| Type: Drone               |
| Confidence: 97.4%         |
| Status: ENEMY             |
| Streak: 3                 |
+---------------------------+
```

This allows field operators to monitor the node without needing a laptop or phone to view the web dashboard.

---

## Workflow Steps

### Phase 1: Data Preparation and ML Training (Team 1)
1. Load the Zenodo 77GHz FMCW radar dataset (`.npy` format).
2. Extract 1280-sample windows for Bird, Drone, and Human classes.
3. Apply 24GHz spectral scaling (factor 0.311) to simulate CDM324 hardware.
4. Normalize data by global maximum value (stored as `143.2784` for ESP8266 firmware).
5. Train a 1D-CNN model (Conv1D 64 filters, Conv1D 128 filters, GlobalAveragePooling, Dense 64, Dense 32, Dense 3 with softmax).
6. Save stable model as `.h5` and convert to TFLite, then to C header (`.h`).

### Phase 2: Microcontroller Firmware (Team 1)
1. Flash the NodeMCU ESP8266 with `node_echo_alert_firmware.ino` (v6.0).
2. The firmware uses Chirale_TensorFlowLite for inference (not EloquentTinyML).
3. The firmware supports dual mode: live radar sampling (A0 pin) and serial simulation (paste CSV strings).
4. LED behaviour: 2x Yellow alternating (idle), Red+Blue alternating + SOS buzzer (enemy), Yellow+Red+Blue alternating (friendly).
5. Activity gating with baseline calibration (20 frames on boot) prevents false positives from static noise.
6. Enemy streak filter requires 3 consecutive high-confidence drone predictions before triggering ENEMY alert.
7. Structured JSON output over serial for backend ingestion.
8. OLED display shows real-time node status, last detection, and enemy count.

### Phase 3: Cloud Infrastructure (Team 2)
1. Set up Node.js + Express REST API with MongoDB.
2. Implement JWT authentication with three roles: `admin`, `operator`, `viewer`.
3. Implement M2F (Machine-to-Firewall) API key authentication for ESP8266 POST requests.
4. Build detection ingestion endpoint (`POST /api/detections`).
5. Build flight plan CRUD endpoints (admin only).
6. Implement friend/foe cross-reference logic (check flight plans on each detection).
7. Set up automated email alerts (Nodemailer or SendGrid) and SMS alerts (Twilio).
8. WebSocket (Socket.io) events for real-time dashboard updates.
9. Detection stats aggregation endpoint for the bar graph.

### Phase 4: Frontend Dashboard (Team 3)
1. Build radar map with Leaflet.js showing coverage circle and detection pins.
2. Build log table with real-time WebSocket updates.
3. Build date-wise bar graph with Chart.js (stacked: enemy drones, friendly drones, birds, humans).
4. Build alert screen overlay (full-screen red with acknowledge button).
5. Login page with JWT auth flow.
6. Role-based UI (admin sees flight plan management, operator sees acknowledge buttons, viewer is read-only).

### Phase 5: Full System Integration Test
1. Team 1 boots the ESP8266 with radar connected (or uses serial simulation).
2. ESP8266 classifies the target, triggers local LEDs/buzzer/OLED, and POSTs JSON to backend.
3. Team 2's backend validates the API key, cross-references flight plans, stores the log, and sends alerts if ENEMY.
4. Team 3's dashboard receives WebSocket events: map updates, log table fills, bar graph refreshes, alert overlay fires on ENEMY.

### Phase 6: Documentation and Presentation (Team 3 and Team 4)
1. System architecture document with hardware choices, ML pipeline, and network diagram.
2. Setup instructions (README) for firmware, backend server, and frontend dashboard.
3. PowerPoint presentation covering problem statement, solution architecture, results, and live demo.
4. Patent comparison and novelty justification from Team 4.