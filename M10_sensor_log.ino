// M10: TCS34725で色紙を安定して測る。
// 配線：VCC/VIN→3V3、GND→GND、SDA→GPIO21、SCL→GPIO22。
// OLEDは同じI2CのSDA/SCLへつないだままでOK。
// Arduino IDEに「Adafruit TCS34725」ライブラリが必要。
// モーター用電池は外して測定する。

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
    while (true) delay(100);
  }

  Serial.println("M10 COLOR MEASURE");
  Serial.println("Measure in this order: WHITE -> RED -> BLUE -> YELLOW");
}

void loop() {
  uint32_t rs = 0, gs = 0, bs = 0, cs = 0;

  // 5回の平均を使って、数字のゆれを小さくする
  for (int i = 0; i < 5; i++) {
    uint16_t r, g, b, c;
    tcs.getRawData(&r, &g, &b, &c);
    rs += r;
    gs += g;
    bs += b;
    cs += c;
  }

  float r = rs / 5.0f;
  float g = gs / 5.0f;
  float b = bs / 5.0f;
  float c = cs / 5.0f;

  float sum = r + g + b;

  if (sum <= 0 || c <= 0) {
    Serial.println("COLOR READ ERROR: check wires, distance and light");
    delay(300);
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
  Serial.print(r, 0);
  Serial.print(" / ");
  Serial.print(g, 0);
  Serial.print(" / ");
  Serial.print(b, 0);

  Serial.print("  C=");
  Serial.println(c, 0);

  delay(350);
}
