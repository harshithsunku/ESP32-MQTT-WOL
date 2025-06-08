#include "device_info.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_mac.h"
#include "esp_partition.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"
#include "esp_efuse.h"
#include "esp_psram.h"
#include "soc/soc_caps.h"
#include "hal/gpio_hal.h"
#include "driver/gpio.h"
#include "esp_clk_tree.h"
#include <inttypes.h>

static const char *TAG = "DEVICE_INFO";

void device_info_print_chip(void)
{
    ESP_LOGI(TAG, "=== Chip Information ===");
    
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    ESP_LOGI(TAG, "  Model: %s", CONFIG_IDF_TARGET);
    ESP_LOGI(TAG, "  Revision: %d", chip_info.revision);
    ESP_LOGI(TAG, "  Cores: %d", chip_info.cores);
    
    // Enhanced features display
    ESP_LOGI(TAG, "  Features:");
    if (chip_info.features & CHIP_FEATURE_WIFI_BGN) {
        ESP_LOGI(TAG, "    - WiFi 802.11 b/g/n");
    }
    if (chip_info.features & CHIP_FEATURE_BT) {
        ESP_LOGI(TAG, "    - Bluetooth Classic");
    }
    if (chip_info.features & CHIP_FEATURE_BLE) {
        ESP_LOGI(TAG, "    - Bluetooth Low Energy (BLE)");
    }
    if (chip_info.features & CHIP_FEATURE_EMB_FLASH) {
        ESP_LOGI(TAG, "    - Embedded Flash");
    } else {
        ESP_LOGI(TAG, "    - External Flash");
    }
    
    // Additional chip capabilities
    #ifdef CONFIG_IDF_TARGET_ESP32
    ESP_LOGI(TAG, "    - Dual-core Xtensa LX6");
    ESP_LOGI(TAG, "    - Hardware crypto acceleration");
    ESP_LOGI(TAG, "    - RTC with deep sleep support");
    #elif defined(CONFIG_IDF_TARGET_ESP32S2)
    ESP_LOGI(TAG, "    - Single-core Xtensa LX7");
    ESP_LOGI(TAG, "    - USB OTG support");
    ESP_LOGI(TAG, "    - Touch sensor support");
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
    ESP_LOGI(TAG, "    - Dual-core Xtensa LX7");
    ESP_LOGI(TAG, "    - USB OTG support");
    ESP_LOGI(TAG, "    - AI acceleration");
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
    ESP_LOGI(TAG, "    - Single-core RISC-V");
    ESP_LOGI(TAG, "    - IEEE 802.15.4 (Thread/Zigbee)");
    #endif
    
    // Flash information
    uint32_t flash_size;
    esp_flash_get_size(NULL, &flash_size);
    ESP_LOGI(TAG, "  Flash Size: %" PRIu32 " MB", flash_size / (1024 * 1024));
    
    // PSRAM information
    #ifdef CONFIG_SPIRAM_SUPPORT
    if (esp_psram_is_initialized()) {
        ESP_LOGI(TAG, "  PSRAM: Available (%" PRIu32 " bytes)", (uint32_t)esp_psram_get_size());
    } else {
        ESP_LOGI(TAG, "  PSRAM: Not available");
    }
    #else
    ESP_LOGI(TAG, "  PSRAM: Not configured");
    #endif
}

