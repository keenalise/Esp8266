#include <ESP8266WiFi.h>
#include <espnow.h>

typedef struct struct_message {
  float temp;
  float hum;
  int gasValue;
  int flameState;
  float latitude;
  float longitude;
} struct_message;

struct_message incomingData;

void OnDataRecv(uint8_t *mac, uint8_t *incomingDataBytes, uint8_t len) {
  memcpy(&incomingData, incomingDataBytes, sizeof(incomingData));

  Serial.println("--- Received Fire Sensor Data ---");
  Serial.print("Temperature: "); Serial.print(incomingData.temp); Serial.println(" °C");
  Serial.print("Humidity: "); Serial.print(incomingData.hum); Serial.println(" %");
  Serial.print("Gas Value: "); Serial.println(incomingData.gasValue);
  Serial.print("Flame Detected (0=Fire, 1=Clear): "); Serial.println(incomingData.flameState);
  Serial.print("Latitude: "); Serial.println(incomingData.latitude, 6);
  Serial.print("Longitude: "); Serial.println(incomingData.longitude, 6);
  Serial.println("---------------------------------");
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // handled via callback
}
