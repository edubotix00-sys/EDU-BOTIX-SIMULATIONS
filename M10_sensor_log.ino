// M10: measure colored paper with TCS34725 before deciding thresholds.
// Wiring: VCC/VIN->3V3, GND->GND, SDA->GPIO21, SCL->GPIO22.
// OLED may remain connected to the same I2C bus.
// Requires the "Adafruit TCS34725" library.
// Motor battery disconnected while uploading and measuring on the desk.

#include <Wire.h>
#include <Adafruit_TCS34725.h>

Adafruit_TCS34725 tcs(
  TCS34725_INTEGRATIONTIME_50MS,
  TCS34725_GAIN_4X
);

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  if (!tcs.begin()) {
    Serial.println("TCS34725 NOT FOUND: check VCC/GND/SDA/SCL");
    while (true) {
      delay(100);
    }
  }

  Serial.println("M10 COLOR MEASURE: put one paper under the sensor");
}

void loop() {
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);

  uint32_t sum = (uint32_t)r + (uint32_t)g + (uint32_t)b;

  if (sum == 0 || c == 0) {
    Serial.println("COLOR READ ERROR: check wires, distance and light");
    delay(150);
    return;
  }

  float redPct = 100.0f * r / sum;
  float greenPct = 100.0f * g / sum;
  float bluePct = 100.0f * b / sum;

  Serial.print("R=");
  Serial.print(redPct, 1);

  Serial.print("%  G=");
  Serial.print(greenPct, 1);

  Serial.print("%  B=");
  Serial.print(bluePct, 1);

  Serial.print("%  RAW: ");
  Serial.print(r);
  Serial.print(" / ");
  Serial.print(g);
  Serial.print(" / ");
  Serial.print(b);

  Serial.print("  C=");
  Serial.println(c);

  delay(150);
}