void device_info_print_mac_addresses(void)
{
    ESP_LOGI(TAG, "=== MAC Addresses ===");
    uint8_t mac[6];
    
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
        ESP_LOGI(TAG, "  WiFi STA: %02x:%02x:%02x:%02x:%02x:%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    
    if (esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP) == ESP_OK) {
        ESP_LOGI(TAG, "  WiFi AP:  %02x:%02x:%02x:%02x:%02x:%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    
    if (esp_read_mac(mac, ESP_MAC_BT) == ESP_OK) {
        ESP_LOGI(TAG, "  Bluetooth: %02x:%02x:%02x:%02x:%02x:%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
}

void device_info_print_memory(void)
{
    ESP_LOGI(TAG, "=== Memory Information ===");
    ESP_LOGI(TAG, "  Free heap: %" PRIu32 " bytes", (uint32_t)esp_get_free_heap_size());
    ESP_LOGI(TAG, "  Minimum free heap: %" PRIu32 " bytes", (uint32_t)esp_get_minimum_free_heap_size());
    ESP_LOGI(TAG, "  Internal RAM free: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));    ESP_LOGI(TAG, "  External RAM free: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

void device_info_print_hardware_features(void)
{
    ESP_LOGI(TAG, "=== Hardware Features & Capabilities ===");
    
    // SOC capabilities - using proper format specifiers
    ESP_LOGI(TAG, "  GPIO Count: %lu", (unsigned long)SOC_GPIO_PIN_COUNT);
    ESP_LOGI(TAG, "  ADC Units: %lu", (unsigned long)SOC_ADC_PERIPH_NUM);
    ESP_LOGI(TAG, "  DAC Channels: %lu", (unsigned long)SOC_DAC_CHAN_NUM);
    ESP_LOGI(TAG, "  SPI Controllers: %lu", (unsigned long)SOC_SPI_PERIPH_NUM);
    ESP_LOGI(TAG, "  I2C Controllers: %lu", (unsigned long)SOC_I2C_NUM);
    ESP_LOGI(TAG, "  UART Controllers: %lu", (unsigned long)SOC_UART_NUM);
    ESP_LOGI(TAG, "  Timer Groups: %lu", (unsigned long)SOC_TIMER_GROUP_TOTAL_TIMERS);
    ESP_LOGI(TAG, "  PWM Channels: %lu", (unsigned long)SOC_LEDC_CHANNEL_NUM);
      // Touch sensor capabilities
    #ifdef SOC_TOUCH_SENSOR_NUM
    ESP_LOGI(TAG, "  Touch Sensors: %lu", (unsigned long)SOC_TOUCH_SENSOR_NUM);
    #else
    ESP_LOGI(TAG, "  Touch Sensors: Not available");
    #endif
    
    // RTC GPIO
    #ifdef SOC_RTC_GPIO_PIN_COUNT
    ESP_LOGI(TAG, "  RTC GPIO Pins: %lu", (unsigned long)SOC_RTC_GPIO_PIN_COUNT);
    #else
    ESP_LOGI(TAG, "  RTC GPIO Pins: Not available");
    #endif
      // Crypto hardware
    ESP_LOGI(TAG, "  Hardware Crypto:");
    #ifdef SOC_AES_SUPPORT_AES_128
    ESP_LOGI(TAG, "    - AES-128/192/256 acceleration");
    #endif
    #ifdef SOC_SHA_SUPPORT_SHA1
    ESP_LOGI(TAG, "    - SHA-1/224/256 acceleration");
    #endif
    #ifdef SOC_RSA_MAX_BIT_LEN
    ESP_LOGI(TAG, "    - RSA acceleration (up to %lu bits)", (unsigned long)SOC_RSA_MAX_BIT_LEN);
    #endif
    
    // Communication protocols
    ESP_LOGI(TAG, "  Communication Protocols:");
    #ifdef SOC_TWAI_CONTROLLER_NUM
    ESP_LOGI(TAG, "    - CAN/TWAI Controllers: %lu", (unsigned long)SOC_TWAI_CONTROLLER_NUM);
    #endif
    #ifdef SOC_SDMMC_HOST_SUPPORTED
    ESP_LOGI(TAG, "    - SD/MMC Host: Supported");
    #endif
    #ifdef SOC_USB_OTG_SUPPORTED
    ESP_LOGI(TAG, "    - USB OTG: Supported");
    #endif
    #ifdef SOC_IEEE802154_SUPPORTED
    ESP_LOGI(TAG, "    - IEEE 802.15.4: Supported");
    #endif
    
    // Memory capabilities
    ESP_LOGI(TAG, "  Memory Capabilities:");    ESP_LOGI(TAG, "    - DMA capable memory: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_DMA));
    ESP_LOGI(TAG, "    - 32-bit aligned memory: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_32BIT));
    ESP_LOGI(TAG, "    - Executable memory: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_EXEC));
    ESP_LOGI(TAG, "    - RTC fast memory: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_RTCRAM));
}

void device_info_print_peripherals(void)
{
    ESP_LOGI(TAG, "=== Peripheral Information ===");
      // GPIO information
    ESP_LOGI(TAG, "  GPIO Configuration:");
    ESP_LOGI(TAG, "    - Total GPIO pins: %lu", (unsigned long)SOC_GPIO_PIN_COUNT);
    ESP_LOGI(TAG, "    - GPIO pins 0-%lu available for general use", (unsigned long)(SOC_GPIO_PIN_COUNT - 1));
      // ADC information
    ESP_LOGI(TAG, "  ADC Configuration:");
    #ifdef CONFIG_IDF_TARGET_ESP32
    ESP_LOGI(TAG, "    - ADC1 channels: 8 (GPIO32-39)");
    ESP_LOGI(TAG, "    - ADC2 channels: 10 (GPIO0,2,4,12-15,25-27)");
    ESP_LOGI(TAG, "    - ADC resolution: up to 12 bits");
    #elif defined(CONFIG_IDF_TARGET_ESP32S2)
    ESP_LOGI(TAG, "    - ADC1 channels: 10");
    ESP_LOGI(TAG, "    - ADC2 channels: 10");
    ESP_LOGI(TAG, "    - ADC resolution: up to 13 bits");
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
    ESP_LOGI(TAG, "    - ADC1 channels: 10");
    ESP_LOGI(TAG, "    - ADC2 channels: 10");
    ESP_LOGI(TAG, "    - ADC resolution: up to 12 bits");
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
    ESP_LOGI(TAG, "    - ADC1 channels: 5");
    ESP_LOGI(TAG, "    - ADC2 channels: 1");
    ESP_LOGI(TAG, "    - ADC resolution: up to 12 bits");
    #else
    ESP_LOGI(TAG, "    - ADC channels: Available");
    #endif
      // SPI information
    ESP_LOGI(TAG, "  SPI Configuration:");
    ESP_LOGI(TAG, "    - SPI controllers: %lu", (unsigned long)SOC_SPI_PERIPH_NUM);
    ESP_LOGI(TAG, "    - Max transfer size: %lu bytes", (unsigned long)SOC_SPI_MAXIMUM_BUFFER_SIZE);
    
    // I2C information
    ESP_LOGI(TAG, "  I2C Configuration:");
    ESP_LOGI(TAG, "    - I2C controllers: %lu", (unsigned long)SOC_I2C_NUM);
    ESP_LOGI(TAG, "    - Max clock speed: up to 1 MHz");
    
    // UART information
    ESP_LOGI(TAG, "  UART Configuration:");
    ESP_LOGI(TAG, "    - UART controllers: %lu", (unsigned long)SOC_UART_NUM);
    ESP_LOGI(TAG, "    - Hardware flow control: Supported");
    
    // PWM/LEDC information
    ESP_LOGI(TAG, "  PWM/LEDC Configuration:");
    ESP_LOGI(TAG, "    - PWM channels: %lu", (unsigned long)SOC_LEDC_CHANNEL_NUM);
    #ifdef CONFIG_IDF_TARGET_ESP32
    ESP_LOGI(TAG, "    - PWM resolution: up to 20 bits");
    #else
    ESP_LOGI(TAG, "    - PWM resolution: up to 14 bits");
    #endif
}

void device_info_print_clocks(void)
{
    ESP_LOGI(TAG, "=== Clock & Timing Information ===");
    
    // CPU frequency
    uint32_t cpu_freq_hz;
    esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX, &cpu_freq_hz);
    ESP_LOGI(TAG, "  CPU Frequency: %" PRIu32 " MHz", cpu_freq_hz / 1000000);
    
    // APB frequency
    uint32_t apb_freq_hz;
    esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_APB, ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX, &apb_freq_hz);
    ESP_LOGI(TAG, "  APB Frequency: %" PRIu32 " MHz", apb_freq_hz / 1000000);
    
    // XTAL frequency
    uint32_t xtal_freq_hz;
    esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_XTAL, ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX, &xtal_freq_hz);
    ESP_LOGI(TAG, "  XTAL Frequency: %" PRIu32 " MHz", xtal_freq_hz / 1000000);
    
    // RTC slow clock
    ESP_LOGI(TAG, "  RTC Slow Clock: ~150 kHz (internal RC)");
    ESP_LOGI(TAG, "  RTC Fast Clock: ~8 MHz (internal RC)");
    
    // Timer information
    ESP_LOGI(TAG, "  Timer Capabilities:");
    ESP_LOGI(TAG, "    - High resolution timer: 1 MHz (1μs resolution)");
    ESP_LOGI(TAG, "    - System tick: %d Hz", CONFIG_FREERTOS_HZ);
    ESP_LOGI(TAG, "    - Uptime: %" PRId64 " ms", esp_timer_get_time() / 1000);
}

static void print_partition_info(void)
{
    ESP_LOGI(TAG, "=== Partition Information ===");
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
    if (partition != NULL) {
        ESP_LOGI(TAG, "  Running partition: %s", partition->label);
        ESP_LOGI(TAG, "  Partition address: 0x%08" PRIx32, (uint32_t)partition->address);
        ESP_LOGI(TAG, "  Partition size: %" PRIu32 " bytes", (uint32_t)partition->size);
    }
    
    // Boot partition
    const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
    if (boot_partition != NULL) {
        ESP_LOGI(TAG, "  Boot partition: %s", boot_partition->label);
    }
}

static void print_system_info(void)
{
    ESP_LOGI(TAG, "=== System Information ===");
    ESP_LOGI(TAG, "  IDF Version: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "  Up time: %" PRId64 " us", esp_timer_get_time());
    ESP_LOGI(TAG, "  Reset reason: %d", esp_reset_reason());
    
    // CPU frequency
    ESP_LOGI(TAG, "  CPU Frequency: %d MHz", CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ);
    
    // Temperature (if available)
    #ifdef CONFIG_ESP32_PHY_CALIBRATION_AND_DATA_STORAGE
    // Note: Temperature sensor is not always available or accurate
    ESP_LOGI(TAG, "  Note: Temperature sensor reading not implemented in this example");
    #endif
}

static void print_security_info(void)
{
    ESP_LOGI(TAG, "=== Security Information ===");
    
    // Check if secure boot is enabled
    #ifdef CONFIG_SECURE_BOOT
    bool secure_boot_enabled = esp_secure_boot_enabled();
    ESP_LOGI(TAG, "  Security boot: %s", secure_boot_enabled ? "Enabled" : "Disabled");
    #else
    ESP_LOGI(TAG, "  Security boot: Disabled (not configured)");
    #endif
    
    // Check flash encryption status using efuse API
    #ifdef CONFIG_SECURE_FLASH_ENC_ENABLED
    ESP_LOGI(TAG, "  Flash encryption: Enabled");
    #else
    ESP_LOGI(TAG, "  Flash encryption: Disabled");
    #endif
}

void device_info_print_all(void)
{
    ESP_LOGI(TAG, "=== ESP32 Device Information ===");
    
    device_info_print_chip();
    device_info_print_mac_addresses();
    device_info_print_memory();
    device_info_print_hardware_features();
    device_info_print_peripherals();
    device_info_print_clocks();
    print_partition_info();
    print_system_info();
    print_security_info();
    
    ESP_LOGI(TAG, "=== End Device Information ===\n");
}
