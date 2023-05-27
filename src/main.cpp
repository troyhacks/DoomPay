#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <IotWebConf.h>
#include <my_config.h>
#include <time.h>

WiFiClientSecure client;
HTTPClient http;

void handleRoot();

DNSServer dnsServer;
WebServer server(80);

IotWebConf iotWebConf(DEFAULT_SSID, &dnsServer, &server, DEFAULT_PASS);

bool timeset = false;

struct tm timeinfo;
time_t epoch_ts;
struct tm ts = {0};

time_t curr_time = time(NULL);

void setup() {

  Serial.begin(115200);
  delay(100);

  iotWebConf.init();

  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  client.setInsecure();
  client.setTimeout(10000);

}

void loop() {

  iotWebConf.doLoop();

  if (iotWebConf.getState() == iotwebconf::OnLine) {

    if (!timeset) {

      Serial.println("Setting up NTP time");
      configTime(0, 0, "ca.pool.ntp.org");    // First connect to NTP server, with 0 TZ offset

      if(!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
      }

      Serial.println("Got the time from NTP");
      // Now we can set the real timezone

      setenv("TZ","EST5EDT,M3.2.0,M11.1.0",1);
      tzset();

      getLocalTime(&timeinfo);
      Serial.print("Local time is now: ");
      Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");

      timeset = true;

      return;

    }

    getLocalTime(&timeinfo);
    char ts_char[50] = {0};
    strftime(ts_char,sizeof(ts_char),"%Y-%m-%dT%H:%M:%S-05:00", &timeinfo);   // 2023-05-27T04:05:10.039Z
    Serial.println("strftime output is " + String(ts_char));

    http.useHTTP10(true);

    http.addHeader("Square-Version", "2023-05-17");
    http.addHeader("Authorization", SQUARE_AUTH);
    http.addHeader("Content-Type", "application/json");

    http.begin(client, "https://connect.squareupsandbox.com/v2/payments?limit=3&begin_time="+String(ts_char));

    if (http.GET()) {

      // Parse response
      DynamicJsonDocument doc(30000);

      // Use this for speed:
      //
      deserializeJson(doc, http.getStream());
      
      // ... or use this for debugging to see the JSON data:
      //
      // ReadLoggingStream loggingStream(http.getStream(), Serial);
      // deserializeJson(doc, loggingStream);

      Serial.println();

      Serial.println("Extracting transactions:\n");

      Serial.print("Tranaction ID: ");
      Serial.println(doc["payments"][0]["id"].as<String>());
      Serial.print("Timestamp    : ");
      Serial.println(doc["payments"][0]["created_at"].as<String>());

      Serial.print("My Timestamp : ");
      strptime(doc["payments"][0]["created_at"].as<const char*>(),"%Y-%m-%dT%H:%M:%S.%03dZ",&ts);
      epoch_ts = mktime(&ts)-3600*5;
      localtime_r(&epoch_ts, &ts);
      Serial.println(&ts, "%A, %B %d %Y %H:%M:%S");

      Serial.print("Amount       : $");
      Serial.println(doc["payments"][0]["amount_money"]["amount"].as<float>()/100);
      Serial.print("Status       : ");
      Serial.println(doc["payments"][0]["status"].as<String>());

      Serial.println();

      Serial.print("Tranaction ID: ");
      Serial.println(doc["payments"][1]["id"].as<String>());
      Serial.print("Timestamp    : ");
      Serial.println(doc["payments"][1]["created_at"].as<String>());

      Serial.print("My Timestamp : ");
      strptime(doc["payments"][1]["created_at"].as<const char*>(),"%Y-%m-%dT%H:%M:%S.%03dZ",&ts);
      epoch_ts = mktime(&ts)-3600*5;
      localtime_r(&epoch_ts, &ts);
      Serial.println(&ts, "%A, %B %d %Y %H:%M:%S");

      Serial.print("Amount       : $");
      Serial.println(doc["payments"][1]["amount_money"]["amount"].as<float>()/100);
      Serial.print("Status       : ");
      Serial.println(doc["payments"][1]["status"].as<String>());

      // Disconnect
      http.end();

      delay(10000);

    }

  }

}

/**
 * Handle web requests to "/" path.
 */
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>IotWebConf 01 Minimal</title></head><body>";
  s += "Go to <a href='config'>configure page</a> to change settings.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}