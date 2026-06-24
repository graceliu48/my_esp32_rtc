#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include <Arduino.h>
#include <time.h>

typedef void (*TimeUpdateCallback)(time_t now);

class RTCManager {
public:
  RTCManager();
  ~RTCManager();

  void begin();
  void update();

  bool syncNTP();
  bool syncNTP(const String &ssid, const String &password);
  bool doNTP();  // NTP sync only, assumes WiFi connected
  bool setDateTime(int year, int month, int day, int hour, int min, int sec);
  bool setDateTimeFromString(const String &str);

  time_t getNow() const;
  struct tm* getLocalTime() const;

  String getTimeString() const;
  String getDateString() const;
  String getDateTimeString() const;

  void printTime() const;
  void printDate() const;
  void printAll() const;

  void onTimeUpdated(TimeUpdateCallback cb);

  bool isTimeSynced() const;
  bool hasEverConnected() const { return _wifiEverConnected; }
  unsigned long getUptimeSeconds() const;

private:
  bool compileTimeToEpoch(struct tm &tm_out);

  TimeUpdateCallback _callback;
  bool _timeSynced;
  bool _wifiEverConnected;
  unsigned long _lastNTPSync;
  unsigned long _startMillis;
};

#endif
