/*
  Copyright 2021 Massimo Gismondi

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>

// File system
#include "FS.h"
#include <SPIFFS.h>


const int pins[] = {14, 15, 16};
const int pin_mode_selection = 2;
#define PIN_INTERRUTTORE 17

const int pwm_channels[] = {0, 2, 4};

const int PwmFreq = 300;
const int resolution = 8;
const int RED_MAX_DUTY_CYCLE = (int)(pow(2, resolution) - 1);


int color_duty_cycles[] = {00, 100, 100};



// Dati access point bas
const char *SSID_AP = "Lampada";
const char *PASSWORD_AP = "testtesttest";
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

bool access_point_acceso = true;

AsyncWebServer server(80);


// Dati a cui collegarsi
#include <Preferences.h>
Preferences preferences;

void save_wifi_ssid_psw(String ssid, String psw)
{
  preferences.begin("lampada", false);
  preferences.putString("SSID", ssid);
  preferences.putString("PSW", psw);
  preferences.end();
}
String load_wifi_psw()
{
  preferences.begin("lampada", true);
  String psw = preferences.getString("PSW", "");
  preferences.end();
  return psw;
}
String load_wifi_ssid()
{
  preferences.begin("lampada", true);
  String ssid = preferences.getString("SSID", "");
  preferences.end();
  return ssid;
}

void setup_access_point()
{

  
  //WiFi.softAP("lampada", "123412341234");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID_AP, PASSWORD_AP);
  
  WiFi.softAPConfig(
    local_ip, 
    gateway,
    subnet);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // RESTITUISCO I FILE
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });
  server.on("/global.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/global.css", "text/css");
  });
  server.on("/build/bundle.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/build/bundle.js", "text/javascript");
  });
  server.on("/build/bundle.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/build/bundle.css", "text/css");
  });

  server.on("/static/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/static/bootstrap.min.css", "text/css");
  });


  server.on("/save_network", HTTP_POST, [](AsyncWebServerRequest * request) {
    if (
      request->hasParam("ssid")
      &&
      request->hasParam("psw")
      )
    {
      String ssid = request->getParam("ssid")->value().c_str();
      String psw = request->getParam("psw")->value().c_str();
      save_wifi_ssid_psw(ssid, psw);
      request->send(200);
      delay(1000);
      ESP.restart();
    }
    else
    {
      request->send(400);
    }
  });


  server.begin();
  Serial.println("Server started");

}

void connect_to_network()
{
    preferences.begin("lampada", true);
    char ssid[40];
    char psw[40];
    String ssid_str = load_wifi_ssid();
    ssid_str.toCharArray(ssid, 40);
    String psw_str = load_wifi_psw();
    psw_str.toCharArray(psw, 40);
    WiFi.begin(ssid, psw);
    int secondi_tempo_atteso = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      secondi_tempo_atteso++;
      Serial.print(".");

      if (secondi_tempo_atteso > 60)
      {
        Serial.println("Errore di connessione");
        return;
      }
    }
    Serial.println(WiFi.localIP());  
}

void setup()
{
  delay(500);
  Serial.begin(9600);
  Serial.println("Avvio...");


  // pinMode
  pinMode(PIN_INTERRUTTORE, INPUT_PULLUP);
  delay(1.0);
  Serial.println();
  
  if (!digitalRead(PIN_INTERRUTTORE))
  {
    // SE IL PULSANTE È PREMUTO
    // avvio il router
    setup_access_point();
  }
  else
  {
    // Altrimenti provo a collegarmi all'access point
    // salvato, se esiste, e metto colori "giusti"
    connect_to_network();
  }

  // Apro filesystem SPIFFS
  if(!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  

  // IMPOSTO LED LAMPADA RGB
  ledcSetup(pwm_channels[0], PwmFreq, resolution);
  ledcAttachPin(pins[0], pwm_channels[0]);

  ledcSetup(pwm_channels[1], PwmFreq, resolution);
  ledcAttachPin(pins[1], pwm_channels[1]); 

  ledcSetup(pwm_channels[2], PwmFreq, resolution);
  ledcAttachPin(pins[2], pwm_channels[2]); 
}


void loop()
{
  ledcWrite(pwm_channels[0], color_duty_cycles[0]);
  ledcWrite(pwm_channels[1], color_duty_cycles[1]);
  ledcWrite(pwm_channels[2], color_duty_cycles[2]);
  delay(1500);

  Serial.println(load_wifi_ssid());
  Serial.println(load_wifi_psw());
}
