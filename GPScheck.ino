#include <SoftwareSerial.h>
#include <TinyGPS++.h>

SoftwareSerial gpsSerial(D1, D0);  // RX, TX
TinyGPSPlus gps;

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);
  Serial.println("GPS TinyGPS++ test starting...");
}

void loop() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isUpdated()) {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Satellites: ");
    Serial.println(gps.satellites.value());
    Serial.print("Altitude (m): ");
    Serial.println(gps.altitude.meters());
  }

  // Print a "waiting" message every couple seconds if no fix yet
  static unsigned long lastPrint = 0;
  if (!gps.location.isValid() && millis() - lastPrint > 3000) {
    Serial.println("Waiting for GPS fix...");
    lastPrint = millis();
  }
}
