#include <ESP8266WiFi.h>
#include <espnow.h>
#include <DHT.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// MAC Address of the Receiver NodeMCU
uint8_t receiverAddress[] = {0x50, 0x02, 0x91, 0xE1, 0x4A, 0x44};

// Pin Definitions
#define DHTPIN D2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const int mq2Pin = A0;
const int flamePin = D6;

// GPS Setup
static const int RXPin = D1, TXPin = D0;
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

// Structure to send data
typedef struct struct_message {
  float temp;
  float hum;
  int gasValue;
  int flameState;
  float latitude;
  float longitude;
} struct_message;

struct_message sendData;

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0) {
    Serial.println("Delivery Success");
  } else {
    Serial.println("Delivery Fail");
  }
}

void setup() {
  Serial.begin(115200);
  ss.begin(GPSBaud);
  dht.begin();

  pinMode(flamePin, INPUT);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);

  esp_now_add_peer(receiverAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}

void loop() {
  sendData.temp = dht.readTemperature();
  sendData.hum = dht.readHumidity();

  sendData.gasValue = analogRead(mq2Pin);
  sendData.flameState = digitalRead(flamePin);

  while (ss.available() > 0) {
    gps.encode(ss.read());
  }

  if (gps.location.isUpdated()) {
    sendData.latitude = gps.location.lat();
    sendData.longitude = gps.location.lng();
  } else {
    if (sendData.latitude == 0.0) sendData.latitude = 0.0;
    if (sendData.longitude == 0.0) sendData.longitude = 0.0;
  }

  esp_now_send(receiverAddress, (uint8_t *) &sendData, sizeof(sendData));

  delay(5000);
}
