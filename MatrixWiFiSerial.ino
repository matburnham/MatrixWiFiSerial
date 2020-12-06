/*
   Copyright (c) 2015, Majenko Technologies
   Copyright (c) 2020, Mat Burnham (modifications)
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

 * * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

 * * Redistributions in binary form must reproduce the above copyright notice, this
     list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.

 * * Neither the name of Majenko Technologies nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <ESP8266WiFi.h>
#include <time.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#ifndef STASSID
#define STASSID "XXXXXXXXXXXXXX" // TODO: replace with valid credentials
#define STAPSK  "XXXXXXXXXXXXXXXX"
#endif

#define HOSTNAME "MatrixSign"
#define BUFFER_SIZE (1800)

const char *ssid = STASSID;
const char *password = STAPSK;
const char *hostname = HOSTNAME;

ESP8266WebServer server(80);

const int led = LED_BUILTIN;
time_t now;

void handleRoot() {
  digitalWrite(led, 0);
  char temp[BUFFER_SIZE];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf(temp, BUFFER_SIZE,
    "<html>\n\
      <head>\n\
        <title>Matrix sign</title>\n\
        <style>\n\
          body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\n\
        </style>\n\
      </head>\n\
      <body>\n\
        <h1>Matrix sign</h1>\n\
        <p>Date: %s</p>\n\
        <p>Uptime: %02d:%02d:%02d</p>\n\
        <form action='/msg' method='post'>\n\
          <label for='fname'>Message:</label><br>\n\
          <input type='text' id='msg' name='msg'><br>\n\
          <input type='submit' value='Send'>\n\
        </form>\n\
        <form action='/bright' method='post'>\n\
          <label for='fname'>Brightness:</label><br>\n\
          <input type='text' id='msg' name='brightness' value='32'><br>\n\
          <input type='submit' value='Set'>\n\
        </form>\n\
        <form action='/set' method='post'>\n\
          <input type='submit' name='cmd' value='Flash'>\n\
          <input type='submit' name='cmd' value='White'>\n\
          <input type='submit' name='cmd' value='Clear'>\n\
          <input type='submit' name='cmd' value='Off'>\n\
          <input type='submit' name='cmd' value='Next'>\n\
        </form>\n\
      </body>\n\
    </html>",
    ctime(&now),
    hr, min % 60, sec % 60
  );
  server.send(200, "text/html", temp);
  digitalWrite(led, 1);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

void handleBrightness() {
  if( ! server.hasArg("brightness") || server.arg("brightness") == NULL) {
    server.send(400, "text/plain", "400: Invalid Request");
    return;
  }

  Serial.println("B" + server.arg("brightness"));

  server.sendHeader("Location", "/",true);
  server.send(302, "text/plain","");   
}

void handleMsg() {
  if( ! server.hasArg("msg") || server.arg("msg") == NULL) {
    server.send(400, "text/plain", "400: Invalid Request");
    return;
  }

  //String msg = server.arg("msg");
  Serial.println("T" + server.arg("msg"));
  server.sendHeader("Location", "/",true);
  server.send(302, "text/plain","");   
}

void handleSet() {
  if( ! server.hasArg("cmd") || server.arg("cmd") == NULL) {
    server.send(400, "text/plain", "400: Invalid Request");
    return;
  }

  String webCmd = server.arg("cmd");
  String cmd;
  if(webCmd == "Flash") {
    cmd = "F";
  } else if(webCmd == "White") {
    cmd = "W";
  } else if(webCmd == "Clear") {
    cmd = "Z";
  } else if(webCmd == "Off") {
    cmd = "X";
  } else if(webCmd == "Next") {
    cmd = "N";
  } else {
    Serial.print("Unexpected command: ");
    Serial.println(webCmd);
  }

  Serial.println(cmd);

  server.sendHeader("Location", "/",true);
  server.send(302, "text/plain","");     
}

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostname);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.print(ssid);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
    Serial.print("Hostname: ");
    Serial.println(hostname);
  }

  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
  now = time(nullptr);
  Serial.println(ctime(&now));

  server.on("/", handleRoot);
  server.on("/bright", handleBrightness);
  server.on("/msg", handleMsg);
  server.on("/set", handleSet);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  digitalWrite(led, 1);
}

void loop(void) {
  server.handleClient();
  MDNS.update();
}
