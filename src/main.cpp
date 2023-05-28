#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <IotWebConf.h>
#include <my_config.h>
#include <time.h>
#include <Preferences.h>

Preferences preferences;

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

String last_id;

void setup() {

  preferences.begin("doompay", false); // false = R/W
  
  Serial.begin(115200);
  delay(100);

  iotWebConf.init();

  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  client.setInsecure();
  client.setTimeout(10000);

  last_id = preferences.getString("last_id","No saved last_id");

  Serial.print("Persistent stored last_id: ");
  Serial.println(last_id);

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

      setenv("TZ","GMT0",1);
      tzset();

      getLocalTime(&timeinfo);
      Serial.print("GMT time is now: ");
      Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");

      timeset = true;

      return;

    }

    getLocalTime(&timeinfo);
    
    char ts_char[50] = {0};

    timeinfo.tm_min = timeinfo.tm_min - 5;

    strftime(ts_char,sizeof(ts_char),"%Y-%m-%dT%H:%M:%SZ", &timeinfo);   // 2023-05-27T04:05:10.039Z

    // Serial.println("strftime output 5 minutes ago is " + String(ts_char));

    http.useHTTP10(true);

    http.addHeader("Square-Version", "2023-05-17");
    http.addHeader("Authorization", SQUARE_AUTH);
    http.addHeader("Content-Type", "application/json");

    String querystring = "https://connect.squareupsandbox.com/v2/payments?limit=2&begin_time="+String(ts_char);

    http.begin(client, querystring.c_str());

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

      http.end();

      // Serial.println("Last ID: " + last_id);

      if (doc["payments"][0]) {

        if (last_id != doc["payments"][0]["id"].as<String>()) {

          Serial.println();
          Serial.println("*** PLAY DOOMBALL! ***");
          Serial.println();

          Serial.print("Tranaction ID : ");
          Serial.println(doc["payments"][0]["id"].as<String>());
          Serial.print("Timestamp     : ");
          Serial.println(doc["payments"][0]["created_at"].as<String>());
          Serial.print("Amount        : $");
          Serial.println(doc["payments"][0]["amount_money"]["amount"].as<float>()/100);
          Serial.print("Status        : ");
          Serial.println(doc["payments"][0]["status"].as<String>());
          Serial.println();

          last_id = doc["payments"][0]["id"].as<String>();

          preferences.putString("last_id",last_id); // save in case of crash or reboot

          delay(10000);

        } else if (last_id == doc["payments"][0]["id"].as<String>()) {

          Serial.print("Already processed transaction: ");
          Serial.println(doc["payments"][0]["id"].as<String>());

          delay(500);

        }

      } else {

        Serial.println("No recent transactions in the last 5 minutes.");
        
        delay(500);

      }

    }

  }

}

/**
 * Handle web requests to "/" path.
 */
void handleRoot() {

  // -- Let IotWebConf test and handle captive portal requests.
  //
  if (iotWebConf.handleCaptivePortal()) {

    // -- Captive portal request were already served.
    return;
  
  }

  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>IotWebConf 01 Minimal</title></head><body>";
  s += "Go to <a href='config'>configure page</a> to change settings.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);

}