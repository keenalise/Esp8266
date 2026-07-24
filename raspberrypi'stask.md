# Raspberry Pi Setup — Fire and Gas Monitoring System

This document records the steps carried out on the Raspberry Pi 4 as part of the
Fire and Gas Monitoring System project. The Raspberry Pi acts as the central
processing unit of the system: it receives sensor data forwarded by the receiver
NodeMCU over USB serial, uploads it to Firebase Realtime Database, maintains a
history log of readings and alerts, and sends push notifications to a mobile
phone via ntfy.sh whenever fire or smoke is detected.

## 1. Locating the Raspberry Pi on the Network

Since the Raspberry Pi obtains its IP address automatically from the router
(DHCP), the address can change between sessions. Before connecting, the
correct current IP address was found by scanning the local network from a
laptop terminal.

Install the network scanning tool (if not already installed):

```bash
sudo apt install nmap -y
```

Scan the local subnet to list all connected devices:

```bash
sudo nmap -sn 10.177.64.0/24
```

Filter the results specifically for a Raspberry Pi device, where possible:

```bash
sudo nmap -sn 10.177.64.0/24 | grep -B2 -i "raspberry"
```

This returned the Pi's current address on the network, which was then used to
connect to it directly.

## 2. Connecting to the Pi via SSH

With the IP address identified, a remote terminal session was opened from the
laptop using SSH:

```bash
ssh raspberrypi@10.177.64.21
```

On first connection, the terminal displays a host authenticity warning, which
was accepted by typing `yes`. After entering the Pi's password, the terminal
prompt changed to confirm the session was active on the Pi itself:

```
raspberrypi@raspberrypi:~ $
```

## 3. Installing Required Software

The following Python packages were installed on the Pi to support serial
communication with the receiver NodeMCU and HTTP requests to Firebase and
ntfy.sh:

```bash
pip3 install pyserial requests
```

## 4. Identifying the Receiver NodeMCU's Serial Port

With the receiver NodeMCU connected to the Pi via USB, the assigned serial
device was identified:

```bash
ls /dev/tty*
```

The device typically appears as `/dev/ttyUSB0`. This path was used as the
`SERIAL_PORT` value in the upload script (Section 5).

## 5. Creating the Firebase Upload Script

A Python script, `firebase_uploader.py`, was created on the Pi to:

- Read JSON-formatted sensor data lines (prefixed with `DATA:`) sent over
  serial by the receiver NodeMCU.
- Write the latest reading from each sender to `sensor_data/senderX` in
  Firebase Realtime Database (used by the live dashboard panels).
- Permanently log every reading to `sensor_history/senderX` with a
  server-side timestamp (used for the dashboard's historical charts).
- Detect changes in alert state (safe → warning → alarm → clear) and log
  each transition to `alert_history` in Firebase.
- Send a push notification via ntfy.sh whenever a fire (flame sensor) or
  smoke/gas (MQ2 threshold exceeded) condition is detected, and again when
  the reading clears.

The script was created and edited directly on the Pi using:

```bash
nano firebase_uploader.py
```

## 6. Running the Script Manually (Initial Testing)

Before automating the script, it was run manually to confirm it worked
correctly and to observe any errors directly in the terminal:

```bash
python3 firebase_uploader.py
```

## 7. Running the Script Automatically as a Background Service

To ensure the script runs continuously — starting automatically on boot and
restarting itself if it ever crashes — it was configured as a `systemd`
service.

A service definition file was created:

```bash
sudo nano /etc/systemd/system/firebase-uploader.service
```

The service was then registered, enabled to start on boot, and started:

```bash
sudo systemctl daemon-reload
sudo systemctl enable firebase-uploader.service
sudo systemctl start firebase-uploader.service
```

## 8. Monitoring and Troubleshooting the Service

To confirm the service was running correctly:

```bash
sudo systemctl status firebase-uploader.service
```

To view live output from the script (equivalent to watching it run manually,
but for the background service):

```bash
journalctl -u firebase-uploader.service -f
```

Whenever the script was updated (for example, after adding sensor history
logging or fixing the ntfy notification encoding issue), the service was
restarted to apply the changes:

```bash
sudo systemctl restart firebase-uploader.service
```

This live log view was particularly useful for diagnosing two issues during
development:

- Confirming that Firebase uploads were succeeding for each sensor reading.
- Identifying a `UnicodeEncodeError` that was crashing the script, caused by
  emoji characters placed directly inside an HTTP header when sending ntfy
  notifications. This was resolved by removing raw emoji characters from
  the notification title and relying on ntfy's built-in tag-based emoji
  icons instead.

## 9. ntfy.sh Push Notification Setup

A unique topic name was chosen and subscribed to using the ntfy mobile app.
The topic was first tested manually from the Pi's terminal, independently of
the Python script, to confirm the notification pathway worked before
integrating it into the upload script:

```bash
curl -d "Manual test message" https://ntfy.sh/<topic-name>
```

Once confirmed, the topic URL was added to `firebase_uploader.py`, so that
fire and smoke detections trigger real-time push notifications to the
subscribed phone.

## Summary of the Data Flow

```
Sender NodeMCU 1 ─┐
                   ├─► ESP-NOW ─► Receiver NodeMCU ─► USB Serial ─► Raspberry Pi
Sender NodeMCU 2 ─┘                                                     │
                                                                         ▼
                                                        firebase_uploader.py
                                                                         │
                                   ┌─────────────────────────────────────┼──────────────────────────┐
                                   ▼                                     ▼                          ▼
                     Firebase Realtime Database                 Firebase Alert/History         ntfy.sh
                     (sensor_data — live values)                 (sensor_history, alert_history)  (push notification)
                                   │
                                   ▼
                        Real-time Monitoring Dashboard
```
