#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * HARDWARE PIN DEFINITIONS (TENSTAR Robot ESP32-C3 Super Mini + GC9A01)
 * ========================================================================= */
#define PIN_TFT_RST         0   // Display Reset
#define PIN_TFT_CS          1   // SPI Chip Select
#define PIN_TFT_DC          2   // Data / Command Select
#define PIN_TFT_SDA         3   // SPI MOSI / SDA
#define PIN_TFT_SCL         4   // SPI Clock / SCL
#define PIN_BOOT_BTN        9   // Onboard BOOT Button (Active LOW)

/* =========================================================================
 * DISPLAY PARAMETERS
 * ========================================================================= */
#define TFT_WIDTH           240
#define TFT_HEIGHT          240
#define TFT_SPI_HOST        SPI2_HOST
#define TFT_SPI_CLOCK_HZ    (80 * 1000 * 1000) // 80 MHz SPI Clock (matches outside_temp_display)

#if __has_include("config_local.h")
#include "config_local.h"
#endif

/* =========================================================================
 * WIFI & NTP CONFIGURATION
 * ========================================================================= */
#ifndef WIFI_SSID
#define WIFI_SSID           "YOUR_WIFI_SSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD       "YOUR_WIFI_PASSWORD"
#endif

#ifndef NTP_SERVER1
#define NTP_SERVER1         "pool.ntp.org"
#endif

#ifndef NTP_SERVER2
#define NTP_SERVER2         "time.google.com"
#endif

// POSIX Timezone string (default: Eastern Time with DST)
// Examples:
// US Eastern: "EST5EDT,M3.2.0,M11.1.0"
// US Central: "CST6CDT,M3.2.0,M11.1.0"
// US Pacific: "PST8PDT,M3.2.0,M11.1.0"
// UK:         "GMT0BST,M3.5.0/1,M10.5.0"
// Central EU: "CET-1CEST,M3.5.0,M10.5.0/3"
// UTC:        "UTC0"
#ifndef DEFAULT_TIMEZONE
#define DEFAULT_TIMEZONE    "EST5EDT,M3.2.0,M11.1.0"
#endif

/* =========================================================================
 * HOME ASSISTANT CONFIGURATION (OUTSIDE TEMPERATURE SENSOR)
 * ========================================================================= */
#ifndef HA_ENABLED
#define HA_ENABLED              1
#endif

// Base URL of your Home Assistant instance (e.g. "http://192.168.1.100:8123")
#ifndef HA_BASE_URL
#define HA_BASE_URL             "http://YOUR_HA_IP:8123"
#endif

// Outdoor sensor entity ID from Home Assistant
#ifndef HA_SENSOR_ENTITY_ID
#define HA_SENSOR_ENTITY_ID     "sensor.outdoor_temperature"
#endif

// Long-Lived Access Token created in Home Assistant:
// HA UI -> User Profile (bottom left) -> Security -> Long-Lived Access Tokens
#ifndef HA_API_TOKEN
#define HA_API_TOKEN            "YOUR_HA_LONG_LIVED_ACCESS_TOKEN"
#endif

// Temperature polling interval (seconds)
#ifndef HA_POLL_INTERVAL_SEC
#define HA_POLL_INTERVAL_SEC    60
#endif

/* =========================================================================
 * TIMER CONFIGURATION
 * ========================================================================= */
#define TIMER_TOTAL_SECONDS (120)  // 2-minute dental brushing timer

/* =========================================================================
 * RGB565 COLOR DEFINITIONS
 * ========================================================================= */
#define COLOR_BLACK         0x0000
#define COLOR_WHITE         0xFFFF
#define COLOR_DARK_BG       0x0842  // Very deep slate / dark navy
#define COLOR_TIMER_BG      0x10A4  // Muted navy for timer non-elapsed area
#define COLOR_WEDGE         0x05BD  // Vibrant Dental Cyan / Teal (#00B7D4)
#define COLOR_WEDGE_ACCENT  0x07FF  // Bright Cyan
#define COLOR_RING          0x39E7  // Subtle steel ring
#define COLOR_RING_BRIGHT   0x07FF  // Bright ring indicator
#define COLOR_TEXT_DIM      0x7BEF  // Dim grey-blue
#define COLOR_GREEN         0x07E0  // Success green
#define COLOR_AMBER         0xFDA0  // Warning amber
#define COLOR_RED           0xF800  // Red for offline/alerts

#ifdef __cplusplus
}
#endif
