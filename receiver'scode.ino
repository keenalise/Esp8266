/*
  ============================================================
  RECEIVER NODE MCU CODE - ESP-NOW Receiver + Serial Bridge
  ============================================================
  Project   : Fire Detection and Environmental Monitoring System
  Board     : NodeMCU ESP8266 (Receiver)
  This board's MAC address should be: 50:02:91:e1:4a:44

  ROLE OF THIS BOARD:
  - Receives sensor data packets from BOTH sender NodeMCUs via
    ESP-NOW (senders are identified by the "senderId" field: 1 or 2).
  - Forwards each received packet as a JSON line over USB Serial
    to a Raspberry Pi 4, which will then push the data to Firebase
    and handle ntfy.sh fire notifications.

  CONNECTION TO RASPBERRY PI:
  - Connect this NodeMCU to the Raspberry Pi using a USB cable
    (same one used for programming).
  - On the Raspberry Pi, read the data using pyserial, e.g.:
      import serial
      ser = serial.Serial('/dev/ttyUSB0', 115200)
      line = ser.readline().decode().strip()
      if line.startswith("DATA:"):
          json_str = line[5:]
          # json.loads(json_str) to parse it

  Required libraries: none beyond the ESP8266 core (ESP8266WiFi,
  espnow) — no sensor libraries are needed on this board.
  ============================================================
*/

#include <ESP8266WiFi.h>
#include <espnow.h>

// ---------------- Data Packet Structure ----------------
// This MUST exactly match the struct used on both sender boards.
typedef struct struct_message {
  uint8_t senderId;
  float   temperature;
  float   humidity;
  int     mq2Value;
  bool    flameDetected;
  float   latitude;
  float   longitude;
  bool    gpsFix;
} struct_message;

struct_message incomingData;

// ---------------- ESP-NOW Receive Callback ----------------
// This function runs automatically whenever a packet arrives
// from either sender board.
void onDataRecv(uint8_t *mac, uint8_t *incomingBytes, uint8_t len) {
  if (len != sizeof(incomingData)) {
    Serial.println("Warning: received packet size mismatch, ignoring.");
    return;
  }

  memcpy(&incomingData, incomingBytes, sizeof(incomingData));
  forwardToRaspberryPi(incomingData);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // Not connecting to any router, ESP-NOW only

  Serial.println();
  Serial.println("=== Receiver NodeMCU Starting ===");
  Serial.print("This board's MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("(Confirm this matches 50:02:91:e1:4a:44)");

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed. Restarting...");
    ESP.restart();
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Receiver ready. Waiting for sender data...");
  Serial.println();
}

void loop() {
  // Nothing needed here — all handling happens in onDataRecv()
}

// ---------------- Forward Data to Raspberry Pi ----------------
// Builds a compact JSON line and sends it over USB Serial.
// The "DATA:" prefix lets the Raspberry Pi script easily separate
// actual sensor data from any other debug text.
void forwardToRaspberryPi(struct_message &data) {
  String json = "{";
  json += "\"sender\":" + String(data.senderId) + ",";
  json += "\"temp\":" + String(data.temperature, 2) + ",";
  json += "\"hum\":" + String(data.humidity, 2) + ",";
  json += "\"mq2\":" + String(data.mq2Value) + ",";
  json += "\"flame\":" + String(data.flameDetected ? "true" : "false") + ",";
  json += "\"lat\":" + String(data.latitude, 6) + ",";
  json += "\"lng\":" + String(data.longitude, 6) + ",";
  json += "\"gpsFix\":" + String(data.gpsFix ? "true" : "false");
  json += "}";

  Serial.println("DATA:" + json);
}
