#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <UltrasonicSensor.h>

#define PIN_LED 5
#define CHN 0
#define FRQ 1000
#define PWM_BIT 8
#define PRESS_VAL 14
#define RELEASE_VAL 25

#define SDA 13
#define SCL 14

#define TRIG_PIN 27
#define ECHO_PIN 26
#define MAX_DISTANCE 700
#define SOUND_VELOCITY 340
const float TIMEOUT = MAX_DISTANCE * 60;

bool isProcessed = false;
LiquidCrystal_I2C lcd(0x27, 16, 2);
void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  ledcSetup(CHN, FRQ, PWM_BIT);
  ledcAttachPin(PIN_LED, CHN);

  Wire.begin(SDA, SCL);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Distance:");
}

void loop() {
  int touch = touchRead(T0);
  float sonar = getSonar();
  lcd.setCursor(0, 1);
  lcd.print(sonar);
  lcd.print(" cm    ");
  if (sonar < 20) {
    ledcWrite(CHN, ((20.0 - sonar) / 20.0) * 255.0);
  } else {
    ledcWrite(CHN, 0);
  }
  
  delay(40);
}

void reverseGPIO(int pin) {
  digitalWrite(pin, !digitalRead(pin));
}

float getSonar() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long pingTime = pulseIn(ECHO_PIN, HIGH, TIMEOUT);
  float distance = (float)pingTime * SOUND_VELOCITY / 2 / 10000;
  return distance;
}
