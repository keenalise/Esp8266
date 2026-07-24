# Forest Fire Prediction and Monitoring System

A distributed IoT and machine learning system for forest fire risk
monitoring and prediction. Two sensor nodes (NodeMCU ESP8266) deployed across
a forest area collect environmental data, which is relayed through a
receiver node to a Raspberry Pi 4 gateway. The Raspberry Pi uploads live
readings to Firebase Realtime Database for real-time monitoring via a web
dashboard, and feeds sensor data into a machine learning model trained on
historical forest fire and environmental datasets to predict the likelihood
of a forest fire occurring in the monitored area. The system pushes
real-time alerts to both the dashboard and a mobile phone (via ntfy.sh)
whenever fire or smoke is detected, in addition to its predictive
capability.

## System Architecture

```
 Zone 01                          Zone 02
 Sender NodeMCU 1                 Sender NodeMCU 2
 (DHT11, MQ2, Flame, GPS)         (DHT11, MQ2, Flame, GPS)
        │                                  │
        └──────────────┬───────────────────┘
                        │  ESP-NOW (wireless)
                        ▼
              Receiver NodeMCU
                        │  USB Serial (JSON)
                        ▼
              Raspberry Pi 4
        (firebase_uploader.py service)
                        │
        ┌───────────────┼────────────────┬───────────────────┐
        ▼               ▼                ▼                   ▼
 Firebase Realtime   Firebase Alert   ntfy.sh          Machine Learning
 Database            & Sensor         (push notification  Prediction Module
 (live + history)    History Logs      to mobile phone)   (forest fire risk
        │                                                  prediction)
        ▼                                                        │
 Real-time Web Dashboard  ◄──────────────────────────────────────┘
 (firemonitoringdashboard.html)
```

## Project Status

| Component                                      | Status         |
|-------------------------------------------------|----------------|
| Sender node firmware (sensors + ESP-NOW)         | Complete       |
| Receiver node firmware                           | Complete       |
| Raspberry Pi data bridge (Firebase + ntfy.sh)    | Complete       |
| Real-time dashboard                              | Complete       |
| Sensor and alert history logging                 | Complete       |
| Machine learning forest fire prediction module   | In development |

## Repository Structure

| File                        | Description                                                                 |
|-----------------------------|-------------------------------------------------------------------------------|
| `sender'scode1.ino`          | Firmware for Sender NodeMCU 1 (Zone 01). Reads DHT11, MQ2, flame, and GPS, then transmits readings to the receiver via ESP-NOW. |
| `sender2.ino`                | Firmware for Sender NodeMCU 2 (Zone 02). Identical to Sender 1, distinguished by `SENDER_ID = 2`. |
| `receiver'scode.ino`         | Firmware for the Receiver NodeMCU. Receives ESP-NOW packets from both senders and forwards them as JSON over USB serial to the Raspberry Pi. |
| `DHT11check.ino`             | Standalone test sketch used to verify the DHT11 temperature/humidity sensor in isolation, without ESP-NOW or GPS interference. |
| `MQ2check.ino`               | Standalone test sketch used to verify MQ2 gas sensor readings. |
| `GPScheck.ino`               | Standalone test sketch used to verify GPS module output with TinyGPS++. |
| `MACaddresschec.ino`         | Utility sketch used to read a NodeMCU board's MAC address, needed to configure the receiver's address in the sender firmware. |
| `firebase_uploader.py`       | Python script run on the Raspberry Pi. Reads serial data from the receiver, uploads live readings to Firebase, logs sensor and alert history, and sends ntfy.sh push notifications on fire/smoke detection. |
| `firebase-uploader.service`  | systemd service definition that runs `firebase_uploader.py` automatically in the background on Pi boot. |
| `firemonitoringdashboard.html` | Self-contained real-time web dashboard. Connects directly to Firebase Realtime Database and displays live readings, historical charts, and alert history. |
| `raspberrypi'stask.md`       | Step-by-step log of the Raspberry Pi setup process (networking, SSH, service configuration, ntfy setup). |
| `README.md`                  | This file — project overview and documentation. |

## Hardware Components

- 2 x NodeMCU ESP8266 (sensor nodes / senders)
- 1 x NodeMCU ESP8266 (receiver)
- 1 x Raspberry Pi 4
- 2 x DHT11 temperature and humidity sensor
- 2 x MQ2 gas/smoke sensor
- 2 x Flame sensor module
- 2 x GPS module

## Data Flow

1. Each sender NodeMCU reads its four sensors every 3 seconds and transmits
   the readings wirelessly to the receiver NodeMCU using the ESP-NOW protocol.
2. The receiver NodeMCU forwards each received packet as a JSON line over
   USB serial to the Raspberry Pi.
