/*
  ============================================================
  SENSOR DIAGNOSTICS SKETCH
  ============================================================
  Purpose : Test DHT11, MQ2, Flame sensor, and GPS in isolation,
            with NO WiFi / ESP-NOW / SoftwareSerial interrupt
            competition, to determine whether sensor issues are
            hardware/wiring/power related or caused by the
            ESP-NOW radio stack.

  How to use:
  - Upload this sketch as-is (no changes needed).
  - Open Serial Monitor at 115200 baud.
  - Watch the readings for at least 1-2 minutes.
  - If DHT11 fails HERE too, the problem is wiring/power, not code.
  - If DHT11 works fine HERE, the problem is specifically caused
    by ESP-NOW/GPS running at the same time, and we will need a
    different strategy (e.g. reading DHT11 before enabling WiFi).
  ============================================================
*/

#include <DHT.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// ---------------- Pin Definitions (as wired) ----------------
#define DHTPIN      D2
#define DHTTYPE     DHT11
#define MQ2_PIN     A0
#define FLAME_PIN   D6
#define GPS_RX_PIN  D1
#define GPS_TX_PIN  D0

DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
TinyGPSPlus gps;

unsigned long lastReadTime = 0;
const unsigned long readInterval = 2500; // DHT11 needs at least ~1-2s between reads

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== Sensor Diagnostics Starting ===");

  dht.begin();
  pinMode(FLAME_PIN, INPUT);
  gpsSerial.begin(9600);

  Serial.println("Setup complete. Reading sensors every 2.5 seconds...");
  Serial.println();
}

void loop() {
  // Feed any available GPS bytes to the parser (non-blocking)
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  if (millis() - lastReadTime >= readInterval) {
    lastReadTime = millis();
    runDiagnostics();
  }
}

void runDiagnostics() {
  Serial.println("---------------------------------------------");

  // ---- DHT11 ----
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  Serial.print("[DHT11]   ");
  if (isnan(h) || isnan(t)) {
    Serial.println("READ FAILED (NaN returned)");
  } else {
    Serial.print("Temp: "); Serial.print(t); Serial.print(" C   ");
    Serial.print("Humidity: "); Serial.print(h); Serial.println(" %");
  }

  // ---- MQ2 ----
  int mq2Raw = analogRead(MQ2_PIN);
  Serial.print("[MQ2]     Raw ADC value: "); Serial.println(mq2Raw);

  // ---- Flame Sensor ----
  int flameRaw = digitalRead(FLAME_PIN);
  Serial.print("[Flame]   Raw digital pin state: "); Serial.println(flameRaw);
  Serial.println("          (Note which state this shows with NO flame present,");
  Serial.println("           and adjust the sensitivity potentiometer if needed.)");

  // ---- GPS ----
  Serial.print("[GPS]     Characters processed: ");
  Serial.print(gps.charsProcessed());
  Serial.print("   Satellites: ");
  Serial.print(gps.satellites.isValid() ? String(gps.satellites.value()) : "N/A");
  Serial.print("   Fix: ");
  if (gps.location.isValid()) {
    Serial.print("YES  Lat: "); Serial.print(gps.location.lat(), 6);
    Serial.print("  Lng: "); Serial.println(gps.location.lng(), 6);
  } else {
    Serial.println("NO");
  }

  Serial.println("---------------------------------------------");
  Serial.println();
}
