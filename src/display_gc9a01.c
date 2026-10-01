#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "display_gc9a01.h"

static const char *TAG = "GC9A01";

static spi_device_handle_t s_spi = NULL;
static uint16_t *s_framebuffers[2] = {NULL, NULL};
static bool s_double_buffered = false;
static uint8_t s_draw_idx = 0;

static void dc_command(void)
{
    gpio_set_level(PIN_TFT_DC, 0);
}

static void dc_data(void)
{
    gpio_set_level(PIN_TFT_DC, 1);
}

static void send_cmd(uint8_t cmd)
{
    dc_command();
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    spi_device_polling_transmit(s_spi, &t);
}

static void send_data(const uint8_t *data, size_t len)
{
    if (len == 0) return;
    dc_data();
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;
    t.tx_buffer = data;
    spi_device_polling_transmit(s_spi, &t);
}

static void send_cmd_byte(uint8_t cmd, uint8_t byte)
{
    send_cmd(cmd);
    send_data(&byte, 1);
}

static void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t caset[4] = {
        (uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF),
        (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF)
    };
    uint8_t raset[4] = {
        (uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF),
        (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF)
    };

    send_cmd(0x2A); // CASET
    send_data(caset, 4);

    send_cmd(0x2B); // RASET
    send_data(raset, 4);

    send_cmd(0x2C); // RAMWR
}


bool display_is_double_buffered(void)
{
    return s_double_buffered;
}

uint16_t *display_get_framebuffer(void)
{
    return s_framebuffers[s_draw_idx];
}

void display_clear(uint16_t color)
{
    uint16_t *fb = s_framebuffers[s_draw_idx];
    if (!fb) return;

    if (color == 0) {
        // Fast memset for black (< 0.1ms)
        memset(fb, 0, TFT_FB_SIZE);
    } else {
        uint32_t c32 = ((uint32_t)color << 16) | color;
        uint32_t *p32 = (uint32_t *)fb;
        size_t count32 = TFT_PIXELS / 2;
        for (size_t i = 0; i < count32; i++) {
            p32[i] = c32;
        }
    }
}

void display_wait_flush(void)
{
    // Synchronous polling DMA chunks complete before returning
}

esp_err_t display_flush(void)
{
    uint16_t *buf_to_send = s_framebuffers[s_draw_idx];
    if (!buf_to_send) return ESP_ERR_INVALID_STATE;

    // 1. Set full screen window on GC9A01 once
    set_window(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);

    // 2. Transmit in chunks <= 32,768 bytes (hardware limit is 18 bits = 32,768 bytes)
    // 115,200 bytes total / 4 = 28,800 bytes per chunk (60 lines each)
    dc_data();
    const uint8_t *ptr = (const uint8_t *)buf_to_send;
    const size_t chunk_size = 28800; // 28,800 * 8 = 230,400 bits <= 262,144 max bit len

    for (int i = 0; i < 4; i++) {
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = chunk_size * 8; // length in bits
        t.tx_buffer = ptr + (i * chunk_size);
        esp_err_t ret = spi_device_polling_transmit(s_spi, &t);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "DMA chunk %d failed: %s", i, esp_err_to_name(ret));
            return ret;
        }
    }

    // 3. In double-buffered mode, swap to other buffer for next frame
    if (s_double_buffered) {
        s_draw_idx = 1 - s_draw_idx;
    }

    return ESP_OK;
}

