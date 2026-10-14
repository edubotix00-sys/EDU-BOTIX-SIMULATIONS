// EDU-BOTIX M05 90-minute edition, Arduino-ESP32 Core 3.x
// HC-SR04P 3.3V: TRIG=5, ECHO=18. OLED I2C: SDA=21, SCL=22.
// Button: GPIO4 to GND. Motor driver: TB6612FNG.
// IMPORTANT: Keep the wheels OFF THE GROUND in this lesson.
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

const uint8_t PIN_TRIG=5, PIN_ECHO=18, PIN_BUTTON=4;
const uint8_t PIN_STBY=13, PIN_AIN1=33, PIN_AIN2=25, PIN_PWMA=32;
const uint8_t PIN_BIN1=26, PIN_BIN2=27, PIN_PWMB=14;
const int N=32, RAW_CAP=96;
const unsigned long PERIOD=70, QUIET_MS=700, MAX_MS=5000;
float rawBuffer[RAW_CAP];
Adafruit_SSD1306 screen(128,64,&Wire,-1);

void show(const char* top,const char* lower=""){
  screen.clearDisplay(); screen.setTextColor(SSD1306_WHITE);
  screen.setTextSize(2); screen.setCursor(0,4); screen.println(top);
  screen.setTextSize(1); screen.setCursor(0,42); screen.println(lower);
  screen.display();
}
uint8_t findOled(){
  const uint8_t addr[]={0x3C,0x3D};
  for(uint8_t x:addr){
    Wire.beginTransmission(x);
    if(Wire.endTransmission()==0)return x;
  }
  return 0;
}
void preparePins(){
  pinMode(PIN_TRIG,OUTPUT);digitalWrite(PIN_TRIG,LOW);
  pinMode(PIN_ECHO,INPUT);pinMode(PIN_BUTTON,INPUT_PULLUP);
  pinMode(PIN_STBY,OUTPUT);pinMode(PIN_AIN1,OUTPUT);
  pinMode(PIN_AIN2,OUTPUT);pinMode(PIN_BIN1,OUTPUT);
  pinMode(PIN_BIN2,OUTPUT);pinMode(PIN_PWMA,OUTPUT);pinMode(PIN_PWMB,OUTPUT);
  // Do not enable the motor driver while capturing gestures.
  digitalWrite(PIN_STBY,LOW);
  digitalWrite(PIN_AIN1,LOW);digitalWrite(PIN_AIN2,LOW);
  digitalWrite(PIN_BIN1,LOW);digitalWrite(PIN_BIN2,LOW);
}
float readCm(){
  digitalWrite(PIN_TRIG,LOW);delayMicroseconds(3);
  digitalWrite(PIN_TRIG,HIGH);delayMicroseconds(10);
  digitalWrite(PIN_TRIG,LOW);
  unsigned long t=pulseIn(PIN_ECHO,HIGH,18000UL);
  if(t==0)return -1;
  return float(t)*0.0343f/2.0f;
}
bool buttonPressed(){
  static bool previous=HIGH,stable=HIGH;
  static unsigned long changed=0;
  bool current=digitalRead(PIN_BUTTON);
  if(current!=previous)changed=millis();
  previous=current;
  if(millis()-changed>=35 && stable!=current){
    stable=current;
    return stable==LOW;
  }
  return false;
}
bool timedRead(unsigned long &next,float &d){
  // Sampling cadence never depends on the student's gesture duration.
  while((long)(millis()-next)<0)delay(1);
  d=readCm();
  next+=PERIOD;
  return d>=7.0f && d<=65.0f;
}
void resample(const float* input,int count,float output[N]){
  for(int k=0;k<N;k++){
    float pos=float(k)*(count-1)/(N-1);
    int a=int(pos);
    int b=(a+1<count)?a+1:a;
    float f=pos-a;
    output[k]=input[a]*(1-f)+input[b]*f;
  }
}
struct Capture {
  float values[N];
  unsigned long duration;
  const char* reason;
};
bool capture(Capture &c){
  c.reason="MEASURE ERROR";c.duration=0;
  show("READY","Keep palm still at 25-35cm");
  delay(650);
  for(int n=3;n>0;n--){
    char num[8];snprintf(num,sizeof(num),"%d",n);
    show(num,"Keep your hand still");delay(450);
  }
  show("WAIT","Move when you are ready");
  // Establish baseline from several stable samples.
  float baseRead[10];float lo=1000,hi=0;
  unsigned long next=millis();
  for(int i=0;i<10;i++){
    if(!timedRead(next,baseRead[i])){c.reason="SENSOR ERROR";return false;}
    if(baseRead[i]<lo)lo=baseRead[i];
    if(baseRead[i]>hi)hi=baseRead[i];
  }
  if(hi-lo>2.5f){c.reason="HOLD STILL";return false;}
  float base=0;
  for(float v:baseRead)base+=v;
  base/=10.0f;
  if(base<18 || base>43){c.reason="START 20-40cm";return false;}
  // Detect the first real movement; timeout if no gesture begins.
  int triggerCount=0;
  float prev[4]={base,base,base,base};
  unsigned long waitUntil=millis()+6000;
  while(millis()<waitUntil){
    float v;
    if(!timedRead(next,v)){c.reason="SENSOR ERROR";return false;}
    prev[0]=prev[1];prev[1]=prev[2];prev[2]=prev[3];prev[3]=v;
    if(fabsf(v-base)>=4.5f)triggerCount++;else triggerCount=0;
    if(triggerCount>=2)break;
  }
  if(triggerCount<2){c.reason="NO MOVEMENT";return false;}
  show("REC","Finish and hold at start");
  // Keep a little of the pre-trigger baseline, so the whole curve is captured.
  int n=4;
  for(int i=0;i<4;i++)rawBuffer[i]=prev[i];
  unsigned long began=millis();
  unsigned long returnedAt=0;
  float minValue=1000,maxValue=-1000;
  for(int i=0;i<n;i++){
    if(rawBuffer[i]<minValue)minValue=rawBuffer[i];
    if(rawBuffer[i]>maxValue)maxValue=rawBuffer[i];
  }
  bool completed=false;
  while(millis()-began<MAX_MS && n<RAW_CAP){
    float d;
    if(!timedRead(next,d)){c.reason="SENSOR ERROR";return false;}
    if(fabsf(d-rawBuffer[n-1])>19.0f){c.reason="SUDDEN JUMP";return false;}
    rawBuffer[n++]=d;
    if(d<minValue)minValue=d;
    if(d>maxValue)maxValue=d;
    unsigned long elapsed=millis()-began;
    if(elapsed>1000 && fabsf(d-base)<3.8f){
      if(returnedAt==0)returnedAt=millis();
      if(millis()-returnedAt>=QUIET_MS){
        completed=true;
        break;
      }
    }else returnedAt=0;
  }
  if(!completed){c.reason="NOT FINISHED";return false;}
  if(maxValue-minValue<7.0f){c.reason="MOVE BIGGER";return false;}
  if(n<12){c.reason="TOO SHORT";return false;}
  c.duration=millis()-began;
  if(c.duration<1000 || c.duration>MAX_MS){c.reason="BAD DURATION";return false;}
  resample(rawBuffer,n,c.values);
  c.reason="OK";return true;
}

