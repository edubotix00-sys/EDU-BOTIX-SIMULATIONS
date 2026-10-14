// EDU-BOTIX M05: gesture data collection (Arduino-ESP32 3.x)
// HC-SR04P (3.3V): VCC=3V3, GND=GND, TRIG=GPIO5, ECHO=GPIO18
// OLED SSD1306: SDA=GPIO21, SCL=GPIO22, VCC=3V3, GND=GND
// Button: GPIO4 <-> GND. Wheels must remain off the ground.
// Libraries: Adafruit GFX Library, Adafruit SSD1306

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int BUTTON_PIN = 4;
const int STBY_PIN = 13;
const int AIN1_PIN = 33;
const int AIN2_PIN = 25;
const int BIN1_PIN = 26;
const int BIN2_PIN = 27;
const int PWMA_PIN = 32;
const int PWMB_PIN = 14;

const int SAMPLE_COUNT = 32;
const int SAMPLE_INTERVAL_MS = 80;
float distances[SAMPLE_COUNT];
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

void motorsOff() {
  digitalWrite(STBY_PIN, LOW);
  digitalWrite(AIN1_PIN, LOW);
  digitalWrite(AIN2_PIN, LOW);
  digitalWrite(BIN1_PIN, LOW);
  digitalWrite(BIN2_PIN, LOW);
  digitalWrite(PWMA_PIN, LOW);
  digitalWrite(PWMB_PIN, LOW);
}

void show(const char* line1, const char* line2 = "") {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(2);
  oled.setCursor(0, 6);
  oled.println(line1);
  oled.setTextSize(1);
  oled.setCursor(0, 42);
  oled.println(line2);
  oled.display();
}

uint8_t findOledAddress() {
  const uint8_t candidates[] = {0x3C, 0x3D};
  for (uint8_t addr : candidates) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) return addr;
  }
  return 0;
}

float distanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(3);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long us = pulseIn(ECHO_PIN, HIGH, 18000UL);
  if (us == 0) return -1.0f;
  return us * 0.0343f / 2.0f;
}

bool buttonPress() {
  static bool prev = HIGH;
  static bool stable = HIGH;
  static unsigned long changed = 0;
  bool value = digitalRead(BUTTON_PIN);
  if (value != prev) changed = millis();
  prev = value;
  if (millis() - changed > 35 && value != stable) {
    stable = value;
    if (stable == LOW) return true;
  }
  return false;
}

void captureGesture() {
  motorsOff();
  show("READY", "Move only after countdown");
  Serial.println("Get ready. Start moving when OLED shows GO.");
  for (int n = 3; n >= 1; --n) {
    char count[6];
    snprintf(count, sizeof(count), "%d", n);
    show(count, "Hand 10-35 cm from sensor");
    delay(650);
  }
  show("GO!", "Recording 2.56 seconds");
  bool valid = true;
  unsigned long next = millis();
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    while ((long)(millis() - next) < 0) delay(1);
    float d = distanceCm();
    if (d < 5 || d > 100) valid = false;
    distances[i] = d;
    next += SAMPLE_INTERVAL_MS;
  }
  if (!valid) {
    show("RETRY", "Keep palm facing sensor");
    Serial.println("ERROR: measurement failed/out of range; repeat gesture.");
    return;
  }
  Serial.print("SAMPLE");
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    Serial.print(',');
    Serial.print(distances[i], 1);
  }
  Serial.println();
  show("DONE!", "Copy SAMPLE,... from Serial");
  Serial.println("Copy the entire SAMPLE,... line into M05 AI LAB.");
}

void setup() {
  pinMode(STBY_PIN, OUTPUT);
  pinMode(AIN1_PIN, OUTPUT);
  pinMode(AIN2_PIN, OUTPUT);
  pinMode(BIN1_PIN, OUTPUT);
  pinMode(BIN2_PIN, OUTPUT);
  pinMode(PWMA_PIN, OUTPUT);
  pinMode(PWMB_PIN, OUTPUT);
  motorsOff();

  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  Wire.begin(21, 22);

  uint8_t addr = findOledAddress();
  if (addr == 0 || !oled.begin(SSD1306_SWITCHCAPVCC, addr)) {
    Serial.println("OLED ERROR: check SDA=21 SCL=22 and power.");
    while (true) { motorsOff(); delay(100); }
  }
  show("M05 DATA", "Press button to capture");
  Serial.println("M05 READY: Press button, wait for GO, move hand.");
}

void loop() {
  motorsOff();
  if (buttonPress()) captureGesture();
  delay(10);
}
