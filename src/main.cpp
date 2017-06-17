#include <ArduinoOTA.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
extern "C" {
  #include "user_interface.h"
}

//only define const char *ssid & const char *password in env.h
#include "env.h"

// static const uint8_t D0   = 16;
// static const uint8_t D1   = 5;
// static const uint8_t D2   = 4;
// static const uint8_t D3   = 0;
// static const uint8_t D4   = 2;
// static const uint8_t D5   = 14;
// static const uint8_t D6   = 12;
// static const uint8_t D7   = 13;
// static const uint8_t D8   = 15;
// static const uint8_t RX   = 3;
// static const uint8_t TX   = 1;

// Chip name
String chipName = "three";

// Deep sleep time
int sleepTime = 590000000;
// int sleepTime = 5000000;

// DHT sensor settings
#define DHTPIN 5     // what digital pin we're connected to
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

// Host
char *nasHost = "10.0.0.2";
const int httpPort = 88;
const char *url = "/sensor/upload";

// DHT variables
float humidity = 0.00;
float temperature = 0.00;

// void OLEDDisplay2Ctl();
void DHTSenserPost();
void DHTSenserUpdate();
String getSensorsJson();

int resetStatus = -1;

void setup() {
  // connect wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.waitForConnectResult();

  dht.begin();
  rst_info *resetInfo;
  resetInfo = ESP.getResetInfoPtr();
  resetStatus = resetInfo->reason;

  //OAT
  if(resetInfo->reason == REASON_DEFAULT_RST || resetInfo->reason == REASON_EXT_SYS_RST){
    ArduinoOTA.begin();
    for(int whileCount = 0;whileCount < 150; ++whileCount){
        ArduinoOTA.handle();
        delay(100);
    }

    ESP.restart();
  }

  DHTSenserUpdate();
  DHTSenserPost();
  ESP.deepSleep(sleepTime);
}

void loop() {
}

void DHTSenserUpdate() {
  double localHumidity = dht.readHumidity();
  double localTemperature = dht.readTemperature();

  if (isnan(localHumidity) || isnan(localTemperature)) {
    return;
  }

  if (localHumidity != 0.00 || localTemperature != 0.00) {
    humidity = localHumidity - 5.0;
    temperature = localTemperature;
  }
}

void DHTSenserPost() {
  WiFiClient client;

  if (client.connect(nasHost, httpPort)) {

    String jsonStr = getSensorsJson();

    String httpBody = String("POST ") + String(url) + " HTTP/1.1\r\n" +
                      "Host: " + nasHost + "\r\n" + "Content-Length: " +
                      jsonStr.length() + "\r\n\r\n" + jsonStr;

    client.print(httpBody);
    delay(10);

    // Read all the lines of the reply from server and print them to Serial
    while (client.available()) {
      String line = client.readStringUntil('\r');
    }
  }
}

// sensors json
String getSensorsJson() {
  float h = humidity;
  float t = temperature;
  if (isnan(h)) {
    h = 0.00;
  }
  if (isnan(t)) {
    t = 0.00;
  }

  String res = "{\"temperature\": ";
  res += (String)t;
  res += ",\"humidity\": ";
  res += (String)h;
  res += ",\"chip\": \"";
  res += chipName;
  res += "\",\"ip\": \"";
  res += WiFi.localIP().toString();
  res += "\",\"reset\": \"";
  res += (String)resetStatus;
  res += "\"}";

  return res;
}
