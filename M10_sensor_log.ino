// M10: measure colored paper before deciding any thresholds.
// TCS3200 at 3V3; S0=GND, S1=3V3 (2% frequency scaling).
// S2=GPIO19, S3=GPIO23, OUT=GPIO34, OE=GND.
// Motor battery disconnected; keep the wheels raised while uploading.

const int SENSOR_S2 = 19;
const int SENSOR_S3 = 23;
const int SENSOR_OUT = 34;
const unsigned long PULSE_TIMEOUT_US = 8000;

struct Reading {
  float redHz;
  float greenHz;
  float blueHz;
  bool valid;
};

unsigned long medianOfThree(unsigned long a, unsigned long b, unsigned long c) {
  if (a > b) { unsigned long t = a; a = b; b = t; }
  if (b > c) { unsigned long t = b; b = c; c = t; }
  if (a > b) { unsigned long t = a; a = b; b = t; }
  return b;
}

float readFrequency(bool s2, bool s3) {
  digitalWrite(SENSOR_S2, s2);
  digitalWrite(SENSOR_S3, s3);
  // Throw away one pulse after changing the sensor's color filter.
  if (!pulseIn(SENSOR_OUT, LOW, PULSE_TIMEOUT_US)) return -1;
  unsigned long a = pulseIn(SENSOR_OUT, LOW, PULSE_TIMEOUT_US);
  unsigned long b = pulseIn(SENSOR_OUT, LOW, PULSE_TIMEOUT_US);
  unsigned long c = pulseIn(SENSOR_OUT, LOW, PULSE_TIMEOUT_US);
  if (!a || !b || !c) return -1;
  // The output is approximately a 50% duty square wave.
  // frequency [Hz] = 1,000,000 / (2 * LOW time [us]).
  return 500000.0f / medianOfThree(a, b, c);
}

Reading readColor() {
  Reading value;
  value.redHz = readFrequency(LOW, LOW);
  value.blueHz = readFrequency(LOW, HIGH);
  value.greenHz = readFrequency(HIGH, HIGH);
  value.valid = value.redHz > 0 && value.greenHz > 0 && value.blueHz > 0;
  return value;
}

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_S2, OUTPUT);
  pinMode(SENSOR_S3, OUTPUT);
  pinMode(SENSOR_OUT, INPUT);
  Serial.println("M10 COLOR MEASURE: put one paper under the sensor");
}

void loop() {
  Reading v = readColor();
  if (!v.valid) {
    Serial.println("NO SIGNAL: check wires, distance and light");
    delay(150);
    return;
  }
  float sum = v.redHz + v.greenHz + v.blueHz;
  Serial.print("R="); Serial.print(v.redHz / sum * 100, 1);
  Serial.print("%  G="); Serial.print(v.greenHz / sum * 100, 1);
  Serial.print("%  B="); Serial.print(v.blueHz / sum * 100, 1);
  Serial.print("%  Hz: ");
  Serial.print(v.redHz, 0); Serial.print(" / ");
  Serial.print(v.greenHz, 0); Serial.print(" / ");
  Serial.println(v.blueHz, 0);
  delay(150);
}
