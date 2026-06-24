#include "rtc_manager.h"
#include "rtc_config.h"
#include <WiFi.h>

RTCManager::RTCManager()
  : _callback(nullptr), _timeSynced(false), _wifiEverConnected(false), _lastNTPSync(0), _startMillis(0) {
}

RTCManager::~RTCManager() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void RTCManager::begin() {
  _startMillis = millis();
  WiFi.persistent(false);

  // 先判断 RTC 是否已有有效时间
  time_t now = ::time(nullptr);
  if (now > 100000) {
    _timeSynced = true;
    Serial.print("[RTC] 使用已有时间: ");
    Serial.println(getDateTimeString());
    if (_callback) _callback(getNow());
    return;
  }

  Serial.println("[RTC] 尝试 NTP 同步...");
  if (syncNTP()) {
    Serial.println("[RTC] NTP 同步成功");
    return;
  }

  // 编译时间回退
  struct tm tm0;
  if (compileTimeToEpoch(tm0)) {
    time_t t = mktime(&tm0);
    struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&tv, nullptr);
    _timeSynced = true;
    Serial.print("[RTC] 使用编译时间: ");
    Serial.println(getDateTimeString());
    if (_callback) _callback(getNow());
    return;
  }

  Serial.println("[RTC] 无法获取时间!");
}

void RTCManager::update() {
  unsigned long nowMs = millis();

  // 未同步时也定期重试
  if (!_timeSynced) {
    if (nowMs - _lastNTPSync > 30000) {
      _lastNTPSync = nowMs;
      if (syncNTP()) {
        Serial.println("[RTC] 延迟 NTP 同步成功");
      }
    }
    return;
  }

  // 定期重新同步
  if (nowMs - _lastNTPSync > NTP_SYNC_INTERVAL_SEC * 1000UL) {
    _lastNTPSync = nowMs;
    if (syncNTP()) {
      Serial.println("[RTC] 定期 NTP 同步成功");
    }
  }
}

bool RTCManager::syncNTP() {
  return syncNTP(WIFI_SSID, WIFI_PASSWORD);
}

bool RTCManager::syncNTP(const String &ssid, const String &password) {
  for (int attempt = 0; attempt < 2; attempt++) {
    Serial.printf("[WiFi] 尝试 %d/2: %s\n", attempt + 1, ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long start = millis();
    bool connected = false;
    while (millis() - start < 15000) {
      if (WiFi.status() == WL_CONNECTED) { connected = true; break; }
      delay(100);
      Serial.print(".");
    }

    if (!connected) {
      Serial.println(" 超时");
      WiFi.disconnect(true);
      continue;
    }
    _wifiEverConnected = true;
    Serial.println(" OK");

    configTime(TIMEZONE_OFFSET_SEC, 0,
               NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

    struct tm timeinfo;
    start = millis();
    bool gotTime = false;
    while (millis() - start < 10000) {
      delay(100);
      if (::getLocalTime(&timeinfo, 0) && timeinfo.tm_year >= (2024 - 1900)) {
        gotTime = true;
        break;
      }
    }

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    if (gotTime) {
      _timeSynced = true;
      _lastNTPSync = millis();
      if (_callback) _callback(getNow());
      return true;
    }

    Serial.println("[NTP] 超时");
  }
  return false;
}

bool RTCManager::doNTP() {
  configTime(TIMEZONE_OFFSET_SEC, 0,
             NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
  struct tm timeinfo;
  unsigned long start = millis();
  while (millis() - start < 10000) {
    delay(100);
    if (::getLocalTime(&timeinfo, 0) && timeinfo.tm_year >= (2024 - 1900)) {
      _timeSynced = true;
      _lastNTPSync = millis();
      _wifiEverConnected = true;
      if (_callback) _callback(getNow());
      return true;
    }
  }
  return false;
}

bool RTCManager::compileTimeToEpoch(struct tm &tm_out) {
  const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  char mon[4] = {0};
  int d, y, h, m, s;
  if (sscanf(__DATE__, "%3s %d %d", mon, &d, &y) != 3) return false;
  if (sscanf(__TIME__, "%d:%d:%d", &h, &m, &s) != 3) return false;
  const char *p = months;
  int idx = -1;
  for (int i = 0; i < 12; i++) {
    if (strncmp(p, mon, 3) == 0) { idx = i; break; }
    p += 3;
  }
  if (idx < 0) return false;
  tm_out.tm_year = y - 1900;
  tm_out.tm_mon  = idx;
  tm_out.tm_mday = d;
  tm_out.tm_hour = h;
  tm_out.tm_min  = m;
  tm_out.tm_sec  = s;
  tm_out.tm_isdst = -1;
  return true;
}

bool RTCManager::setDateTime(int year, int month, int day,
                              int hour, int min, int sec) {
  struct tm tm = {0};
  tm.tm_year = year - 1900;
  tm.tm_mon  = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min  = min;
  tm.tm_sec  = sec;
  tm.tm_isdst = -1;
  time_t t = mktime(&tm);
  if (t < 0) return false;
  struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
  settimeofday(&tv, nullptr);
  _timeSynced = true;
  _lastNTPSync = millis();
  if (_callback) _callback(t);
  return true;
}

bool RTCManager::setDateTimeFromString(const String &str) {
  int y, M, d, h, m, s;
  if (sscanf(str.c_str(), "%d-%d-%d %d:%d:%d",
             &y, &M, &d, &h, &m, &s) == 6) {
    return setDateTime(y, M, d, h, m, s);
  }
  return false;
}

time_t RTCManager::getNow() const {
  return time(nullptr);
}

struct tm* RTCManager::getLocalTime() const {
  static struct tm timeinfo;
  if (!::getLocalTime(&timeinfo, 0)) return nullptr;
  return &timeinfo;
}

String RTCManager::getTimeString() const {
  struct tm *ti = getLocalTime();
  if (!ti) return "N/A";
  char buf[10];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
           ti->tm_hour, ti->tm_min, ti->tm_sec);
  return String(buf);
}

String RTCManager::getDateString() const {
  struct tm *ti = getLocalTime();
  if (!ti) return "N/A";
  char buf[12];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
           ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday);
  return String(buf);
}

String RTCManager::getDateTimeString() const {
  struct tm *ti = getLocalTime();
  if (!ti) return "N/A";
  char buf[22];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
           ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
           ti->tm_hour, ti->tm_min, ti->tm_sec);
  return String(buf);
}

void RTCManager::printTime() const {
  Serial.print("[RTC] ");
  if (!_timeSynced) { Serial.println("未同步"); return; }
  Serial.print(getTimeString());
  Serial.print("  (开机 ");
  Serial.print(getUptimeSeconds());
  Serial.println("s)");
}

void RTCManager::printDate() const {
  Serial.print("[RTC] ");
  if (!_timeSynced) { Serial.println("未同步"); return; }
  Serial.println(getDateString());
}

void RTCManager::printAll() const {
  Serial.println("====== RTC =====");
  if (!_timeSynced) {
    Serial.println("状态: 未同步");
  } else {
    Serial.print("时间: ");
    Serial.println(getDateTimeString());
    Serial.print("开机: ");
    Serial.print(getUptimeSeconds());
    Serial.println("s");
  }
  Serial.println("================");
}

void RTCManager::onTimeUpdated(TimeUpdateCallback cb) {
  _callback = cb;
}

bool RTCManager::isTimeSynced() const {
  return _timeSynced;
}

unsigned long RTCManager::getUptimeSeconds() const {
  return (millis() - _startMillis) / 1000;
}
