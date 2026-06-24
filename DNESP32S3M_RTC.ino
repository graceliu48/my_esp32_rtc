#include "rtc_config.h"
#include "rtc_manager.h"
#include "lcd_display.h"
#include "wifi_manager.h"
#include "api_server.h"

// TG0_T0 hardware timer (1ms interrupt)
static hw_timer_t *g_timer = NULL;
static volatile unsigned long g_timer_ms = 0;

void IRAM_ATTR onTimer() {
  g_timer_ms++;
}

static inline unsigned long timerMillis() {
  return g_timer_ms;
}

// Breathing LED (PWM0, IO1)
#define BREATH_LED_PIN      1
#define BREATH_LED_CH       4
#define BREATH_FREQ         1000
#define BREATH_RESOLUTION   8
#define BREATH_STEP         5
#define BREATH_INTERVAL_MS  10

RTCManager rtc;
LCDDisplay lcd;
WiFiManager wifiMgr;
ApiServer apiServer(rtc, lcd, wifiMgr);
static bool apiServerActive = false;

static const char* WEEKDAYS_EN[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static bool configMode = false;
static unsigned long btnPressStart = 0;
static bool btnWasPressed = false;
static unsigned long lastNTPSync = 0;
static unsigned long lastRetry = 0;
static bool brandFlash = false;

static const uint8_t brandBMP[] = {
  0x08,0x20,0x08,0x20,0x04,0x40,0x08,0x00,
  0x04,0x48,0x08,0x20,0x04,0x44,0x0C,0x00,
  0x7F,0xFC,0x08,0x20,0xFF,0xFE,0x08,0x08,
  0x04,0x40,0x08,0x24,0x04,0x40,0x1F,0xFC,
  0x24,0x48,0xFE,0xFE,0x04,0x40,0x20,0x08,
  0x14,0x50,0x08,0x20,0x00,0x20,0x41,0x10,
  0x04,0x44,0x18,0x60,0x08,0x20,0x81,0x00,
  0xFF,0xFE,0x1C,0x70,0x04,0x40,0x01,0x00,
  0x00,0x10,0x2A,0xA8,0x04,0x40,0x09,0x40,
  0x1F,0xF8,0x28,0xAE,0x02,0x80,0x09,0x20,
  0x10,0x10,0x49,0x24,0x01,0x00,0x11,0x10,
  0x10,0x10,0x8A,0x20,0x02,0x80,0x11,0x18,
  0x1F,0xF0,0x08,0x20,0x04,0x40,0x21,0x08,
  0x10,0x10,0x08,0x20,0x08,0x30,0x01,0x00,
  0x10,0x10,0x08,0x20,0x30,0x0E,0x05,0x00,
  0x1F,0xF0,0x08,0x20,0xC0,0x04,0x02,0x00,
};

static bool wifiConnectAndNTP(const String &ssid, const String &password, bool keepWiFi = false) {
  if (wifiMgr.connectToWiFi(15000)) {
    if (rtc.doNTP()) {
      Serial.printf("[NTP] 同步成功: %s\n", rtc.getDateTimeString().c_str());
      if (!keepWiFi) {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
      }
      return true;
    }
    if (!keepWiFi) {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("====================================");
  Serial.println("  正点原子 DNESP32S3M RTC+NTP");
  Serial.println("====================================");

  // TG0_T0 1ms hardware timer
  g_timer = timerBegin(0, 80, true);
  if (g_timer) {
    timerAttachInterrupt(g_timer, &onTimer, true);
    timerAlarmWrite(g_timer, 1000, true);
    timerAlarmEnable(g_timer);
    Serial.println("[Timer] TG0_T0 1ms 中断已启动");
  } else {
    Serial.println("[Timer] 定时器初始化失败");
  }

  pinMode(CONFIG_BUTTON, INPUT_PULLUP);

  pinMode(IO4_INPUT, INPUT_PULLUP);
  pinMode(IO6_INPUT, INPUT_PULLUP);
  pinMode(IO5_OUTPUT, OUTPUT);
  digitalWrite(IO5_OUTPUT, HIGH);
  Serial.println("[GPIO] IO4=IN_PULLUP, IO5=OUT_HIGH");

  ledcSetup(BREATH_LED_CH, BREATH_FREQ, BREATH_RESOLUTION);
  ledcAttachPin(BREATH_LED_PIN, BREATH_LED_CH);
  Serial.printf("[PWM] CH%d @ IO%d, %dHz, %d-bit\n",
                BREATH_LED_CH, BREATH_LED_PIN, BREATH_FREQ, BREATH_RESOLUTION);

  lcd.begin();
  wifiMgr.begin();

  if (digitalRead(CONFIG_BUTTON) == LOW) {
    delay(50);
    if (digitalRead(CONFIG_BUTTON) == LOW) {
      Serial.println("[Boot] 检测到按键, 进入配网模式");
      enterConfigMode();
      printHelp();
      return;
    }
  }

  String ssid, pass;
  if (wifiMgr.hasStoredCredentials()) {
    ssid = wifiMgr.getStoredSSID();
    pass = wifiMgr.getStoredPassword();
    Serial.printf("[Boot] 使用已保存WiFi: %s\n", ssid.c_str());
  } else {
    ssid = WIFI_SSID;
    pass = WIFI_PASSWORD;
    Serial.println("[Boot] 使用默认WiFi配置");
  }

  rtc.onTimeUpdated([](time_t now) {
    Serial.printf("[RTC] 时间已更新: %s\n", rtc.getDateTimeString().c_str());
  });

  if (!wifiConnectAndNTP(ssid, pass, true)) {
    Serial.println("[Boot] NTP同步失败, 使用编译时间回退");
    rtc.begin();
    if (!rtc.isTimeSynced()) {
      Serial.println("[Boot] 无法获取时间, 进入配网模式");
      enterConfigMode();
      printHelp();
      return;
    }
  }

  apiServer.begin();
  apiServerActive = true;
  if (WiFi.isConnected()) {
    Serial.printf("[Boot] API Server: http://%s/\n", WiFi.localIP().toString().c_str());
  }

  printHelp();
}

void loop() {
  if (configMode) {
    wifiMgr.loop();
    lcd.drawWifiLED(false, (timerMillis() / 500) % 2);
    return;
  }

  if (apiServerActive) apiServer.loop();

  handleButton();

  // Periodic NTP re-sync
  if (rtc.isTimeSynced() &&
      timerMillis() - lastNTPSync > NTP_SYNC_INTERVAL_SEC * 1000UL) {
    lastNTPSync = timerMillis();
    if (apiServerActive && WiFi.isConnected()) {
      rtc.doNTP();
    } else {
      String ssid = wifiMgr.hasStoredCredentials() ? wifiMgr.getStoredSSID() : WIFI_SSID;
      String pass = wifiMgr.hasStoredCredentials() ? wifiMgr.getStoredPassword() : WIFI_PASSWORD;
      wifiConnectAndNTP(ssid, pass, false);
    }
  }

  // Retry if never synced
  if (!rtc.isTimeSynced() && timerMillis() - lastRetry > 30000) {
    lastRetry = timerMillis();
    if (apiServerActive && WiFi.isConnected()) {
      rtc.doNTP();
    } else {
      String ssid = wifiMgr.hasStoredCredentials() ? wifiMgr.getStoredSSID() : WIFI_SSID;
      String pass = wifiMgr.hasStoredCredentials() ? wifiMgr.getStoredPassword() : WIFI_PASSWORD;
      wifiConnectAndNTP(ssid, pass, false);
    }
  }

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    processCommand(cmd);
  }

  static unsigned long lastDisplay = 0;
  if (timerMillis() - lastDisplay > DISPLAY_INTERVAL_MS) {
    lastDisplay = timerMillis();
    rtc.printTime();
    updateLCD();
  }

  static unsigned long lastBlink = 0;
  static bool blinkOn = true;
  if (timerMillis() - lastBlink > 500) {
    lastBlink = timerMillis();
    blinkOn = !blinkOn;
    lcd.drawWifiLED(rtc.hasEverConnected(), blinkOn);
  }

  static unsigned long lastBreath = 0;
  static int breathVal = 0;
  static int breathDir = 1;
  if (timerMillis() - lastBreath > BREATH_INTERVAL_MS) {
    lastBreath = timerMillis();
    breathVal += breathDir * BREATH_STEP;
    if (breathVal >= (1 << BREATH_RESOLUTION) - 1) {
      breathVal = (1 << BREATH_RESOLUTION) - 1;
      breathDir = -1;
    }
    if (breathVal <= 0) {
      breathVal = 0;
      breathDir = 1;
    }
    ledcWrite(BREATH_LED_CH, breathVal);
  }

  static bool lastIO4 = HIGH;
  static bool lcdFlag = false;
  static unsigned long io4Debounce = 0;
  bool curIO4 = digitalRead(IO4_INPUT);
  if (lastIO4 == HIGH && curIO4 == LOW && timerMillis() - io4Debounce > 50) {
    io4Debounce = timerMillis();
    lcdFlag = !lcdFlag;
    if (lcdFlag) {
      lcd.on();
      digitalWrite(IO5_OUTPUT, LOW);
    } else {
      lcd.off();
      digitalWrite(IO5_OUTPUT, HIGH);
    }
    Serial.printf("[GPIO] IO4 HIGH->LOW, flag=%d, LCD=%s, IO5=%d\n",
                  lcdFlag, lcdFlag ? "ON" : "OFF", digitalRead(IO5_OUTPUT));
  }
  lastIO4 = curIO4;

  static bool lastIO6 = HIGH;
  static unsigned long io6Debounce = 0;
  bool curIO6 = digitalRead(IO6_INPUT);
  if (lastIO6 == HIGH && curIO6 == LOW && timerMillis() - io6Debounce > 50) {
    io6Debounce = timerMillis();
    brandFlash = !brandFlash;
    Serial.printf("[GPIO] IO6 HIGH->LOW, brandFlash=%d\n", brandFlash);
  }
  lastIO6 = curIO6;

  // Brand flash control (1Hz when brandFlash=1)
  static unsigned long lastBrandDraw = 0;
  static bool brandState = true;
  if (brandFlash) {
    if (timerMillis() - lastBrandDraw >= 500) {
      lastBrandDraw = timerMillis();
      brandState = !brandState;
      lcd.drawMonochromeBitmap(48, 62, 64, 16, brandBMP, brandState ? COLOR_WHITE : COLOR_BLACK);
    }
  } else if (timerMillis() - lastBrandDraw >= 1000) {
    lastBrandDraw = timerMillis();
    lcd.drawMonochromeBitmap(48, 62, 64, 16, brandBMP, COLOR_WHITE);
  }
}

void enterConfigMode() {
  configMode = true;
  if (apiServerActive) { apiServer.stop(); apiServerActive = false; }
  lcd.clear();
  lcd.drawCentered(10, "WiFi Config", COLOR_CYAN, 1);
  lcd.drawCentered(28, wifiMgr.getAPSSID(), COLOR_YELLOW, 1);
  lcd.drawCentered(44, "Connect to AP", COLOR_WHITE, 1);
  lcd.drawCentered(60, "Open 192.168.4.1", COLOR_GRAY, 1);
  wifiMgr.startConfigMode();
}

void handleButton() {
  bool pressed = (digitalRead(CONFIG_BUTTON) == LOW);
  if (pressed && !btnWasPressed) {
    btnPressStart = timerMillis();
    btnWasPressed = true;
  }
  if (!pressed && btnWasPressed) {
    if (timerMillis() - btnPressStart >= LONG_PRESS_MS) {
      Serial.println("[Button] 长按进入配网模式");
      enterConfigMode();
    }
    btnWasPressed = false;
  }
}



void updateLCD() {
  if (!rtc.isTimeSynced()) {
    lcd.updateTime("-- -- ----", "--:--:--", "N/A",
                   "Syncing NTP...");
    return;
  }

  struct tm *ti = rtc.getLocalTime();
  if (!ti) return;

  char dateBuf[12];
  snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d",
           ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday);

  char timeBuf[10];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d",
           ti->tm_hour, ti->tm_min, ti->tm_sec);

  String weekdayStr = String(WEEKDAYS_EN[ti->tm_wday]);

  char statusBuf[22];
  unsigned long uptime = rtc.getUptimeSeconds();
  if (rtc.hasEverConnected()) {
    snprintf(statusBuf, sizeof(statusBuf), "NTP on  UP %luh %02lum",
             uptime / 3600, (uptime % 3600) / 60);
  } else {
    snprintf(statusBuf, sizeof(statusBuf), "NTP --- UP %luh %02lum",
             uptime / 3600, (uptime % 3600) / 60);
  }

  lcd.updateTime(dateBuf, timeBuf, weekdayStr, statusBuf);
}

