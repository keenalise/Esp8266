"""
============================================================
FIREBASE UPLOADER - Reads receiver NodeMCU serial data and
pushes it to Firebase Realtime Database
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
SERIAL_PORT = "/dev/ttyUSB0"   # Change this to match your device (see Step 3)
BAUD_RATE = 115200
FIREBASE_URL = "https://nodemcu-2cf24-default-rtdb.firebaseio.com"
# -------------------------------------------------

def connect_serial():
    while True:
        try:
            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
            print(f"Connected to {SERIAL_PORT}")
            return ser
        except serial.SerialException:
            print(f"Could not open {SERIAL_PORT}, retrying in 3 seconds...")
            time.sleep(3)

def send_to_firebase(data):
    sender_id = data.get("sender", "unknown")
    path = f"{FIREBASE_URL}/sensor_data/sender{sender_id}.json"
    try:
        response = requests.put(path, json=data, timeout=5)
        if response.status_code == 200:
            print(f"Uploaded sender{sender_id} data successfully.")
        else:
            print(f"Firebase upload failed. Status: {response.status_code}, Response: {response.text}")
    except requests.exceptions.RequestException as e:
        print(f"Network error while uploading to Firebase: {e}")

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
                send_to_firebase(data)

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
</br
