/*    _   _ _ _____ _    _              _____     _ _     ___ ___  _  __
     /_\ | | |_   _| |_ (_)_ _  __ _ __|_   _|_ _| | |__ / __|   \| |/ /
    / _ \| | | | | | ' \| | ' \/ _` (_-< | |/ _` | | / / \__ \ |) | ' <
   /_/ \_\_|_| |_| |_||_|_|_||_\__, /__/ |_|\__,_|_|_\_\ |___/___/|_|\_\
                               |___/

   Copyright 2018 AllThingsTalk

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ATT_IOT.h>
#include <Wire.h>
#include <SPI.h>  // required to have support for signed/unsigned long type.
#include <DHT.h>
#include <CborBuilder.h>
#include "MutichannelGasSensor.h"
#include "keys.h"

// Define mqtt endpoint
#define mqtt "api.allthingstalk.io"  // broker

#define DHTPIN 13       // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)


//Constructors
ATTDevice device(DEVICE_ID, DEVICE_TOKEN);
CborBuilder payload(device);
DHT dht(DHTPIN, DHTTYPE);

//Setup Communication

WiFiClient espClient;
PubSubClient pubSub(mqtt, 1883, espClient);

void setup()
{
 
  //Start the serial port for debugging
  Serial.begin(9600);  // Init serial link for debugging
  Serial.println("Init sensors!");

  //Power on the Grove Sensors
  pinMode(15, OUTPUT);
  digitalWrite(15, 1);

  //Start the DHT sensor
  dht.begin();

  // Start the Gas Sensor
  gas.begin(0x04);
  gas.powerOn();

  delay(1000); //Here, you should normally wait for 10minutes (600e6) for warming up the gas sensor to get reliable results

  Serial.println("ready to sample the sensors!");

  //Time to connect to the wifi
  setupWiFi(WIFI_SSID, WIFI_PASSWORD);

  // Connect to the DS IoT Cloud MQTT Broker
  Serial.println("Subscribe MQTT");

  while (!device.subscribe(pubSub)) // Subscribe to mqtt
  Serial.println("retrying");

}


void setupWiFi(const char* ssid, const char* password)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
  }
  Serial.println();
  Serial.println("WiFi connected");
}

unsigned long timer; 

void loop()
{
  unsigned long curTime = millis();
  if (curTime > (timer + 30000))    {                     // Sample each 30 seconds

     // sampling data
     
     Serial.println("sample DHT sensor");
     
     float temperature = dht.readTemperature(false, true);
     float humidity = dht.readHumidity(true);

     // Check if any reads failed and exit early (to try again).
     if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
        }
     
     Serial.println("sample gas sensor");
     
     float no2 = gas.measure_NO2();
     float co = gas.measure_CO();
     float nh3 = gas.measure_NH3();
     float c3h8 = gas.measure_C3H8();
     float c4H10 = gas.measure_C4H10();
     float ch4 = gas.measure_CH4();
     float h2 = gas.measure_H2();
     float c2h5oh = gas.measure_C2H5OH();

     // Check if any reads failed and exit early (to try again).
     if ((no2 < 0) || (co < 0) || (nh3 < 0) || (nh3 < 0) || (c4H10 < 0) || (ch4 < 0) || (h2 < 0) || (c2h5oh < 0)) {
        Serial.println("Failed to read from GAS sensor!");
        gas.factory_setting();
        return;
        }
     

  // Transmit the counter value to DS IoT Cloud
  
  payload.reset();
  payload.map(10); //number of assets that need to be decoded. If number is not correct, parsing in the platform will not be correct.
  payload.addNumber(temperature, "t");
  payload.addInteger(int(humidity), "h");
  payload.addNumber(no2, "n");
  payload.addNumber(co, "c");
  payload.addNumber(nh3, "nh");
  payload.addNumber(c3h8, "p");
  payload.addNumber(c4H10, "b");
  payload.addNumber(ch4, "m");
  payload.addNumber(h2, "hy");
  payload.addNumber(c2h5oh, "e");
  payload.send();
  
  Serial.println("Transmitted data");
  Serial.print("NO2: ");
  Serial.println(no2);
  Serial.print("CO: ");
  Serial.println(co);
  Serial.print("NH3: ");
  Serial.println(nh3);
  Serial.print("C3H8: ");
  Serial.println(c3h8);
  Serial.print("C4H10: ");
  Serial.println(c4H10);
  Serial.print("CCH4: ");
  Serial.println(ch4);
  Serial.print("H2: ");
  Serial.println(h2);
  Serial.print("C2H5OH: ");
  Serial.println(c2h5oh);        
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("Humidity: ");
  Serial.println(int(humidity));
  Serial.println("");

  
  Serial.print("time = ");Serial.println(curTime);
  timer = curTime;
  }
  device.process();
}


