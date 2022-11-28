#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define PIN_LED 2
#define CHN 0
#define FRQ 1000
#define PWM_BIT 8
#define PRESS_VAL 14
#define RELEASE_VAL 25

#define SDA 13
#define SCL 14

bool isProcessed = false;
LiquidCrystal_I2C lcd(0x27, 16, 2);
void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  // ledcSetup(CHN, FRQ, PWM_BIT);
  // ledcAttachPin(PIN_LED, CHN);

  Wire.begin(SDA, SCL);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Hello, world!");
}

void loop() {
  int touch = touchRead(T0);
  lcd.setCursor(0, 1);
  lcd.print("Sensor: ");
  lcd.print(touch);
  lcd.print(" ");

  // ledcWrite(CHN, (32 - touch) * 10);
  if (touch < PRESS_VAL) {
    if (!isProcessed) {
      isProcessed = true;
      Serial.println("Touch detected!");
      reverseGPIO(PIN_LED);
    }
  }

  if (touch > RELEASE_VAL) {
    if (isProcessed) {
      isProcessed = false;
      Serial.println("Released!");
    }
  }
}

void reverseGPIO(int pin) {
  digitalWrite(pin, !digitalRead(pin));
}
