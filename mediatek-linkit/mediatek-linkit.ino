/*

# ShipIoT with Mediatek LinkIt ONE and bip.io Data Services

by Aaron Kondziela <aaron@aaronkondziela.com>

# Description

This is a demonstration of the Mediatek LinkIt ONE development
board, bip.io data services, and an HTTP webhook. The demo is
a motorcycle accident detector, with outputs to Twitter and a
Google spreadsheet.

# Usage

For usage and more information please visit:

* http://labs.wot.io/ship-iot-with-mediatek-linkit-one/
* https://shipiot.net
* https://github.com/wotio/shipiot-mediatek-linkit-one

# License

Copyright (c) 2015 wot.io, Inc. All Rights Reserved.

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

#include <LGPS.h> 
#include <LGPRS.h> 
#include <LGPRSClient.h> 

// Change these to match your own config
#define GPRS_APN     "fast.t-mobile.com"
#define API_HOSTNAME "your_hostname_here.api.shipiot.net"
#define AUTH_HEADER  "your_auth_header_here"

String request_template = F("POST /bip/http/mediatek HTTP/1.1\r\n"
                            "Host: " API_HOSTNAME "\r\n"
                            AUTH_HEADER "\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: ");
String data_template =    F("{\"x\":X,\"y\":Y,\"z\":Z,\"nmea\":\"NMEA\"}");
String request;
String data;
String nmea;
LGPRSClient c;
gpsSentenceInfoStruct gpsDataStruct;
char request_buf[512];


void waitForGPSFix() {
  byte i = 0;
  Serial.print("Getting GPS fix...");
  while (gpsDataStruct.GPGGA[43] != '1') { // 1 indicates a good fix
    LGPS.getData( &gpsDataStruct );
    delay(250);
    // toggle the red LED during GPS init...
    digitalWrite(1, i++ == 0 ? LOW : HIGH);
    i = (i > 1 ? 0 : i);
  }
  Serial.println("GPS locked.");
}


void setup() {
  Serial.begin(115200);
  Serial.println("Starting up...");
  
  pinMode(0,OUTPUT);
  pinMode(1,OUTPUT);
  pinMode(13,OUTPUT);
  
  // Turn on red LEDs, turn off green
  digitalWrite(0,LOW);
  digitalWrite(1,LOW);
  digitalWrite(13,LOW);
  
  Serial.print("Connecting to GPRS APN...");
  while (!LGPRS.attachGPRS(GPRS_APN, NULL, NULL)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Success");

  Serial.print("GPS Powering up...");
  LGPS.powerOn();
  delay(1000);
  Serial.println("done");
  
  // Phase 1 init complete,
  // so turn off first red LED
  digitalWrite(0,HIGH);
  
  waitForGPSFix();
  
  // Phase 2 init complete, LEDs to green
  digitalWrite(0,HIGH);
  digitalWrite(1,HIGH);
  digitalWrite(13,HIGH);
  
  Serial.println("Setup done");
}


void loop() { 
  Serial.println("Reading accelerometer...");
  int accel_x = analogRead(A0);
  int accel_y = analogRead(A1);
  int accel_z = analogRead(A2);
  Serial.print("x: ");    Serial.print(accel_x);
  Serial.print(" \ty: "); Serial.print(accel_y);
  Serial.print(" \tz: "); Serial.println(accel_z);
  
  Serial.print("Reading GPS: ");
  LGPS.getData( &gpsDataStruct );
  Serial.print( (char *)gpsDataStruct.GPGGA );
  
  // Format the HTTP API request template
  nmea = String( (char *)(gpsDataStruct.GPGGA) );
  nmea.trim();
  data = data_template;
  data.replace("X", String(accel_x,DEC) );
  data.replace("Y", String(accel_y,DEC) );
  data.replace("Z", String(accel_z,DEC) );
  data.replace("NMEA", nmea );
  request = request_template + data.length() + "\r\n\r\n" + data;
  request.toCharArray(request_buf,512);
  
  // Uncomment this to print the full request to debug console
  // Serial.print(request);
  
  Serial.print("Connecting to api... ");
  c.connect(API_HOSTNAME, 80);
  Serial.print("checkking connection.. ");
  if (c.connected()) {
    Serial.print("Sending...");
    c.write((uint8_t *)request_buf, strlen(request_buf));
  }
  
  Serial.print("waiting...");
  
  digitalWrite(13,LOW);
  delay(250);
  digitalWrite(13,HIGH);
  
  Serial.print("receiving...");
  
  // uncomment this to print the API response to debug console
  /*
  while (!c.available()) { 
    delay(1);
  }
  while (c.available()) {
    char output = c.read();
    Serial.print( output );
  }
  */

  Serial.println("Closing.");
  c.stop();
  
  delay(5000);
}
