#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <my_config.h>

WiFiClientSecure client;  // or WiFiClientSecure for HTTPS
HTTPClient http;

void setup() {

  client.setInsecure();
  client.setTimeout(10000);

  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  delay(100);

  Serial.print("Attempting to connect to SSID: ");
  Serial.println(DEFAULT_SSID);
  WiFi.begin(DEFAULT_SSID, DEFAULT_PASS);

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    // wait 1 second for re-trying
    delay(1000);
  }

  Serial.println();

  Serial.print("Connected to ");
  Serial.println(DEFAULT_SSID);

}

void loop() {

  http.useHTTP10(true);

  http.addHeader("Square-Version", "2023-05-17");
  http.addHeader("Authorization", SQUARE_AUTH);
  http.addHeader("Content-Type", "application/json");

  http.begin(client, "https://connect.squareupsandbox.com/v2/payments?limit=10");
  http.GET();

  // Parse response
  DynamicJsonDocument doc(30000);
  // deserializeJson(doc, http.getStream());
  ReadLoggingStream loggingStream(http.getStream(), Serial);
  deserializeJson(doc, loggingStream);

  Serial.println();

  Serial.println("Extracting transactions:\n");

  // Read values
  Serial.print("Tranaction ID: ");
  Serial.println(doc["payments"][0]["id"].as<String>());
  Serial.print("Timestamp    : ");
  Serial.println(doc["payments"][0]["created_at"].as<String>());
  Serial.print("Amount       : $");
  Serial.println(doc["payments"][0]["amount_money"]["amount"].as<float>()/100);
  Serial.print("Status       : ");
  Serial.println(doc["payments"][0]["status"].as<String>());

  Serial.println();

  Serial.print("Tranaction ID: ");
  Serial.println(doc["payments"][1]["id"].as<String>());
  Serial.print("Timestamp    : ");
  Serial.println(doc["payments"][1]["created_at"].as<String>());
  Serial.print("Amount       : $");
  Serial.println(doc["payments"][1]["amount_money"]["amount"].as<float>()/100);
  Serial.print("Status       : ");
  Serial.println(doc["payments"][1]["status"].as<String>());

  // Disconnect
  http.end();

  delay(1000);

}
