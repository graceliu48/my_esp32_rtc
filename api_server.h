#ifndef API_SERVER_H
#define API_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <ESPmDNS.h>

class RTCManager;
class LCDDisplay;
class WiFiManager;

class ApiServer {
public:
  ApiServer(RTCManager &rtc, LCDDisplay &lcd, WiFiManager &wifi);
  void begin();
  void loop();
  void stop();

private:
  void handleGetTime();
  void handleGetStatus();
  void handlePostLcd();
  void handlePostBrightness();
  void handlePostSync();
  void handlePostTime();
  void handlePostWifi();
  void handlePostRestart();
  void handleOptions();
  void sendJson(int code, const String &json);
  String readBody();
  bool authCheck();

  WebServer _server;
  RTCManager &_rtc;
  LCDDisplay &_lcd;
  WiFiManager &_wifi;
  bool _started;
};

#endif
