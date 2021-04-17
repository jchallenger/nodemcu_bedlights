#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <FastLED.h>
#include <vector>
#include <string>

#include <LEDStrip.h>

#define DATA_PIN 4
#define NUM_LEDS 156

#define FPS_RATE 33.33333f

unsigned long lTime = millis();
unsigned long prevTime = millis();
unsigned long ticks = 0;

CRGB leds[NUM_LEDS];
LEDStrip * myLEDs;

const char* ssid = "HomeNet-24";
const char* password = "fourwordsalluppercase";

AsyncWebServer server(80);

void setup(void) {
  Serial.begin(9600);
  
  // Start LED
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);  // GRB ordering is typical
  myLEDs = new LEDStrip();

  // Wifi indicators
  for(int ii=0; ii<10; ii++){
    leds[ii] = CRGB(100,0,0);
    myLEDs->addLight(new led_animation(leds[ii]));
  }

  for(int ii=10; ii<20; ii++){
    leds[ii] = CRGB(0,0,250);
    myLEDs->addLight(new led_animation(leds[ii]));
  }
  for(int ii=20; ii<30; ii++){
    leds[ii] = CRGB(100,0,100);
    myLEDs->addLight(new led_animation(leds[ii], PLAYMODE::PING));
  }
  for(int ii=30; ii<40; ii++){
    leds[ii] = CRGB(0,100,100);
    led_animation * li = new led_animation(leds[ii], PLAYMODE::PLAY);
    li->setDuration(15*1000);
    myLEDs->addLight(li);
  }

  // animations..
  for(int ii=40; ii<NUM_LEDS; ii++){
    leds[ii] = CRGB(10,100,200);
    myLEDs->addLight(new led_animation_blink(leds[ii]));
  }

  FastLED.show();
  delay(500);
  for(int ii=10; ii<NUM_LEDS; ii++){
    leds[ii] = CRGB(0,50,50);
  }
  FastLED.show();
  delay(500);
  for(int ii=10; ii<NUM_LEDS; ii++){
    leds[ii] = CRGB(0,100,100);
  }
  FastLED.show();
  delay(500);
  for(int ii=10; ii<NUM_LEDS; ii++){
    leds[ii] = CRGB(10,10,10);
  }
  FastLED.show();
  delay(1000);
  


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    // Wifi indicators
    for(int ii=0; ii<10; ii++){
      leds[ii] = CRGB(100,0,0);
    }
    FastLED.show();
    delay(100);
    for(int ii=0; ii<10; ii++){
      leds[ii] = CRGB(10,0,0);
    }
    FastLED.show();
    delay(100);
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

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
    //Check if GET parameter exists
    if(request->hasParam("brightness")){
      AsyncWebParameter* p = request->getParam("brightness");
    }
    request->send(200, "text/plain", "Updates Complete!");
  });

  
  // Server with different default file
  server.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");


}


float lerp(float a, float b, float f) 
{
    return (a * (1.0f - f)) + (b * f);
}

void loop(void) {
  ticks++;
  AsyncElegantOTA.loop();
  unsigned long now = millis();
  myLEDs->update((now - prevTime));
  prevTime = now;

  if((now - lTime) >= FPS_RATE ){
    // Analog IN
    int sensorValue = analogRead(A0);
    Serial.print("now=");
    Serial.print(now);
    Serial.print("  sincelast= ");
    Serial.print(now - lTime);
    Serial.print("  upf= ");
    Serial.print(ticks);
    Serial.print("  LED[1]=");
    Serial.print(myLEDs->getLight(1)->getProgress());
    Serial.print("  LED[10]=");
    Serial.print(myLEDs->getLight(10)->getValue());
    Serial.print("  LED[100]=");
    Serial.print(myLEDs->getLight(100)->getValue());
    Serial.print("  LED[150]=");
    Serial.print(myLEDs->getLight(150)->getProgress());

    Serial.print("  sensor = ");
    Serial.println(sensorValue);

    // apply LEDs
    myLEDs->display();
    
    // Turn the LED on, then pause
    FastLED.show();
    ticks = 0;
    lTime = now;
  }
}
