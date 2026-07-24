"""
============================================================
FIREBASE UPLOADER - Reads receiver NodeMCU serial data and
pushes it to Firebase Realtime Database, including a
persistent sensor history log and an alert history log.
============================================================
Run this on the Raspberry Pi with the receiver NodeMCU
connected via USB.
============================================================
"""

import serial
import json
import requests
import time

# ---------------- CONFIGURATION ----------------
SERIAL_PORT = "/dev/ttyUSB0"   # Change this to match your device
BAUD_RATE = 115200
FIREBASE_URL = "https://nodemcu-2cf24-default-rtdb.firebaseio.com"
MQ2_THRESHOLD = 400
NTFY_TOPIC_URL = "https://ntfy.sh/wildnahiwildddddddfire"
# -------------------------------------------------

# Tracks the last known alarm state per sender, so we only log an
# alert_history entry when the state actually CHANGES, not on every reading.
last_alarm_state = {1: "safe", 2: "safe"}


def connect_serial():
    while True:
        try:
            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
            print(f"Connected to {SERIAL_PORT}")
            return ser
        except serial.SerialException:
            print(f"Could not open {SERIAL_PORT}, retrying in 3 seconds...")
            time.sleep(3)


def compute_state(data):
    """Determine safe / warning / alarm from a sensor reading."""
    if data.get("flame") is True:
        return "alarm"
    if isinstance(data.get("mq2"), (int, float)) and data["mq2"] > MQ2_THRESHOLD:
        return "warning"
    return "safe"


def send_current_snapshot(data):
    """Overwrites the 'latest reading' node used by the live dashboard panels."""
    sender_id = data.get("sender", "unknown")
    path = f"{FIREBASE_URL}/sensor_data/sender{sender_id}.json"
    try:
        response = requests.put(path, json=data, timeout=5)
        if response.status_code == 200:
            print(f"Uploaded sender{sender_id} current snapshot.")
        else:
            print(f"Firebase snapshot upload failed. Status: {response.status_code}, Response: {response.text}")
    except requests.exceptions.RequestException as e:
        print(f"Network error while uploading snapshot to Firebase: {e}")


def push_sensor_history(data):
    """Appends this reading to a permanent, timestamped history log."""
    sender_id = data.get("sender", "unknown")
    path = f"{FIREBASE_URL}/sensor_history/sender{sender_id}.json"
    record = dict(data)
    record["timestamp"] = {".sv": "timestamp"}  # Firebase server-side timestamp
    try:
        response = requests.post(path, json=record, timeout=5)
        if response.status_code != 200:
            print(f"Sensor history push failed. Status: {response.status_code}, Response: {response.text}")
    except requests.exceptions.RequestException as e:
        print(f"Network error while pushing sensor history: {e}")


def send_ntfy_notification(title, message, priority="default", tags=None):
    """Sends a push notification to your phone via ntfy.sh."""
    headers = {"Title": title, "Priority": priority}
    if tags:
        headers["Tags"] = tags
    try:
        response = requests.post(NTFY_TOPIC_URL, data=message.encode("utf-8"), headers=headers, timeout=5)
        if response.status_code == 200:
            print(f"ntfy notification sent: {title}")
        else:
            print(f"ntfy notification failed. Status: {response.status_code}, Response: {response.text}")
    except requests.exceptions.RequestException as e:
        print(f"Network error while sending ntfy notification: {e}")


def push_alert_history(sender_id, alert_type, message):
    """Logs an alert state transition (alarm/warning/clear) permanently."""
    path = f"{FIREBASE_URL}/alert_history.json"
    record = {
        "sender": sender_id,
        "type": alert_type,
        "message": message,
        "timestamp": {".sv": "timestamp"},
    }
    try:
        response = requests.post(path, json=record, timeout=5)
        if response.status_code == 200:
            print(f"Alert logged: sender{sender_id} -> {alert_type} ({message})")
        else:
            print(f"Alert history push failed. Status: {response.status_code}, Response: {response.text}")
    except requests.exceptions.RequestException as e:
        print(f"Network error while pushing alert history: {e}")


def handle_alert_transition(data):
    sender_id = data.get("sender", 0)
    new_state = compute_state(data)
    old_state = last_alarm_state.get(sender_id, "safe")

    if new_state == old_state:
        return  # No change, nothing to log

    if new_state == "alarm":
        push_alert_history(sender_id, "alarm", "Flame detected")
        send_ntfy_notification(
            title=f"🔥 Fire Alert — Sender {sender_id}",
            message=f"Fire is detected in Sender {sender_id}",
            priority="urgent",
            tags="fire,rotating_light",
        )
    elif new_state == "warning":
        push_alert_history(sender_id, "warning", f"MQ2 reading exceeded {MQ2_THRESHOLD}")
        send_ntfy_notification(
            title=f"⚠️ Smoke Alert — Sender {sender_id}",
            message=f"Smoke is detected in Sender {sender_id}",
            priority="high",
            tags="warning",
        )
    else:
        push_alert_history(sender_id, "clear", "Reading returned to normal")
        send_ntfy_notification(
            title=f"✅ Cleared — Sender {sender_id}",
            message=f"Sender {sender_id} readings are back to normal.",
            priority="low",
            tags="white_check_mark",
        )

    last_alarm_state[sender_id] = new_state


def main():
    ser = connect_serial()
    print("Listening for sensor data...")

    while True:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line.startswith("DATA:"):
                json_str = line[5:]
                try:
                    data = json.loads(json_str)
                except json.JSONDecodeError:
                    print(f"Skipping malformed JSON: {json_str}")
                    continue

                print(f"Received: {data}")
                send_current_snapshot(data)
                push_sensor_history(data)
                handle_alert_transition(data)

        except serial.SerialException:
            print("Serial connection lost. Reconnecting...")
            ser.close()
            ser = connect_serial()
        except KeyboardInterrupt:
            print("\nStopping uploader.")
            ser.close()
            break


if __name__ == "__main__":
    main()
