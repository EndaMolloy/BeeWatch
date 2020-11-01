#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <FirebaseArduino.h>
#define FIREBASE_HOST "beewatch-d4d0a.firebaseio.com"
//#define FIREBASE_AUTH "token_or_secret"
#define FIREBASE_AUTH "rTv3f4MOd0aV3F9RhMSJZjEFxKO1KObCogYbG6jS"
#include <ArduinoJson.h>
#include <SimpleTimer.h>

#include "DHT.h"
#define DHTPIN 4     // what digital pin we're connected to
#define DHTTYPE DHT11   // DHT 11
SimpleTimer timer;

// --------------------- Variables for sound level measuring ---------
// -------------------------------------------------------------------
const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;
double soundValue;
// -------------------------------------------------------------------


// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);


void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);

    WiFiManager wifiManager;
    //reset saved settings
   
    wifiManager.autoConnect("AutoConnectAP");
    
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    Firebase.begin(FIREBASE_HOST);
    dht.begin();

    // Wait a few seconds between measurements.
    delay(1000);
  
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
  
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    
    /**
     * Based on code from Adafruit: 
     * https://learn.adafruit.com/adafruit-microphone-amplifier-breakout/measuring-sound-levels
     * Reads values from A0
     */
       unsigned long startMillis= millis();  // Start of sample window
       unsigned int peakToPeak = 0;   // peak-to-peak level
     
       unsigned int signalMax = 0;
       unsigned int signalMin = 1024;
     
       // collect data for 50 mS
       while (millis() - startMillis < sampleWindow)
       {
          sample = analogRead(0);
          if (sample < 1024)  // toss out spurious readings
          {
             if (sample > signalMax)
             {
                signalMax = sample;  // save just the max levels
             }
             else if (sample < signalMin)
             {
                signalMin = sample;  // save just the min levels
             }
          }
       }
       peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
       
       // Source to below code: https://forum.arduino.cc/index.php?topic=432991.0
       double v = ((peakToPeak * 3.3) / 1024); //* 0.707;  // convert to volts
       //double first = log10(volts/0.00631)*20; // The microphone sensitivity is -44 ±2 so V RMS / PA is 0.00631
       //double second = first + 30; // Adjusted value to microphone sensitivity
   
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print("Sound: ");
    Serial.print(v);
    Serial.print(" V ");
  
  
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& sensorObject = jsonBuffer.createObject();
    JsonObject& sensorTime = sensorObject.createNestedObject("timestamp");
    sensorObject["temperature"] = t;
    sensorObject["humidity"] = h;
    sensorObject["sound"] = v;
    sensorTime[".sv"] = "timestamp";
          
    // append a new value to /hives
    Firebase.push("/hives/data", sensorObject);
  
    if (Firebase.failed()) {
      Serial.print("pushing /temperature failed:");
      Serial.println(Firebase.error());
      return;
    }

    // Now let's go in deep sleep for 30 seconds:
    ESP.deepSleep(30e6, WAKE_RF_DEFAULT); // 1st parameter is in µs!

}


void loop() {  
}
