#ifndef RTC_CONFIG_H
#define RTC_CONFIG_H

// ============================================
// WiFi 配置
// ============================================
#define WIFI_SSID       "TP-LINK_2.4G_68F6B0"
#define WIFI_PASSWORD   "Lu13506410711"

// ============================================
// NTP 服务器配置
// ============================================
#define NTP_SERVER1     "ntp.aliyun.com"
#define NTP_SERVER2     "ntp1.aliyun.com"
#define NTP_SERVER3     "pool.ntp.org"

// ============================================
// 时区配置 ( Asia/Shanghai = UTC+8 )
// ============================================
#define TIMEZONE_OFFSET_SEC   (8 * 3600)
#define NTP_TIMEOUT_MS        5000

// ============================================
// 显示间隔 (毫秒)
// ============================================
#define DISPLAY_INTERVAL_MS   1000

// ============================================
// NTP 自动同步间隔
// ============================================
#define NTP_SYNC_INTERVAL_SEC 3600

// ============================================
// LCD 引脚定义 (DNESP32S3M 板载 0.96" TFT)
// 驱动: ST7735S, 分辨率: 160x80, 接口: SPI
// ============================================
#define LCD_CS    39
#define LCD_DC    40
#define LCD_RST   38
#define LCD_BL    41
#define LCD_MOSI  11
#define LCD_SCLK  12

#define LCD_WIDTH   160
#define LCD_HEIGHT  80

// ============================================
// 按键定义 (GPIO0 = BOOT 按键)
// ============================================
#define CONFIG_BUTTON   0
#define LONG_PRESS_MS   3000

// ============================================
// IO4 输入检测 / IO5 输出 (电平变化触发翻转)
// ============================================
#define IO4_INPUT   4
#define IO5_OUTPUT  5
#define IO6_INPUT   6

// ============================================
// API Server (normal mode HTTP REST API)
// ============================================
#define API_PORT  80

#endif