void processCommand(const String &cmd) {
  if (cmd == "help" || cmd == "?") {
    printHelp();
  } else if (cmd == "time") {
    rtc.printTime();
  } else if (cmd == "date") {
    rtc.printDate();
  } else if (cmd == "info") {
    rtc.printAll();
  } else if (cmd == "sync") {
    if (apiServerActive && WiFi.isConnected()) {
      Serial.println(rtc.doNTP() ? "NTP 同步成功" : "NTP 同步失败");
    } else {
      String ssid = wifiMgr.hasStoredCredentials() ? wifiMgr.getStoredSSID() : WIFI_SSID;
      String pass = wifiMgr.hasStoredCredentials() ? wifiMgr.getStoredPassword() : WIFI_PASSWORD;
      if (wifiConnectAndNTP(ssid, pass)) {
        Serial.println("NTP 同步成功");
      } else {
        Serial.println("NTP 同步失败");
      }
    }
  } else if (cmd.startsWith("set ")) {
    String args = cmd.substring(4);
    Serial.println(rtc.setDateTimeFromString(args) ?
      "时间已设置" : "格式错误, 请用 set YYYY-MM-DD HH:MM:SS");
  } else if (cmd == "lcd_on") {
    lcd.on(); Serial.println("LCD on");
  } else if (cmd == "lcd_off") {
    lcd.off(); Serial.println("LCD off");
  } else if (cmd.startsWith("bright ")) {
    int v = cmd.substring(7).toInt();
    if (v >= 0 && v <= 100) {
      lcd.setBrightness(v);
      Serial.printf("亮度: %d\n", v);
    } else {
      Serial.println("范围 0-100");
    }
  } else if (cmd == "wifi") {
    enterConfigMode();
  } else if (cmd == "wifi_clear") {
    wifiMgr.clearCredentials();
    Serial.println("WiFi配置已清除");
  } else if (cmd.length() > 0) {
    Serial.println("未知命令, 输入 help 查看");
  }
}

void printHelp() {
  Serial.println("");
  Serial.println("--- 命令 ---");
  Serial.println("  time            显示时间");
  Serial.println("  date            显示日期");
  Serial.println("  info            RTC 信息");
  Serial.println("  sync            NTP 同步");
  Serial.println("  lcd_on/off     LCD 开关");
  Serial.println("  bright 0-100   亮度");
  Serial.println("  set YYYY-MM-DD HH:MM:SS  校时");
  Serial.println("  wifi            进入配网模式");
  Serial.println("  wifi_clear      清除WiFi配置");
  Serial.println("------------");
  Serial.println("");
}
