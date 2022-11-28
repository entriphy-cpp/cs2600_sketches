#include <stdlib.h>
#include <string.h>
#include <BluetoothSerial.h>
#include <UltrasonicSensor.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define RED_PIN   4
#define GREEN_PIN 2
#define BLUE_PIN  15
#define DIST_PIN  5

#define RED_CHN   0
#define GREEN_CHN 1
#define BLUE_CHN  2
#define DIST_CHN  3

#define FRQ 1000
#define PWM_BIT 8

#define SDA 13
#define SCL 14

#define TRIG_PIN 27
#define ECHO_PIN 26
#define MAX_DISTANCE 700
#define SOUND_VELOCITY 340
#define DISTANCE_TRIGGER 10
const float TIMEOUT = MAX_DISTANCE * 60;

char buffer[20];
static int bufCount = 0;
static bool isRainbow = false;
static int rainbowPos = 0;
static float distance = 0.00;

BluetoothSerial SerialBT;
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  // Setup RGB LED
  ledcSetup(RED_CHN,   FRQ, PWM_BIT);
  ledcSetup(GREEN_CHN, FRQ, PWM_BIT);
  ledcSetup(BLUE_CHN,  FRQ, PWM_BIT);
  ledcSetup(DIST_CHN,  FRQ, PWM_BIT);
  ledcAttachPin(RED_PIN,   RED_CHN);
  ledcAttachPin(GREEN_PIN, GREEN_CHN);
  ledcAttachPin(BLUE_PIN,  BLUE_CHN);
  ledcAttachPin(DIST_PIN,  DIST_CHN);
  
  // Set RGB LED color to white
  setColor(255, 255, 255);

  // Setup ultrasonic sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Setup LCD
  Wire.begin(SDA, SCL);
  lcd.init();
  lcd.backlight();
  
  // Print initial text
  lcd.setCursor(0, 0);
  lcd.print("Dist: 0.00 cm  ");
  lcd.setCursor(0, 1);
  lcd.printf("RGB:  #FFFFFF");

  SerialBT.begin("mfinerty-esp32");
  Serial.begin(115200);
  Serial.println("Device started!");
}

void loop() {
  while (SerialBT.available()) {
    buffer[bufCount] = SerialBT.read();
    bufCount++;
    delay(10);
  }

  if (bufCount > 0) {
    if (strncmp(buffer, "random", 6) == 0) {
      randomCmd();
      SerialBT.println("random: ok");
      isRainbow = false;
    } else if (strncmp(buffer, "rainbow", 7) == 0) {
      SerialBT.println("rainbow: ok");
      isRainbow = true;
    } else if (strncmp(buffer, "rgb", 3) == 0) {
      rgbCmd();
      SerialBT.println("rgb: ok");
      isRainbow = false;
    } else if (strncmp(buffer, "distance", 8) == 0) {
      SerialBT.printf("distance: %.2f cm\n", distance);
    } else {
      Serial.println(buffer);
      SerialBT.printf("unknown command: %s\n", buffer);
    }

    clearBuffer();
  }

  if (isRainbow) {
    long wheelValue = wheel(rainbowPos++);
    setColor((wheelValue >> 16) & 0xFF, (wheelValue >> 8) & 0xFF, (wheelValue >> 0) & 0xFF);
    if (rainbowPos == 256) {
      rainbowPos = 0;
    }
  }

  distance = getSonar();
  lcd.setCursor(0, 0);
  lcd.printf("Dist: %.2f cm  ", distance);
  if (distance < DISTANCE_TRIGGER) {
    ledcWrite(DIST_CHN, ((DISTANCE_TRIGGER - distance) / DISTANCE_TRIGGER) * 255.0);
  } else {
    ledcWrite(DIST_CHN, 0);
  }

  delay(10);
}

void clearBuffer() {
  bufCount = 0;
  memset(buffer, 0, 20);
}

void randomCmd() {
  Serial.println("random");
  setColor(random(0, 256), random(0, 256), random(0, 256));
}

void rgbCmd() {
  char *rgb = strtok(buffer, " ");

  char *red = strtok(NULL, " ");
  if (red == nullptr) {
    SerialBT.println("rgb: invalid red value");
    clearBuffer();
    return;
  }

  char *green = strtok(NULL, " ");
  if (green == nullptr) {
    SerialBT.println("rgb: invalid green value");
    clearBuffer();
    return;
  }

  char *blue = strtok(NULL, " ");
  if (blue == nullptr) {
    SerialBT.println("rgb: invalid blue value");
    clearBuffer();
    return;
  }

  char r = atoi(red);
  char g = atoi(green);
  char b = atoi(blue);
  Serial.printf("%s(%d, %d, %d)\n", rgb, r, g, b);
  setColor(r, g, b);
}

long wheel(int pos) {
  long wheelPos = pos % 0xFF;
  if (wheelPos < 85) {
    return ((255 - wheelPos * 3) << 16) | ((wheelPos * 3) << 8);
  } else if (wheelPos < 170) {
    wheelPos -= 85;
    return (((255 - wheelPos * 3) << 8) | (wheelPos * 3));
  } else {
    wheelPos -= 170;
    return ((wheelPos * 3) << 16 | (255 - wheelPos * 3));
  }
}

void setColor(byte r, byte g, byte b) {
  ledcWrite(RED_CHN,   255 - r);
  ledcWrite(GREEN_CHN, 255 - g);
  ledcWrite(BLUE_CHN,  255 - b);

  lcd.setCursor(0, 1);
  lcd.printf("RGB:  #%02X%02X%02X", r, g, b);
}

float getSonar() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long pingTime = pulseIn(ECHO_PIN, HIGH, TIMEOUT);
  float distance = (float)pingTime * SOUND_VELOCITY / 2 / 10000;
  return distance;
}