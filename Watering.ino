#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
//#include <ArduinoJson.h>
#include "ESP8266WiFi.h"

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_SOIL "SoilMoisture"
#define AWS_IOT_PUBLISH_WATER "WaterFullness"
#define AWS_IOT_SUBSCRIBE_TOPIC "Watering"

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);
const int pumpPin = 9; // The pin that controls the pump
int soilMoisture = 0;
int waterRemaining = 0;
unsigned long lastTime = 0;
//equals 0 by default? 

void connectAWS()
{
  Serial.println("Nice Coooock");
  Serial.println(THINGNAME);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert((const uint8_t*)AWS_CERT_CA, sizeof(AWS_CERT_CA) - 1);
  net.setCertificate((const uint8_t*)AWS_CERT_CRT, sizeof(AWS_CERT_CRT) - 1);
  net.setPrivateKey((const uint8_t*)AWS_CERT_PRIVATE, sizeof(AWS_CERT_PRIVATE) - 1);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
//  client.onMessage(messageHandler);

  Serial.println("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

void publishMessage(String topic, int portNumber)
{
//  StaticJsonDocument<100> doc;
//  doc["time"] = millis();
//  doc["value"] = analogRead(portNumber);
//  char jsonBuffer[512];
//  serializeJson(doc, jsonBuffer);
  client.publish(topic, "1");
}

void messageHandler(String &topic, String &payload) {
  // MillisTimer timer(payload.toInt() * 1000); // The timer for the pump
  // timer.start(); // Start the timer
  digitalWrite(pumpPin, HIGH); // Turn on the pump
  delay(1000);
  //if (timer.isFinished()) { // If the timer has finished
  digitalWrite(pumpPin, LOW); // Turn off the pump
 // }
}

void setup() {
  delay(5000);
  Serial.begin(115200);
//  pinMode(pumpPin, OUTPUT); // Set the pump pin as an output
  connectAWS();
}

void loop() {
  Serial.print("GG");
  delay(1000);
  publishMessage(AWS_IOT_PUBLISH_SOIL, 0);
  publishMessage(AWS_IOT_PUBLISH_WATER, 1);
  /* loop() is a PubSubClient library function that reads the receive and send buffers and processes any messages it finds. 
   * It allows the client to process incoming messages and send publish data while refreshing the connection */
  client.loop();
}
