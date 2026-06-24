#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include <Arduino.h>
#include <SPI.h>

#define LCD_WIDTH   160
#define LCD_HEIGHT  80

#define RGB565(r,g,b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

#define COLOR_BLACK    RGB565(0,0,0)
#define COLOR_WHITE    RGB565(255,255,255)
#define COLOR_RED      RGB565(255,0,0)
#define COLOR_GREEN    RGB565(0,255,0)
#define COLOR_BLUE     RGB565(0,0,255)
#define COLOR_CYAN     RGB565(0,255,255)
#define COLOR_YELLOW   RGB565(255,255,0)
#define COLOR_GRAY     RGB565(128,128,128)

class LCDDisplay {
public:
  LCDDisplay();
  void begin();
  void updateTime(const String &dateStr, const String &timeStr,
                  const String &weekdayStr, const String &statusStr);
  void showSplash(const String &line1, const String &line2);
  void drawWifiLED(bool connected, bool visible);
  void drawMonochromeBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *data, uint16_t color);
  int16_t drawCentered(int16_t y, const String &str, uint16_t color, uint8_t size);
  void setBrightness(uint8_t level);
  void clear(uint16_t color = COLOR_BLACK);
  void on();
  void off();

private:
  void writeCmd(uint8_t cmd);
  void writeData(uint8_t data);
  void writeData16(uint16_t data);
  void writeBuf(uint8_t *buf, uint32_t len);
  void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void fillCircle(int16_t cx, int16_t cy, int16_t r, uint16_t color);
  void drawChar(int16_t x, int16_t y, char c, uint16_t color, uint8_t size);
  void drawString(int16_t x, int16_t y, const String &str, uint16_t color, uint8_t size);
  void initST7735();

  SPIClass _spi;
  int8_t _cs, _dc, _rst, _bl;
  bool _initialized;
  uint8_t _brightness;
  String _lastDate, _lastTime, _lastWeekday, _lastStatus;
};

#endif
