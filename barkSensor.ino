#include <ArduinoWiFiServer.h>
#include <BearSSLHelpers.h>
#include <CertStoreBearSSL.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiGratuitous.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiServer.h>
#include <WiFiServerSecure.h>
#include <WiFiServerSecureBearSSL.h>
#include <WiFiUdp.h>

#include<MegunoLink.h>;
#include<Filter.h>;
#include<PubSubClient.h>;
#include<Wire.h>;

#include <ArduinoJson.h>

// Wifi settings
char ssid[] = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// MQTT Broker settings
const char* mqtt_server = "YOUR_MQTT_SERVER_IP";
const char* mqtt_user = "YOUR_MQTT_USER";
const char* mqtt_password = "YOUR_MQTT_PASSWORD";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
int peak = 0;
int threshold = 145;
int peaks[5] = {0, 0, 0, 0, 0};
int idx = 0;
bool isBarking = false;

bool debug = false;

#define MIC_PIN A0
#define NOISE 550

ExponentialFilter<long> ADCFilter(5,0);

void setup_wifi() {
  // Connect to network
  Serial.print("Connecting to WiFi ..");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  // baud for my board is 9600
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == "bark/debug") {
    Serial.print("Changing debug to ");
    if(messageTemp == "on"){
      Serial.println("on");
      debug = true;
    }
    else if(messageTemp == "off"){
      Serial.println("off");
      debug = false;
    }
  }
  if (String(topic) == "bark/setthreshold") {
    Serial.print("Changing threshold value to ");
    Serial.println(messageTemp.toInt());
    threshold = messageTemp.toInt();
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("bark/setthreshold");
      client.subscribe("bark/debug");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  int n, lvl;
  n = analogRead(MIC_PIN);
  n = abs(1023 - n);
  n = (n <= NOISE) ? 0 : abs(n - NOISE);
  ADCFilter.Filter(n);
  lvl = ADCFilter.Current();

  peak = max(peak, lvl);

  unsigned long now = millis();

  if (now - lastMsg > 10000) {
    lastMsg = now;

    peaks[idx] = peak;
    idx++;
    if (idx == 4) {
      idx = 0;
    }

    int bark = 0;
    
    for (int i=0; i <= 4; i++) {
      if (peaks[i] >= threshold) {
        bark++;
      }
    }
    if (!isBarking && bark >= 3) {
      isBarking = true;
      client.publish("bark/dog", "bark");
    }
    if (isBarking && bark < 1) {
      isBarking = false;
      client.publish("bark/dog", "silence");
    }
    
    // Convert value to char array
    char lvlString[8];
    dtostrf(peak, 1, 2, lvlString);
    if (debug) {
      StaticJsonBuffer<300> JSONbuffer;
      JsonObject& JSONencoder = JSONbuffer.createObject();
      JSONencoder["peak"] = lvlString;
      JSONencoder["threshold"] = threshold;
      char JSONmessageBuffer[100];
      JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
      client.publish("bark/dog", JSONmessageBuffer);
    }
    peak = 0;

  }

  delay(10);
}