uint8_t accepted=0;
void stopMotors(){
  digitalWrite(PIN_STBY,LOW);
  digitalWrite(PIN_PWMA,LOW); digitalWrite(PIN_PWMB,LOW);
  digitalWrite(PIN_AIN1,LOW);digitalWrite(PIN_AIN2,LOW);
  digitalWrite(PIN_BIN1,LOW);digitalWrite(PIN_BIN2,LOW);
}
const char* nextLabel(){return accepted<3?"DOUBLE":accepted<6?"HOLD":"TEST";}
void showReady(){
  char caption[40];
  if(accepted<6)snprintf(caption,sizeof(caption),"%s %d/3",nextLabel(),(accepted%3)+1);
  else snprintf(caption,sizeof(caption),"TEST (new gestures)");
  show("M05 DATA",caption);
}
void setup(){
  preparePins();stopMotors();
  Serial.begin(115200);
  Wire.begin(21,22);
  uint8_t addr=findOled();
  if(!addr || !screen.begin(SSD1306_SWITCHCAPVCC,addr)){
    Serial.println("OLED ERROR. Check SDA21 SCL22.");
    while(true){stopMotors();delay(100);}
  }
  showReady();
  Serial.println("READY: collect DOUBLE 3 times, HOLD 3 times. Afterward TEST.");
}
void loop(){
  stopMotors();
  if(buttonPressed()){
    Capture c;
    if(capture(c)){
      Serial.print("SAMPLE,");
      Serial.print(nextLabel());
      Serial.print(",");
      Serial.print(c.duration);
      for(int i=0;i<N;i++){Serial.print(",");Serial.print(c.values[i],2);}
      Serial.println();
      if(accepted<6)accepted++;
      show("SAVED!",nextLabel());
      delay(700);
    }else{
      Serial.print("RETRY: ");Serial.println(c.reason);
      show("RETRY",c.reason);
      delay(1150);
    }
    showReady();
  }
  delay(10);
}
