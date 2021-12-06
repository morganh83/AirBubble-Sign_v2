#include <Arduino.h>
#include "LittleFS.h"
#include <SPI.h>
#include "WiFiManager.h"
#include "webServer.h"
#include "updater.h"
#include "fetch.h"
#include "configManager.h"
#include "timeSync.h"
#include <MQTT.h>
#include <Adafruit_NeoPixel.h>

WiFiClient espClient;
MQTTClient client;

const char* mSub = "mqtt/subscribe";
const char* mPub = "mqtt/publish";
const char* mqttURL = "mqtt.maker2maker.com";
const char* userName = "your_username";
const char* mqttPass = "your_password";
const char* devName = "unique_name";
const uint16_t mqttPort = 1883;

int payloadVar = 0;

// Pins
#define PIN 12
#define NUMLED 32

Adafruit_NeoPixel strip(NUMLED, PIN, NEO_GRB + NEO_KHZ800);
//Functions
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void messageReceived(String &topic, String &payload) {
  Serial.println("Received: " + topic + " - " + payload);
  if (payload == "onCall") {
    colorWipe(strip.Color(255, 0, 0), NUMLED);
    strip.show();
    payloadVar = 1;
  }
  else if (payload == "busy") {
    colorWipe(strip.Color(255, 128, 0), NUMLED);
    strip.show();
    payloadVar = 2;
  }
  else if (payload == "clear") {
    colorWipe(strip.Color(0, 0, 0), NUMLED);
    strip.show();
    payloadVar = 3;
  }
}


void saveCallback() {
    Serial.println("EEPROM saved"); 
}

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.print("Connected!\n");
  Serial.print("IP address: ");
  Serial.print(WiFi.localIP());
  Serial.print("\n");
  Serial.print("MQTT connecting...");
  Serial.print(mqttURL);
  Serial.print("\n");
  Serial.print("Username: ");
  Serial.print(userName);
  Serial.print("\n");
  Serial.print("Password: ");
  Serial.print(mqttPass);
  Serial.print("\n");
  Serial.print("Subscribing to: ");
  Serial.print(mSub);
  Serial.print("\n");
  Serial.print("Publishing to: ");
  Serial.print(mPub);
  Serial.print("\n");
  while (!client.connect(devName, userName, mqttPass)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");
  client.subscribe(mSub);
}

void setup()
{
    Serial.begin(115200);
    LittleFS.begin();
    GUI.begin();
    configManager.begin();
    configManager.setConfigSaveCallback(saveCallback);
    WiFiManager.begin(configManager.data.projectName);
    timeSync.begin();
    client.begin(mqttURL, mqttPort, espClient);
    client.onMessage(messageReceived);
    strip.begin();
}

void loop()
{
  WiFiManager.loop();
  updater.loop();
  configManager.loop();
  client.loop();

  if (!WiFiManager.isCaptivePortal()) {
    if (!client.connected()) {
      connect();
    }
  }
  if (payloadVar == 1) {
    payloadVar = 0;
    colorWipe(strip.Color(255, 0, 0), NUMLED);
    strip.show();
    client.publish(mPub, "onCallAck");
    Serial.print("Sent: onCallAck\n");
  }
  else if (payloadVar == 2) {
    payloadVar = 0;
    colorWipe(strip.Color(255, 128, 0), NUMLED);
    strip.show();
    client.publish(mPub, "busyAck");
    Serial.print("Sent: busyAck\n");
  }
  else if (payloadVar == 3) {
    payloadVar = 0;
    colorWipe(strip.Color(0, 0, 0), NUMLED);
    strip.show();
    client.publish(mPub, "clearAck");
    Serial.print("Sent: clearAck\n");
  }
}
