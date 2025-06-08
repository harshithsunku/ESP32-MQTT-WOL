#include "device_info.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_mac.h"
#include "esp_partition.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"
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
    ESP_LOGI(TAG, "  Features: %s%s%s%s",
             (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
             (chip_info.features & CHIP_FEATURE_BT) ? "BT/" : "",
             (chip_info.features & CHIP_FEATURE_BLE) ? "BLE/" : "",
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "Embedded Flash" : "External Flash");
    
    // Flash information
    uint32_t flash_size;
    esp_flash_get_size(NULL, &flash_size);
    ESP_LOGI(TAG, "  Flash Size: %" PRIu32 " MB", flash_size / (1024 * 1024));
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
    ESP_LOGI(TAG, "  Internal RAM free: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    ESP_LOGI(TAG, "  External RAM free: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
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
    print_partition_info();
    print_system_info();
    print_security_info();
    
    ESP_LOGI(TAG, "=== End Device Information ===\n");
}
