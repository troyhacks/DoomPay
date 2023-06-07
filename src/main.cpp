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

struct tm timeinfo;
char ts_char[50] = {0};
String last_id;
bool timeset = false;
float money_collected = 0.0;
float last_money_collected = 0.0;
uint16_t games_played = 0;
String last_game_gmt;

void setup() {

  preferences.begin("doompay", false);
  
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

  // Adapt this loop as required. This is checking payments constantly in a loop with delay()
  // In an installation, this may be better as a separate loop called from the main loop
  // without delays, using millis counted to trigger, etc.

  iotWebConf.doLoop(); // may NOT want to do this if the gameplay is currently running? Do test this.

  if (iotWebConf.getState() == iotwebconf::OnLine) {

    if (!timeset) {

      Serial.println("Setting up NTP time");
      configTime(0, 0, "ca.pool.ntp.org");    // First connect to NTP server, with 0 TZ offset

      if(!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
      }

      Serial.println("Got the time from NTP");

      setenv("TZ","GMT0",1);
      tzset();

      getLocalTime(&timeinfo);

      Serial.print("GMT time is now: ");
      Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z");

      timeset = true;

      return;

    }

    getLocalTime(&timeinfo);
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

      DynamicJsonDocument doc(30000);

      // Use this for speed:
      //
      deserializeJson(doc, http.getStream());
    
      // ... or use this for debugging to see the JSON data:
      //
      // ReadLoggingStream loggingStream(http.getStream(), Serial);
      // deserializeJson(doc, loggingStream);

      http.end();

      if (doc["payments"][0]) {

        if (last_id != doc["payments"][0]["id"].as<String>()) {

          // Put whatever trgger you want in this if() block, likely after the debug.

          Serial.println();
          Serial.println("*** PLAY DOOMBALL! ***");
          Serial.println();

          games_played += 1;

          Serial.print("Tranaction ID : ");
          Serial.println(doc["payments"][0]["id"].as<String>());

          Serial.print("Timestamp     : ");
          last_game_gmt = doc["payments"][0]["created_at"].as<String>();
          Serial.println(last_game_gmt);

          Serial.print("Amount        : $");
          last_money_collected = doc["payments"][0]["amount_money"]["amount"].as<float>()/100;
          Serial.println(last_money_collected);
          money_collected += last_money_collected;

          Serial.print("Status        : ");
          Serial.println(doc["payments"][0]["status"].as<String>());
          Serial.println();

          last_id = doc["payments"][0]["id"].as<String>();

          preferences.putString("last_id",last_id); // save in case of crash or reboot

          delay(10000); // Adapt this as required, may not be needed if we're heading off to "playgame()"

        } else if (last_id == doc["payments"][0]["id"].as<String>()) {

          Serial.print("Already processed transaction: ");
          Serial.println(doc["payments"][0]["id"].as<String>());

          delay(500); // Adapt this as required. May be a better as "has X millis passed, then run" in the overall loop

        }

      } else {

        Serial.println("No recent transactions in the last 5 minutes.");
        
        delay(500); // Adapt this as required. May be a better as "has X millis passed, then run" in the overall loop

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

  getLocalTime(&timeinfo);
  strftime(ts_char,sizeof(ts_char),"%Y-%m-%dT%H:%M:%SZ", &timeinfo);   // 2023-05-27T04:05:10.039Z

  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>DoomPay</title></head><body>";
  s += "<h1>DoomPay Info</h1>";
  s += "<table border=1 cellspacing=0 cellpadding=3>";
  s += "<tr><td><b>Games Played Since Boot</b></td><td>"+String(games_played)+"</td></tr>";
  s += "<tr><td><b>Money Collected Since Boot</b></td><td>$"+String(money_collected)+"</td></tr>";
  s += "<tr><td><b>Last Money Collected</b></td><td>$"+String(last_money_collected)+"</td></tr>";
  s += "<tr><td><b>Last Transacation ID</b></td><td>"+last_id+"</td></tr>";
  s += "<tr><td><b>Last Transaction GMT</b></td><td>"+last_game_gmt+"</td></tr>";
  s += "<tr><td><b>Current Time in GMT</b></td><td>"+String(ts_char)+"</td></tr>";
  s += "</table>";
  s += "<p>Go to <a href='config'>configure page</a> to change settings.</p>";
  s += "</body></html>\n";

  server.send(200, "text/html", s);

}