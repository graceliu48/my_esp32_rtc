#include "api_server.h"
#include "rtc_manager.h"
#include "lcd_display.h"
#include "wifi_manager.h"
#include "rtc_config.h"
#include "app_html.h"
#include <WiFi.h>

ApiServer::ApiServer(RTCManager &rtc, LCDDisplay &lcd, WiFiManager &wifi)
  : _server(API_PORT), _rtc(rtc), _lcd(lcd), _wifi(wifi), _started(false) {
}

void ApiServer::begin() {
  // mDNS
  if (!MDNS.begin("dnesp32s3m")) {
    Serial.println("[API] mDNS 启动失败");
  } else {
    MDNS.addService("http", "tcp", API_PORT);
    Serial.println("[API] mDNS: dnesp32s3m.local");
  }

  _server.on("/api/time", HTTP_GET, [this]() { handleGetTime(); });
  _server.on("/api/status", HTTP_GET, [this]() { handleGetStatus(); });
  _server.on("/api/lcd", HTTP_POST, [this]() { handlePostLcd(); });
  _server.on("/api/brightness", HTTP_POST, [this]() { handlePostBrightness(); });
  _server.on("/api/sync", HTTP_POST, [this]() { handlePostSync(); });
  _server.on("/api/time", HTTP_POST, [this]() { handlePostTime(); });
  _server.on("/api/wifi", HTTP_POST, [this]() { handlePostWifi(); });
  _server.on("/api/restart", HTTP_POST, [this]() { handlePostRestart(); });
  _server.on("/api/wifi/clear", HTTP_POST, [this]() { handleOptions(); });
  _server.on("/api/scan", HTTP_GET, [this]() { handleOptions(); });

  _server.on("/", HTTP_GET, [this]() { handleRoot(); });
  _server.on("/index.html", HTTP_GET, [this]() { handleRoot(); });
  _server.on("/manifest.json", HTTP_GET, [this]() {
    _server.send_P(200, "application/json", APP_MANIFEST_JSON);
  });
  _server.on("/sw.js", HTTP_GET, [this]() {
    _server.send_P(200, "application/javascript", APP_SW_JS);
  });

  _server.onNotFound([this]() {
    _server.send(404, "application/json", "{\"error\":\"not found\"}");
  });

  _server.begin();
  _started = true;
  Serial.printf("[API] HTTP 服务已启动 :%d\n", API_PORT);
}

void ApiServer::loop() {
  if (_started) _server.handleClient();
}

void ApiServer::stop() {
  if (_started) {
    _server.stop();
    MDNS.end();
    _started = false;
    Serial.println("[API] 服务已停止");
  }
}

// ---------- helpers ----------

void ApiServer::sendJson(int code, const String &json) {
  _server.send(code, "application/json; charset=utf-8", json);
}

String ApiServer::readBody() {
  if (!_server.hasArg("plain")) return "";
  return _server.arg("plain");
}

// ---------- handlers ----------

