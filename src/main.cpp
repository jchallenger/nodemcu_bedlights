#include <Arduino.h>
#include <AsyncElegantOTA.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FastLED.h>
#include <string>
#include <vector>

#include <LEDStrip.h>

#define DATA_PIN 4
#define NUM_LEDS 156

#define FPS_RATE 33.33333f

unsigned long lTime = millis();
unsigned long prevTime = millis();
unsigned long ticks = 0;

CRGB leds[NUM_LEDS];
LEDStrip *myLEDs;

WiFiEventHandler mConnectHandler, mDisConnectHandler, mGotIpHandler;
const char *ssid = "HomeNet-24";
const char *password = "fourwordsalluppercase";
int reqConnect = 0;
int isConnected = 0;
const long reqConnectNum = 15; // number of intervals to wait for connection

AsyncWebServer server(80);

void onConnected(const WiFiEventStationModeConnected &event) {
  Serial.println("Connected to AP.");
  isConnected = 1;
}

void onDisconnect(const WiFiEventStationModeDisconnected &event) {
  Serial.println("WiFi On Disconnect.");
  isConnected = 0;
}

void onGotIP(const WiFiEventStationModeGotIP &event) {
  Serial.print("Got Ip: ");
  Serial.println(WiFi.localIP());
  isConnected = 2;
}

void setup(void) {
  Serial.begin(9600);

  // Start LED
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds,
                                          NUM_LEDS); // GRB ordering is typical
  myLEDs = new LEDStrip();

  // Wifi indicators
  for (int ii = 0; ii < 20; ii++) {
    leds[ii] = CRGB(0, 200, 50);
    myLEDs->addLight(new led_animation(leds[ii]));
  }
  for (int ii = 20; ii < NUM_LEDS; ii++) {
    leds[ii] = CRGB(100, 0, 100);
    led_animation_blink *li = new led_animation_blink(leds[ii], PLAYMODE::PING);
    li->setColorRange(CRGB(200, 0, 200), CRGB(0, 50, 0));
    li->setDuration(5 * 1000);
    myLEDs->addLight(li);
  }
  FastLED.show();
  delay(500);

  // Wifi init
  WiFi.disconnect();
  WiFi.persistent(false);
  mConnectHandler = WiFi.onStationModeConnected(onConnected);
  mDisConnectHandler = WiFi.onStationModeDisconnected(onDisconnect);
  mGotIpHandler = WiFi.onStationModeGotIP(onGotIP);

  // HTTP Server
  server.on("/r", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Switching to Red");
  });
  server.on("/g", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Switching to Green");
  });
  server.on("/b", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Switching to Blue");
  });
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Switching to Off");
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Check if GET parameter exists
    if (request->hasParam("brightness")) {
      AsyncWebParameter *p = request->getParam("brightness");
    }
    request->send(200, "text/plain", "Updates Complete!");
  });

  // Server with different default file
  server.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");

  AsyncElegantOTA.begin(&server); // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  ticks++;
  AsyncElegantOTA.loop();
  unsigned long now = millis();
  myLEDs->update(now - prevTime);
  prevTime = now;

  if ((now - lTime) >= FPS_RATE) {
    reqConnect++;

    if (WiFi.status() != WL_CONNECTED && reqConnect > reqConnectNum &&
        isConnected < 2) {
      WiFi.begin(ssid, password);
      Serial.print(".");
    }

    if (isConnected < 2) {
    }

    // Analog IN
    int sensorValue = analogRead(A0);
    Serial.print((String) "now=" + now + "  sincelast= " + (now - lTime) +
                 "  upf= " + ticks);
    Serial.print((String) "  LED[50]=" + myLEDs->getLight(50)->getValue() +
                 " - " + myLEDs->getLight(50)->getProgress());
    /*
    Serial.print("  LED[1]=");
    Serial.print(myLEDs->getLight(1)->getProgress());
    Serial.print("  LED[10]=");
    Serial.print(myLEDs->getLight(10)->getValue());
    Serial.print("  LED[100]=");
    Serial.print(myLEDs->getLight(100)->getValue());
    Serial.print("  LED[150]=");
    Serial.print(myLEDs->getLight(150)->getProgress());
    */
    Serial.println((String) "  sensor = " + sensorValue);

    // set Brightness
    myLEDs->setBrightness(sensorValue / 1024.f);
    // apply LEDs
    myLEDs->display();

    // Turn the LED on, then pause
    FastLED.show();
    ticks = 0;
    lTime = now;
  }
}