3. `firebase_uploader.py`, running continuously as a background service on
   the Pi, parses each JSON line and:
   - Overwrites the corresponding sender's entry under `sensor_data/` in
     Firebase (used for live dashboard readings).
   - Appends the reading to `sensor_history/` with a server-side timestamp
     (used for the dashboard's historical charts).
   - Detects changes in alert state (normal → gas warning → fire alarm →
     cleared) and logs each transition to `alert_history/`.
   - Sends a push notification to a subscribed mobile phone via ntfy.sh
     whenever a fire or smoke condition is newly detected or cleared.
4. The web dashboard connects directly to Firebase using Server-Sent Events
   (no backend server required) and updates instantly as new data arrives.

## Alert Logic

| Condition                          | Zone Status   | Action                                      |
|-------------------------------------|---------------|----------------------------------------------|
| Flame sensor reads `true`           | Fire Alarm    | Dashboard turns red, siren sound plays, ntfy notification "Fire is detected in Sender X" |
| MQ2 reading exceeds 400             | Gas Warning   | Dashboard turns amber, ntfy notification "Smoke is detected in Sender X" |
| Neither condition is met            | Normal        | Dashboard returns to green, ntfy "Cleared" notification sent |

## Dashboard

The dashboard (`firemonitoringdashboard.html`) is a single self-contained HTML
file with no build step or server dependency. It can be opened directly in
any modern browser and includes:

- **Live zone panels** showing temperature, humidity, MQ2 gas level, flame
  status, and GPS coordinates for both senders, updating in real time.
- **System status strip** with per-zone indicator lights, styled after a
  physical fire alarm control panel, showing overall system state at a
  glance (Normal / Gas Warning / Fire Alarm).
- **Sensor History charts** plotting temperature, humidity, and MQ2 trends
  over time for each zone.
- **Alert History log** showing a permanent, timestamped record of every
  alert transition, loaded from Firebase so it persists across page reloads.
- **Audible alarm siren**, synthesized in-browser, that plays automatically
  while a fire condition is active, with a mute toggle.
- **Browser push notifications**, opt-in via the "Enable alerts" button.

### Normal state

All readings within safe thresholds; status strip and zone lights are green.

### Fire alarm state

When a flame sensor reports a detection, the affected zone's panel and the
system status strip turn red, the "FIRE ALARM" status is displayed, an
audible siren plays, and a push notification is sent to the subscribed
mobile device.

## Machine Learning Prediction Module

*Status: in development.*

In addition to real-time monitoring and threshold-based alerts, this project
incorporates a predictive component intended to estimate the likelihood of a
forest fire occurring in the monitored area, rather than only reacting once
one has already started.

**Intended approach:**

- **Input features**: live sensor readings collected by the two sender
  nodes (temperature, humidity, MQ2 gas/smoke level, flame status, and GPS
  location), combined with historical forest fire and environmental
  datasets to provide additional context (e.g. seasonal patterns, regional
  fire history, and weather-related variables).
- **Historical data**: existing forest fire and environmental datasets are
  used to train the model, alongside the live sensor history already being
  logged to `sensor_history/` in Firebase.
- **Output**: a fire risk prediction (e.g. a probability or risk category)
  for the monitored area, intended to be displayed on the dashboard
  alongside the existing live readings and alerts.
- **Model training and integration**: model architecture, training
  pipeline, and integration with the existing Firebase/dashboard system are
  in progress and will be documented here once finalized.

This module is what distinguishes the system from a purely reactive
monitoring setup: threshold-based alerts (flame detected, MQ2 above 400)
respond to a fire that is already occurring, whereas the machine learning
component aims to flag elevated risk conditions before ignition occurs.

## Setup Summary

Detailed Raspberry Pi setup steps (network discovery, SSH access, service
configuration, and ntfy.sh integration) are documented in
[`raspberrypi'stask.md`](./raspberrypi'stask.md).

1. Flash `sender'scode1.ino` to Sender NodeMCU 1 and `sender2.ino` to Sender
   NodeMCU 2, with each sensor wired as documented in the source comments.
2. Flash `receiver'scode.ino` to the receiver NodeMCU and connect it to the
   Raspberry Pi via USB.
3. On the Raspberry Pi, run `firebase_uploader.py` (or install it as a
   background service using `firebase-uploader.service`) after configuring:
   - `FIREBASE_URL` — your Firebase Realtime Database URL
   - `NTFY_TOPIC_URL` — your subscribed ntfy.sh topic
   - `SERIAL_PORT` — the serial device the receiver is connected to
4. Set Firebase Realtime Database rules to allow read/write access for the
   dashboard and uploader script.
5. Open `firemonitoringdashboard.html` in a browser to view live data.

## Technologies Used

- **Arduino / ESP8266 Core** — sender and receiver firmware
- **ESP-NOW** — low-latency wireless communication between NodeMCU boards
- **Python 3** (`pyserial`, `requests`) — Raspberry Pi data bridge
- **systemd** — background service management on the Raspberry Pi
- **Firebase Realtime Database** — live data storage and historical logging
- **Server-Sent Events (SSE)** — real-time dashboard updates without polling
- **Chart.js** — sensor history visualization
- **ntfy.sh** — mobile push notifications
- **Python (scikit-learn / pandas, or equivalent)** — machine learning
  model training and forest fire risk prediction *(module in development)*

## Project Context

This project was developed as part of the BSc (Hons) Computer Science with
Artificial Intelligence programme at Softwarica College of IT and E-commerce,
delivered in partnership with Coventry University, UK.