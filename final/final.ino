#include <PubSubClient.h>
#include <WiFi.h>
#include <string.h>
#include "wifi_secrets.h"

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

// Ultrasonic sensor config
#define TRIG_PIN 27
#define ECHO_PIN 26
#define MAX_DISTANCE 700
#define SOUND_VELOCITY 340
#define DISTANCE_TRIGGER 10
const float TIMEOUT = MAX_DISTANCE * 60;

// MQTT constants
const char *mqtt_broker = "test.mosquitto.org";
const char *TOPIC       = "cs2600-esp32";
const int   mqtt_port   = 1883;

// Connection
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void setup() {
  // Setup serial
  Serial.begin(115200);

  // Setup RGB LED
  ledcSetup(RED_CHN,   FRQ, PWM_BIT);
  ledcSetup(GREEN_CHN, FRQ, PWM_BIT);
  ledcSetup(BLUE_CHN,  FRQ, PWM_BIT);
  ledcAttachPin(RED_PIN,   RED_CHN);
  ledcAttachPin(GREEN_PIN, GREEN_CHN);
  ledcAttachPin(BLUE_PIN,  BLUE_CHN);
  
  // Set RGB LED color to white
  setColor(255, 255, 255);

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.printf("Connecting to WiFi @ %s...", WIFI_SSID);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi!");

  // Connect to MQTT broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  String client_id = "cs2600-esp32-" + WiFi.macAddress(); // Create client ID
  while (!client.connected()) { // Loop until succesfully connected to broker
    Serial.printf("Connecting to %s as %s...\n", mqtt_broker, client_id.c_str());
    if (client.connect(client_id.c_str())) { // Attempt to connect to broker
        Serial.println("Connected to MQTT broker!");
        client.subscribe(TOPIC); // Subscribe to topic
        if (client.publish(TOPIC, "Hello from ESP32!")) { // Attempt to publish message
          // Success
          Serial.println("Published message!");
        } else {
          // Fail
          Serial.println("Failed to publish message.");
        }
    } else {
        Serial.printf("Failed to connect (%d); retrying...\n", client.state());
        delay(2000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  if (strcmp(topic, TOPIC) == 0) {
    switch (length) {
      case 3:
        setColor(payload[0], payload[1], payload[2]);
        break;
      // case 4:
      //   client.publish(TOPIC, getSonar());
      //   client.beginPublish(const char *topic, unsigned int plength, boolean retained)
      //   break;
    }
  }
  // Serial.print("Message arrived in topic: ");
  // Serial.println(topic);
  // Serial.print("Message length: ");
  // Serial.println(length);
  // Serial.print("Message: ");
  // for (int i = 0; i < length; i++) {
  //     Serial.print((char) payload[i]);
  // }
  // Serial.println();
  // Serial.println("-----------------------");
}

void loop() {
  client.loop();
  if (Serial.available() > 0) {
    // Send message if user entered one in Serial Monitor
    String str = Serial.readString();
    str.trim();
    client.publish(TOPIC, str.c_str());
  }
}

// From Chapter 5.2
void setColor(byte r, byte g, byte b) {
  ledcWrite(RED_CHN,   255 - r);
  ledcWrite(GREEN_CHN, 255 - g);
  ledcWrite(BLUE_CHN,  255 - b);
}

// From Chapter 21.1
int getSonar() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long pingTime = pulseIn(ECHO_PIN, HIGH, TIMEOUT);
  float distance = (float)pingTime * SOUND_VELOCITY / 2 / 10000;
  return (int)distance;
}