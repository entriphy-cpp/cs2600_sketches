#include <stdlib.h>
#include <string.h>
#include <BluetoothSerial.h>
#include <UltrasonicSensor.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// LED pins
#define RED_PIN   4
#define GREEN_PIN 2
#define BLUE_PIN  15
#define DIST_PIN  5

// LED PWM channels
#define RED_CHN   0
#define GREEN_CHN 1
#define BLUE_CHN  2
#define DIST_CHN  3

// PWM constants
#define FRQ 1000
#define PWM_BIT 8

// LCD pins
#define SDA 13
#define SCL 14

// Ultrasonic sensor config
#define TRIG_PIN 27
#define ECHO_PIN 26
#define MAX_DISTANCE 700
#define SOUND_VELOCITY 340
#define DISTANCE_TRIGGER 10
const float TIMEOUT = MAX_DISTANCE * 60;

char buffer[20]; // Bluetooth buffer
static int bufCount = 0; // Buffer size
static bool isRainbow = false; // Whether rainbow is active
static int rainbowPos = 0; // Color wheel position
static float distance = 0.00; // Sensor value from ultrasonic sensor

BluetoothSerial SerialBT;
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  // Setup RGB LED
  ledcSetup(RED_CHN,   FRQ, PWM_BIT);
  ledcSetup(GREEN_CHN, FRQ, PWM_BIT);
  ledcSetup(BLUE_CHN,  FRQ, PWM_BIT);
  ledcAttachPin(RED_PIN,   RED_CHN);
  ledcAttachPin(GREEN_PIN, GREEN_CHN);
  ledcAttachPin(BLUE_PIN,  BLUE_CHN);
  
  // Set RGB LED color to white
  setColor(255, 255, 255);

  // Setup ultrasonic sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Setup range LED
  ledcSetup(DIST_CHN, FRQ, PWM_BIT);
  ledcAttachPin(DIST_PIN, DIST_CHN);

  // Setup LCD
  Wire.begin(SDA, SCL);
  lcd.init();
  lcd.backlight();
  
  // Print initial text on LCD
  lcd.setCursor(0, 0);
  lcd.print("Dist: 0.00 cm  ");
  lcd.setCursor(0, 1);
  lcd.printf("RGB:  #FFFFFF");

  // Bluetooth serial setup
  SerialBT.begin("mfinerty-esp32");

  // PC serial setup
  Serial.begin(115200);
  Serial.println("Device started!");
}

void loop() {
  while (SerialBT.available()) {
    buffer[bufCount] = SerialBT.read();
    bufCount++; // Increase buffer size
    delay(10); // Add delay to prevent it from reading too fast
  }

  if (bufCount > 0) {
    if (strncmp(buffer, "random", 6) == 0) {
      // random command
      randomCmd();
      SerialBT.println("random: ok");
      isRainbow = false;
    } else if (strncmp(buffer, "rainbow", 7) == 0) {
      // rainbow command
      SerialBT.println("rainbow: ok");
      isRainbow = true;
    } else if (strncmp(buffer, "rgb", 3) == 0) {
      // rgb command
      rgbCmd();
      SerialBT.println("rgb: ok");
      isRainbow = false;
    } else if (strncmp(buffer, "distance", 8) == 0) {
      // distance command
      SerialBT.printf("distance: %.2f cm\n", distance);
    } else {
      // unknown command
      Serial.println(buffer);
      SerialBT.printf("unknown command: %s\n", buffer);
    }

    clearBuffer();
  }

  if (isRainbow) {
    long wheelValue = wheel(rainbowPos++);
    char red = (wheelValue >> 16) & 0xFF;
    char green = (wheelValue >> 8) & 0xFF;
    char blue = (wheelValue >> 0) & 0xFF;
    setColor(red, green, blue);

    if (rainbowPos == 256) {
      // Loop to beginning of color wheel
      rainbowPos = 0;
    }
  }

  // Get reading from ultrasonic sensor
  distance = getSonar();

  // Update distance text on LCD
  lcd.setCursor(0, 0);
  lcd.printf("Dist: %.2f cm  ", distance);

  // Change brightness of LED based on distance
  if (distance < DISTANCE_TRIGGER) {
    ledcWrite(DIST_CHN, ((DISTANCE_TRIGGER - distance) / DISTANCE_TRIGGER) * 255.0);
  } else {
    ledcWrite(DIST_CHN, 0);
  }

  // delay i guess
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

  // Get space-separated RGB values
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

  // Convert RGB values to bytes
  char r = atoi(red);
  char g = atoi(green);
  char b = atoi(blue);
  Serial.printf("%s(%d, %d, %d)\n", rgb, r, g, b);
  setColor(r, g, b);
}

// From Chapter 5.2
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

// From Chapter 5.2
void setColor(byte r, byte g, byte b) {
  ledcWrite(RED_CHN,   255 - r);
  ledcWrite(GREEN_CHN, 255 - g);
  ledcWrite(BLUE_CHN,  255 - b);

  // Update color text on LCD
  lcd.setCursor(0, 1);
  lcd.printf("RGB:  #%02X%02X%02X", r, g, b);
}

// From Chapter 21.1
float getSonar() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long pingTime = pulseIn(ECHO_PIN, HIGH, TIMEOUT);
  float distance = (float)pingTime * SOUND_VELOCITY / 2 / 10000;
  return distance;
}