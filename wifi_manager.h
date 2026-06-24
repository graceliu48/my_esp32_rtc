#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

class WiFiManager {
public:
  WiFiManager();
  void begin();      // Load stored creds, check button
  void loop();       // Handle web server
  bool startConfigMode();  // Start AP + web server
  void stopConfigMode();
  bool isConfigMode() const { return _configMode; }
  bool connectToWiFi(int timeout_ms = 20000);
  bool hasStoredCredentials() const { return _hasStoredCreds; }
  String getStoredSSID() const { return _storedSSID; }
  String getStoredPassword() const { return _storedPassword; }
  String getAPSSID() const { return _apSSID; }
  void clearCredentials();
  bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
  void saveCredentials(const String &ssid, const String &password);

private:
  void startAP();
  void startWebServer();
  void handleRoot();
  void handleScan();
  void handleSave();
  void handleManifest();
  void handleCSS();
  void loadCredentials();

  WebServer _server;
  Preferences _prefs;
  bool _configMode;
  bool _serverStarted;
  String _apSSID;
  String _storedSSID;
  String _storedPassword;
  bool _hasStoredCreds;
};

#endif
