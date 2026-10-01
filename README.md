# Dental Stopwatch & Smart Clock for ESP32-C3

A kid-friendly 2-minute dental brushing stopwatch and smart clock built with **PlatformIO** and **ESP-IDF** for the **ESP32-C3 Super Mini** and a **1.28" Round TFT display (240x240, GC9A01)**.

---

## Features

- **Smart Watchface (Default Mode)**:
  - Real-world time via **NTP** over WiFi in 12-hour format with AM/PM badge.
  - **Home Assistant Outdoor Weather**: Live outside temperature reading polled via HA REST API, rounded to integer °C and color-coded (Ice Blue &lt;0°C, Mint Green 0–20°C, Sunny Yellow 20–30°C, Hot Red &gt;30°C).
  - Clean horizontal beveled divider separating time, date, and temperature.
  - Smooth orbiting analog seconds marker along the outer bezel.

- **2-Minute Dental Brushing Timer**:
  - Activated by pressing the onboard **BOOT** button (GPIO 9).
  - **4 Guided 30-Second Quadrants**:
    - **Q1 (0s–30s)**: Electric Sky Blue (`Q1: UPPER RIGHT`)
    - **Q2 (30s–60s)**: Mint Green (`Q2: LOWER RIGHT`)
    - **Q3 (60s–90s)**: Golden Amber (`Q3: LOWER LEFT`)
    - **Q4 (90s–120s)**: Neon Pink (`Q4: UPPER LEFT`)
  - Dynamic 4-color progress ring that fills clockwise as brushing progresses.
  - Bold, high-contrast countdown digits in **Ubuntu Mono** (without confusing leading zeros).

- **10 Random Celebration Animations**:
  - Automatically selects a random colorful celebration when the 2 minutes complete:
    0. **Superstar Clean Tooth**: Bobbing smiling tooth, 360° spinning rainbow bezel, and party confetti.
    1. **Fireworks Spectacular**: 4 multi-stage bursting firework shells with rising trails, expanding spark rings, and gravity fall.
    2. **Space Rocket Blast-Off**: Retro rocket with delta wings, animated thruster fire plume, and warp-speed star streaks.
    3. **Golden Trophy Champion**: 12-segment rotating sunburst rays, polished gold chalice with "#1" badge, and golden coin rain.
    4. **Rainbow Spiral Vortex**: Swirling 6-arm rainbow galaxy spiral with center energy shockwaves.
    5. **Happy Smiling Emoji**: Big 64px yellow smiley with wink, rosy cheeks, and wide toothy grin with pearly-white clean teeth.
    6. **Cyber Disco Laser Show**: Mirrored disco ball with reflective mirror facets, 8 sweeping neon lasers, and rhythmic equalizer bars.
    7. **Bubble Pop Fiesta**: 14 translucent shimmering soap bubbles that float up and POP into splash droplets around a protected tooth.
    8. **Superhero Lightning Shield**: Concentric star-shield with 4 crackling, procedural branching electric lightning bolts.
    9. **Carnival Confetti Cannon**: Dual bottom cannons firing ballistic streamer arcs and floating party balloons.

- **Performance & Reliability**:
  - **Fluid ~30–50 FPS**: Rendered 100% offscreen and transmitted via 80 MHz SPI DMA directly into the display's GRAM.
  - **Fast Integer Math**: Zero software floating-point bottlenecks on the RISC-V core.
  - **Background Debouncer**: Dedicated FreeRTOS task running at 100 Hz ensures zero missed button presses.

---

## Hardware Pinout

- **MCU**: TENSTAR Robot ESP32-C3 Super Mini
- **Display**: 1.28" Round LCD (GC9A01, 240x240 RGB565)

| Display Pin | ESP32-C3 Super Mini | Description |
|:---:|:---:|:---|
| **RST** | **GPIO 0** | Display Reset |
| **CS** | **GPIO 1** | SPI Chip Select |
| **DC** | **GPIO 2** | Data / Command Select |
| **SDA (MOSI)** | **GPIO 3** | SPI MOSI Data |
| **SCL (SCK)** | **GPIO 4** | SPI Clock |
| **VCC** | **3.3V** | Power (3.3V) |
| **GND** | **GND** | Ground |
| **BLK** | **3.3V** | Backlight (always on) |
| **BOOT** | **GPIO 9** | Onboard Button (Active LOW) |

---

## Configuration

Credentials and settings can be customized in `include/config.h` or placed in a private `include/config_local.h` (ignored by git):

```c
// include/config_local.h
#define WIFI_SSID           "Your_WiFi_SSID"
#define WIFI_PASSWORD       "Your_WiFi_Password"

#define HA_BASE_URL         "http://192.168.1.100:8123"
#define HA_SENSOR_ENTITY_ID "sensor.outdoor_temperature"
#define HA_API_TOKEN        "Your_Home_Assistant_Long_Lived_Access_Token"
```

---

## Controls

| Mode | Action | Result |
|:---|:---:|:---|
| **Clock Face** | Short Press | Starts 2-minute brushing timer |
| **Clock Face** | Long Press (~1s) | **Testing Shortcut**: Jumps straight into celebration mode |
| **Timer Running** | Short Press | Restarts timer from beginning |
| **Timer Running** | Long Press | Cancels timer & returns to clock |
| **Celebration** | Short Press | **Cycles to next animation** (0 &rarr; 9) & refreshes timer |
| **Celebration** | Long Press | Exits to clock (or auto-exits after 10s) |

---

## Build & Flash

```bash
# Build firmware
pio run

# Flash to ESP32-C3 Super Mini
pio run -t upload

# Monitor serial output
pio device monitor
```

---

## License

MIT License. Free for personal and educational use.
