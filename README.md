# ESP8266 Sensor & Firebase Uploader

Small collection of Arduino sketches and a Python uploader for sending sensor data to Firebase.

Contents
- [Esp8266/DHT11check.ino](Esp8266/DHT11check.ino) — DHT11 temperature/humidity test sketch
- [Esp8266/MQ2check.ino](Esp8266/MQ2check.ino) — MQ-2 gas sensor test sketch
- [Esp8266/GPScheck.ino](Esp8266/GPScheck.ino) — GPS module test sketch
- [Esp8266/MACaddresschec.ino](Esp8266/MACaddresschec.ino) — MAC address / WiFi info example
- [Esp8266/receiver'scode.ino](Esp8266/receiver'scode.ino) — Example receiver sketch
- [Esp8266/sender'scode1.ino](Esp8266/sender'scode1.ino) — Example sender sketch
- [Esp8266/sender2.ino](Esp8266/sender2.ino) — Alternate sender example
- [Esp8266/firebase_uploader.py](Esp8266/firebase_uploader.py) — Python script to upload collected data to Firebase
- [Esp8266/firebase-uploader.service](Esp8266/firebase-uploader.service) — Example systemd service for running the uploader on boot
- [Esp8266/raspberrypi'stask.md](Esp8266/raspberrypi'stask.md) — Notes for Raspberry Pi tasks

Requirements
- Arduino IDE (or PlatformIO) with ESP8266 board support installed
- Python 3 (for `firebase_uploader.py`)
- Internet access and a Firebase Realtime Database or Firestore project (adjust `firebase_uploader.py` accordingly)

Quick start

1. Open the appropriate sketch in the Arduino IDE (for example, [Esp8266/DHT11check.ino](Esp8266/DHT11check.ino)).
2. Update WiFi and any sensor pins or configuration constants in the sketch.
3. Select the correct ESP8266 board and upload the sketch to your device.

Running the Firebase uploader

- Edit `firebase_uploader.py` to include your Firebase project credentials or configuration. The script currently expects credentials/config inside the file or an adjacent config file — inspect the script to confirm exact fields.
- Run locally for testing:

```
python3 Esp8266/firebase_uploader.py
```

Deploying as a service (Raspberry Pi)

- Copy [Esp8266/firebase-uploader.service](Esp8266/firebase-uploader.service) to `/etc/systemd/system/` and adjust the `ExecStart` path and user.
- Enable and start the service:

```
sudo systemctl daemon-reload
sudo systemctl enable firebase-uploader.service
sudo systemctl start firebase-uploader.service
```

Notes & tips
- Many sketches contain hard-coded WiFi or endpoint values — search each `.ino` and `firebase_uploader.py` for `SSID`, `PASSWORD`, `FIREBASE` or similar and update them before use.
- Filenames in this repo include apostrophes; if you copy or reference them in shell commands, escape or quote them appropriately.

License
- No license specified. Add a `LICENSE` file if you want to set reuse terms.

Questions or changes
- Tell me if you want this README expanded with example configs, exact Python requirements, or usage examples extracted from the uploader script.
