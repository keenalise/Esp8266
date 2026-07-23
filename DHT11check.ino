#include <DHT.h>

#define DHTPIN D2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println("DHT11 test starting...");
  dht.begin();
}

void loop() {
  delay(2000);  // DHT11 needs at least 2s between reads

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();  // Celsius by default

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %  Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
}
