/*
BSD 2-Clause License

Copyright (c) 2020, ANM-P4F
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <WebSocketsServer.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "camera_wrap.h"
#include "index_html.h"

#define DEBUG
// #define SAVE_IMG

const char* ssid = "++++++";   //Enter SSID WIFI Name
const char* password = "++++++";   //Enter WIFI Password

//holds the current upload
int cameraInitState = -1;
uint8_t* jpgBuff = new uint8_t[68123];
size_t   jpgLength = 0;
uint8_t camNo=0;
bool clientConnected[3] = {false,false,false};

WebSocketsServer webSocket = WebSocketsServer(86);
AsyncWebServer httpServer(80);
String html_home;

const int PIN_LED     = 4;

void setup(void) {

  Serial.begin(115200);
  Serial.print("\n");
  Serial.setDebugOutput(true);

  cameraInitState = initCamera();

  Serial.printf("camera init state %d\n", cameraInitState);

  if(cameraInitState != 0){
    return;
  }

  //WIFI INIT
  Serial.printf("Connecting to %s\n", ssid);
  if (String(WiFi.SSID()) != String(ssid)) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected! IP address: ");
  String ipAddress = WiFi.localIP().toString();;
  Serial.println(ipAddress);
  // handle index
  // Route for root / web page
  httpServer.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, sizeof(index_html_gz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  //handle command 
  httpServer.on("/MOVE", HTTP_GET, [](AsyncWebServerRequest *request){
    commadProcessing(request);
    request->send(200, "text/plain");
  });
  httpServer.on("/LED", HTTP_GET, [](AsyncWebServerRequest *request){
    commadProcessing(request);
    request->send(200, "text/plain");
  });
  httpServer.on("/CENTER", HTTP_GET, [](AsyncWebServerRequest *request){
    commadProcessing(request);
    request->send(200, "text/plain");
  });
  httpServer.on("/UP", HTTP_GET, [](AsyncWebServerRequest *request){
    commadProcessing(request);
    request->send(200, "text/plain");
  });
  httpServer.on("/DOWN", HTTP_GET, [](AsyncWebServerRequest *request){
    commadProcessing(request);
    request->send(200, "text/plain");
  });
  httpServer.on("/LEFT", HTTP_GET, [](AsyncWebServerRequest *request){
    commadProcessing(request);
    request->send(200, "text/plain");
  });
  httpServer.on("/RIGHT", HTTP_GET, [](AsyncWebServerRequest *request){
    commadProcessing(request);
    request->send(200, "text/plain");
  });

  httpServer.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  pinMode(PIN_LED, OUTPUT);
}

void commadProcessing(AsyncWebServerRequest *request){
  String value = request->getParam(0)->value();
  String url   = request->url();
  if(url.indexOf("LED")!=-1){
    if(value.indexOf("ON")!=-1){
      digitalWrite(PIN_LED, HIGH);
    }else if(value.indexOf("OFF")!=-1){
      digitalWrite(PIN_LED, LOW);
    }
  }else{
    String command = url+":"+value+"\n";
    Serial.print(command);
  }

}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
      case WStype_DISCONNECTED:
          Serial.printf("[%u] Disconnected!\n", num);
          if(num<3){
            camNo = num;
            clientConnected[num] = false;
            String command = "/MOVE:0,0";
            Serial.print(command);
          }
          break;
      case WStype_CONNECTED:
          if(num<3){
            clientConnected[num] = true;
          }
          break;
      case WStype_TEXT:
      case WStype_BIN:
      case WStype_ERROR:      
      case WStype_FRAGMENT_TEXT_START:
      case WStype_FRAGMENT_BIN_START:
      case WStype_FRAGMENT:
      case WStype_FRAGMENT_FIN:
          break;
  }
}

void loop(void) {
  webSocket.loop();
  if(clientConnected[camNo] == true){
    grabImage(jpgLength, jpgBuff);
    webSocket.sendBIN(camNo, jpgBuff, jpgLength);
  }

}