esp_err_t display_gc9a01_init(void)
{
    ESP_LOGI(TAG, "Initializing GC9A01 LCD (240x240, 80MHz SPI, RST=%d, CS=%d, DC=%d, SDA=%d, SCL=%d)",
             PIN_TFT_RST, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_SDA, PIN_TFT_SCL);

    // 1. Configure DC and RST GPIOs
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_TFT_DC) | (1ULL << PIN_TFT_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // 2. Hardware Reset
    gpio_set_level(PIN_TFT_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(PIN_TFT_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    // 3. Configure SPI Bus for max throughput (80 MHz)
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_TFT_SDA,
        .miso_io_num = -1,
        .sclk_io_num = PIN_TFT_SCL,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32768,
    };
    esp_err_t ret = spi_bus_initialize(TFT_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // 4. Attach GC9A01 device to SPI bus at 80 MHz
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = TFT_SPI_CLOCK_HZ,
        .mode = 0,
        .spics_io_num = PIN_TFT_CS,
        .queue_size = 4,
        .flags = SPI_DEVICE_NO_DUMMY,
    };
    ret = spi_bus_add_device(TFT_SPI_HOST, &devcfg, &s_spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    // 5. Allocate DMA Framebuffer memory
    ESP_LOGI(TAG, "Allocating DMA framebuffer memory (%d bytes)...", TFT_FB_SIZE);
    s_framebuffers[0] = (uint16_t *)heap_caps_malloc(TFT_FB_SIZE, MALLOC_CAP_DMA);
    if (!s_framebuffers[0]) {
        ESP_LOGE(TAG, "Failed to allocate primary framebuffer (%d bytes)", TFT_FB_SIZE);
        return ESP_ERR_NO_MEM;
    }

    // Try allocating second buffer for full double-buffering
    s_framebuffers[1] = (uint16_t *)heap_caps_malloc(TFT_FB_SIZE, MALLOC_CAP_DMA);
    if (s_framebuffers[1] != NULL) {
        s_double_buffered = true;
        ESP_LOGI(TAG, "Successfully enabled true double-buffering (2x %d bytes)", TFT_FB_SIZE);
    } else {
        s_double_buffered = false;
        ESP_LOGI(TAG, "Single 115.2 KB framebuffer enabled (DMA synchronized)");
    }
    s_draw_idx = 0;

    // 6. GC9A01 Initialization Commands
    send_cmd(0xEF);
    send_cmd_byte(0xEB, 0x14);
    send_cmd(0xFE);
    send_cmd(0xEF);
    send_cmd_byte(0xEB, 0x14);

    send_cmd_byte(0x84, 0x40);
    send_cmd_byte(0x85, 0xFF);
    send_cmd_byte(0x86, 0xFF);
    send_cmd_byte(0x87, 0xFF);
    send_cmd_byte(0x88, 0x0A);
    send_cmd_byte(0x89, 0x21);
    send_cmd_byte(0x8A, 0x00);
    send_cmd_byte(0x8B, 0x80);
    send_cmd_byte(0x8C, 0x01);
    send_cmd_byte(0x8D, 0x01);
    send_cmd_byte(0x8E, 0xFF);
    send_cmd_byte(0x8F, 0xFF);

    // Display Function Control
    const uint8_t b6_data[] = {0x00, 0x20};
    send_cmd(0xB6);
    send_data(b6_data, sizeof(b6_data));

    // Memory Access Control: BGR mode
    send_cmd_byte(0x36, 0x08);

    // Pixel format: 16-bit 565
    send_cmd_byte(0x3A, 0x05);

    const uint8_t p90[] = {0x08, 0x08, 0x08, 0x08};
    send_cmd(0x90);
    send_data(p90, sizeof(p90));

    send_cmd_byte(0xBD, 0x06);
    send_cmd_byte(0xBC, 0x00);

    const uint8_t pFF[] = {0x60, 0x01, 0x04};
    send_cmd(0xFF);
    send_data(pFF, sizeof(pFF));

    send_cmd_byte(0xC3, 0x13);
    send_cmd_byte(0xC4, 0x13);
    send_cmd_byte(0xC9, 0x22);
    send_cmd_byte(0xBE, 0x11);

    const uint8_t pE1[] = {0x10, 0x0E};
    send_cmd(0xE1);
    send_data(pE1, sizeof(pE1));

    const uint8_t pDF[] = {0x21, 0x0C, 0x02};
    send_cmd(0xDF);
    send_data(pDF, sizeof(pDF));

    // Gamma control
    const uint8_t pF0[] = {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A};
    send_cmd(0xF0);
    send_data(pF0, sizeof(pF0));

    const uint8_t pF1[] = {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F};
    send_cmd(0xF1);
    send_data(pF1, sizeof(pF1));

    const uint8_t pF2[] = {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A};
    send_cmd(0xF2);
    send_data(pF2, sizeof(pF2));

    const uint8_t pF3[] = {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F};
    send_cmd(0xF3);
    send_data(pF3, sizeof(pF3));

    const uint8_t pED[] = {0x1B, 0x0B};
    send_cmd(0xED);
    send_data(pED, sizeof(pED));

    send_cmd_byte(0xAE, 0x77);
    send_cmd_byte(0xCD, 0x63);

    const uint8_t p70[] = {0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03};
    send_cmd(0x70);
    send_data(p70, sizeof(p70));

    send_cmd_byte(0xE8, 0x34);

    const uint8_t p62[] = {0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70};
    send_cmd(0x62);
    send_data(p62, sizeof(p62));

    const uint8_t p63[] = {0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70};
    send_cmd(0x63);
    send_data(p63, sizeof(p63));

    const uint8_t p64[] = {0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07};
    send_cmd(0x64);
    send_data(p64, sizeof(p64));

    const uint8_t p66[] = {0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00};
    send_cmd(0x66);
    send_data(p66, sizeof(p66));

    const uint8_t p67[] = {0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98};
    send_cmd(0x67);
    send_data(p67, sizeof(p67));

    const uint8_t p74[] = {0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00};
    send_cmd(0x74);
    send_data(p74, sizeof(p74));

    const uint8_t p98[] = {0x3E, 0x07};
    send_cmd(0x98);
    send_data(p98, sizeof(p98));

    send_cmd(0x35); // TEON

    // Inversion ON (required for GC9A01 IPS panel)
    send_cmd(0x21);

    // Sleep out
    send_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));

    // Display ON
    send_cmd(0x29);
    vTaskDelay(pdMS_TO_TICKS(20));

    // Clear display to black
    display_clear(COLOR_BLACK);
    display_flush();

    ESP_LOGI(TAG, "GC9A01 display ready at 80 MHz");
    return ESP_OK;
}