void ApiServer::handleGetTime() {
  struct tm *ti = _rtc.getLocalTime();
  if (!ti) {
    sendJson(200, "{\"synced\":false,\"date\":\"--\",\"time\":\"--:--:--\",\"weekday\":\"--\",\"uptime\":0}");
    return;
  }
  static const char *wd[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  char buf[256];
  snprintf(buf, sizeof(buf),
    "{\"synced\":true,"
    "\"date\":\"%04d-%02d-%02d\","
    "\"time\":\"%02d:%02d:%02d\","
    "\"weekday\":\"%s\","
    "\"uptime\":%lu}",
    ti->tm_year+1900, ti->tm_mon+1, ti->tm_mday,
    ti->tm_hour, ti->tm_min, ti->tm_sec,
    wd[ti->tm_wday],
    _rtc.getUptimeSeconds());
  sendJson(200, String(buf));
}

void ApiServer::handleGetStatus() {
  String ssid = WiFi.SSID();
  String ip = WiFi.localIP().toString();
  if (ip == "0.0.0.0") { ssid = "(disconnected)"; ip = "-"; }
  char buf[256];
  snprintf(buf, sizeof(buf),
    "{\"lcd\":%s,\"brightness\":%d,"
    "\"wifi\":\"%s\",\"ssid\":\"%s\",\"ip\":\"%s\"}",
    _lcd.isOn() ? "true" : "false",
    _lcd.getBrightness(),
    WiFi.isConnected() ? "connected" : "disconnected",
    ssid.c_str(), ip.c_str());
  sendJson(200, String(buf));
}

void ApiServer::handlePostLcd() {
  String body = readBody();
  if (body.indexOf("\"on\":true") >= 0) {
    _lcd.on();
    Serial.println("[API] LCD ON");
    sendJson(200, "{\"ok\":true,\"lcd\":true}");
  } else if (body.indexOf("\"on\":false") >= 0) {
    _lcd.off();
    Serial.println("[API] LCD OFF");
    sendJson(200, "{\"ok\":true,\"lcd\":false}");
  } else {
    sendJson(400, "{\"ok\":false,\"msg\":\"need {\\\"on\\\":true|false}\"}");
  }
}

void ApiServer::handlePostBrightness() {
  String body = readBody();
  int idx = body.indexOf("\"value\":");
  if (idx < 0) {
    sendJson(400, "{\"ok\":false,\"msg\":\"need {\\\"value\\\":0-100}\"}");
    return;
  }
  int v = body.substring(idx + 8).toInt();
  if (v < 0 || v > 100) {
    sendJson(400, "{\"ok\":false,\"msg\":\"value 0-100\"}");
    return;
  }
  _lcd.setBrightness((uint8_t)v);
  char buf[64];
  snprintf(buf, sizeof(buf), "{\"ok\":true,\"brightness\":%d}", v);
  sendJson(200, String(buf));
  Serial.printf("[API] 亮度: %d\n", v);
}

void ApiServer::handlePostSync() {
  sendJson(200, "{\"ok\":true,\"msg\":\"NTP sync triggered\"}");
  Serial.println("[API] NTP sync requested (will retry next loop)");
}

void ApiServer::handlePostTime() {
  String body = readBody();
  int di = body.indexOf("\"datetime\":\"");
  if (di < 0) {
    sendJson(400, "{\"ok\":false,\"msg\":\"need {\\\"datetime\\\":\\\"YYYY-MM-DD HH:MM:SS\\\"}\"}");
    return;
  }
  di += 12;
  int de = body.indexOf("\"", di);
  if (de < 0) { sendJson(400, "{\"ok\":false}"); return; }
  String dt = body.substring(di, de);
  int y,M,d,h,m,s;
  if (sscanf(dt.c_str(), "%d-%d-%d %d:%d:%d", &y,&M,&d,&h,&m,&s) != 6) {
    sendJson(400, "{\"ok\":false,\"msg\":\"format: YYYY-MM-DD HH:MM:SS\"}");
    return;
  }
  bool ok = _rtc.setDateTime(y,M,d,h,m,s);
  char buf[64];
  snprintf(buf, sizeof(buf), "{\"ok\":%s}", ok ? "true" : "false");
  sendJson(200, String(buf));
  Serial.printf("[API] 校时: %s -> %s\n", dt.c_str(), ok ? "OK" : "FAIL");
}

void ApiServer::handlePostWifi() {
  String body = readBody();
  int si = body.indexOf("\"ssid\":\"");
  if (si < 0) { sendJson(400, "{\"ok\":false,\"msg\":\"need ssid\"}"); return; }
  si += 7;
  int se = body.indexOf("\"", si);
  if (se < 0) { sendJson(400, "{\"ok\":false}"); return; }
  String ssid = body.substring(si, se);
  int pi = body.indexOf("\"password\":\"");
  String pass;
  if (pi >= 0) {
    pi += 11;
    int pe = body.indexOf("\"", pi);
    if (pe >= 0) pass = body.substring(pi, pe);
  }
  ssid.replace("\\\"", "\"");
  pass.replace("\\\"", "\"");
  _wifi.saveCredentials(ssid, pass);
  Serial.printf("[API] WiFi 配置已保存: %s\n", ssid.c_str());
  sendJson(200, "{\"ok\":true,\"msg\":\"WiFi configured, will restart\"}");
  delay(500);
  ESP.restart();
}

void ApiServer::handlePostRestart() {
  sendJson(200, "{\"ok\":true,\"msg\":\"restarting\"}");
  Serial.println("[API] 重启...");
  delay(500);
  ESP.restart();
}

void ApiServer::handleOptions() {
  _server.send(200, "application/json", "{}");
}

void ApiServer::handleRoot() {
  _server.send_P(200, "text/html; charset=utf-8", APP_INDEX_HTML);
}
