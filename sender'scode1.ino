/*
  ============================================================
  SENDER NODE MCU CODE - Multi-Sensor ESP-NOW Transmitter
  ============================================================
  Project   : Fire Detection and Environmental Monitoring System
  Board     : NodeMCU ESP8266 (Sender 2)
  Protocol  : ESP-NOW (sends to Receiver NodeMCU)

  SENDER_ID is set to 2 for this board.
  Receiver MAC Address used below: 50:02:91:e1:4a:44

  Required libraries (install via Library Manager):
      * DHT sensor library (Adafruit)
      * Adafruit Unified Sensor
      * TinyGPSPlus
  ============================================================
*/

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <DHT.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// ---------- CHANGE THIS FOR EACH SENDER BOARD ----------
#define SENDER_ID 2   // Use 1 for Sender-1, 2 for Sender-2
// --------------------------------------------------------

// ---------------- Pin Definitions (as wired) ----------------
#define DHTPIN      D2      // DHT11 data pin
#define DHTTYPE     DHT11
#define MQ2_PIN     A0      // MQ2 analog output
#define FLAME_PIN   D6      // Flame sensor digital output
#define GPS_RX_PIN  D1      // NodeMCU RX  <- GPS TX
#define GPS_TX_PIN  D0      // NodeMCU TX  -> GPS RX

// Receiver's MAC address: 50:02:91:e1:4a:44
uint8_t receiverMAC[] = {0x50, 0x02, 0x91, 0xE1, 0x4A, 0x44};

DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
TinyGPSPlus gps;

// ---------------- Data Packet Structure ----------------
// This exact structure must be mirrored on the receiver side.
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

struct_message dataPacket;

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 3000; // send readings every 3 seconds

// ---------------- ESP-NOW Send Callback ----------------
void onDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Send status: ");
  Serial.println(sendStatus == 0 ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);
  dht.begin();
  pinMode(FLAME_PIN, INPUT);

  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP); // Prevent modem sleep from disrupting DHT11 timing
  Serial.print("This sender's MAC address: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed. Restarting...");
    ESP.restart();
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(receiverMAC, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  esp_now_register_send_cb(onDataSent);

  Serial.println("Sender ready. Sender ID = " + String(SENDER_ID));
}

void loop() {
  // Continuously feed incoming GPS characters to the parser
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  if (millis() - lastSendTime >= sendInterval) {
    lastSendTime = millis();
    readAndSendSensorData();
  }
}

void readAndSendSensorData() {
  dataPacket.senderId = SENDER_ID;

  // ---- DHT11 (Temperature & Humidity) ----
  // GPS SoftwareSerial generates RX interrupts whenever data arrives, and if
  // one fires during the DHT11's microsecond-timed handshake, the read gets
  // corrupted. We temporarily pause the GPS listener during the DHT11 read
  // so it can complete without interruption, then resume listening after.
  gpsSerial.stopListening();

  float h = NAN, t = NAN;
  for (int attempt = 0; attempt < 5; attempt++) {
    h = dht.readHumidity();
    t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) break;
    delay(1000);
  }

  gpsSerial.listen(); // resume GPS reading

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT11 sensor after retries!");
    dataPacket.temperature = -1;
    dataPacket.humidity = -1;
  } else {
    dataPacket.temperature = t;
    dataPacket.humidity = h;
  }

  // ---- MQ2 Gas/Smoke Sensor ----
  dataPacket.mq2Value = analogRead(MQ2_PIN); // 0 - 1024

  // ---- Flame Sensor ----
  // Print the raw pin state so you can confirm which logic level your
  // specific module uses, and adjust the onboard sensitivity potentiometer
  // if it triggers with no flame present.
  int flameRaw = digitalRead(FLAME_PIN);
  Serial.print("Flame raw pin state: "); Serial.println(flameRaw);

  // Most flame sensor modules pull the digital pin LOW when flame is detected.
  // If your module behaves the opposite way (as the debug print above will
  // reveal), change LOW to HIGH below.
  dataPacket.flameDetected = (flameRaw == LOW);

  // ---- GPS ----
  // If the GPS module hasn't acquired a fix yet (e.g. indoors, or still
  // acquiring satellites), fall back to a known reference location instead
  // of sending 0.0, 0.0 (which would incorrectly plot as a point in the
  // ocean off the coast of Africa). gpsFix stays false either way, so the
  // receiver/Raspberry Pi can still tell this is a fallback, not a live fix.
  const float FALLBACK_LATITUDE  = 27.7061;  // 27.7061° N
  const float FALLBACK_LONGITUDE = 85.3300;  // 85.3300° E

  if (gps.location.isValid()) {
    dataPacket.latitude  = gps.location.lat();
    dataPacket.longitude = gps.location.lng();
    dataPacket.gpsFix    = true;
  } else {
    dataPacket.latitude  = FALLBACK_LATITUDE;
    dataPacket.longitude = FALLBACK_LONGITUDE;
    dataPacket.gpsFix    = false;
  }

  // ---- Send packet via ESP-NOW ----
  uint8_t result = esp_now_send(receiverMAC, (uint8_t *) &dataPacket, sizeof(dataPacket));

  // ---- Debug output ----
  Serial.println("---------- Sending Data ----------");
  Serial.print("Sender ID   : "); Serial.println(dataPacket.senderId);
  Serial.print("Temperature : "); Serial.print(dataPacket.temperature); Serial.println(" C");
  Serial.print("Humidity    : "); Serial.print(dataPacket.humidity); Serial.println(" %");
  Serial.print("MQ2 Value   : "); Serial.println(dataPacket.mq2Value);
  Serial.print("Flame       : "); Serial.println(dataPacket.flameDetected ? "FIRE DETECTED" : "Normal");
  if (dataPacket.gpsFix) {
    Serial.print("Latitude    : "); Serial.println(dataPacket.latitude, 6);
    Serial.print("Longitude   : "); Serial.println(dataPacket.longitude, 6);
  } else {
    Serial.println("GPS         : No fix yet");
  }
  Serial.println(result == 0 ? "Send status : OK" : "Send status : ERROR");
  Serial.println("-----------------------------------");
}
