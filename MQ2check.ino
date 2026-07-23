void setup() {
  Serial.begin(115200);
  Serial.println("MQ2 test starting...");
}

void loop() {
  int rawValue = analogRead(A0);       // ESP8266 ADC range: 0–1023
  float voltage = rawValue * (3.3 / 1023.0);

  Serial.print("Raw ADC: ");
  Serial.print(rawValue);
  Serial.print("   Voltage: ");
  Serial.print(voltage, 2);
  Serial.println(" V");

  delay(1000);
}
