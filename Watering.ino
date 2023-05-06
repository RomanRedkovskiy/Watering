#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <time.h>

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_SOIL "SoilMoisture"
#define AWS_IOT_PUBLISH_WATER "WaterFullness"
#define AWS_IOT_SUBSCRIBE_WATERING "Watering"

WiFiClientSecure net;
BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);
PubSubClient client(net);

const int PUMP_PIN = 4; // The pin that controls the pump
int soilMoisture = 0;
int waterRemaining = 0;
unsigned long measurements_last_time = 0;
unsigned long watering_last_time = 0;
//equals 0 by default?


void setClock()
{
  // configTime is from time.h
  // 1) Connects to ntp server and
  // 2) Sets the internal time to current time in UTC  format asynchronously
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov", "by.pool.ntp.org"); // This function sets time in UTC  format, although we specify 3*3600 for local time, the result of time(nullptr) will be still in UTC
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  /**
     1682899200 == 01.05.23T00:00:00
     basically any close to real time date can be used here as soon we just need to distinguish current datetime from some small values (counted from controller's start in seconds)
  */
  while (now < 1682899200) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    Serial.println(now);
  }
  Serial.println("");
  // Perhaps gmtime_r(&now, &timeinfo) should be removed
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}


void connectAWS()
{
  char err_buf[256];
  Serial.println("Nice Coooock");
  Serial.println(THINGNAME);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }
  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);

  client.setServer(AWS_IOT_ENDPOINT, 8883);
  client.setCallback(messageHandler);

  Serial.println("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print("failed, rc=");
    Serial.println(client.state());
    net.getLastSSLError(err_buf, sizeof(err_buf));
    Serial.print("SSL error: ");
    Serial.println(err_buf);
    Serial.println(" try again in 0.1 seconds");
    delay(100);
  }

  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_WATERING);

  Serial.println("AWS IoT Connected!");
}

void publishMessage(const char* topic, int value) {
  StaticJsonDocument<100> doc;
  //doc["time"] = millis();
  doc["value"] = value;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  client.publish(topic, jsonBuffer);
}

StaticJsonDocument<256> payloadDoc;

void messageHandler(char *topic, byte *payload, unsigned int length) {
  Serial.println("Starting Handling message");
  deserializeJson(payloadDoc, (const byte*)payload, length);
  int interval = payloadDoc["interval"];
  const char* name = payloadDoc["name"];
  Serial.println("Starting watering interval");
  switchRelay();
  delay(interval * 1000);
  Serial.println("Finishing watering interval");
  switchRelay();
}


void switchRelay() {
  digitalWrite(PUMP_PIN, HIGH);
  delay(100);
  digitalWrite(PUMP_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  setClock();
  pinMode(PUMP_PIN, OUTPUT); // Set the pump pin as an output
  connectAWS();
  switchRelay();
}

void loop() {
  //  Serial.println("GG");
  if (!client.connected()) {
    connectAWS();
  }
  else if (millis() - measurements_last_time > 1000) {
    measurements_last_time = millis();
    soilMoisture = analogRead(A0);
    Serial.println(soilMoisture);
    publishMessage(AWS_IOT_PUBLISH_SOIL, soilMoisture);
    //    publishMessage(AWS_IOT_PUBLISH_WATER, 1);

    /* loop() is a PubSubClient library function that reads the receive and send buffers and processes any messages it finds.
      It allows the client to process incoming messages and send publish data while refreshing the connection */
    client.loop();
  }
}
