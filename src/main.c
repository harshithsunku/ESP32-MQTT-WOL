#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

// Project modules
#include "wifi_manager.h"
#include "ping_manager.h"
#include "device_info.h"

static const char *TAG = "MAIN";

// Ping result callback function
void ping_result_handler(const char* ip_address, bool success, uint32_t response_time, void* user_data)
{
    if (success) {
        ESP_LOGI(TAG, "✓ Ping to %s successful: %" PRIu32 " ms", ip_address, response_time);
    } else {
        ESP_LOGW(TAG, "✗ Ping to %s failed", ip_address);
    }
}

void app_main(void)
{
    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);    // Print device information first
    device_info_print_all();
    
    // Initialize ping manager with callback
    ret = ping_manager_init(ping_result_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ping manager");
        return;
    }
    
    // Initialize and connect to WiFi
    ESP_LOGI(TAG, "Starting WiFi connection...");
    ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi manager");
        ping_manager_deinit();
        return;
    }
    
    // Test one-time ping to Google first
    if (wifi_manager_is_connected()) {
        ESP_LOGI(TAG, "Testing one-time ping to Google DNS...");
        ping_manager_ping_google();
        
        // Add some targets for continuous ping monitoring
        ESP_LOGI(TAG, "Adding continuous ping targets...");
        
        // Add Google DNS with 10 second interval
        int google_idx = ping_manager_add_target("8.8.8.8", 10000, 3000, 1);
        ESP_LOGI(TAG, "Added Google DNS at index: %d", google_idx);
        
        // Add Cloudflare DNS with 15 second interval
        int cloudflare_idx = ping_manager_add_target("1.1.1.1", 15000, 3000, 1);
        ESP_LOGI(TAG, "Added Cloudflare DNS at index: %d", cloudflare_idx);
        
        // Add local gateway with 5 second interval
        int gateway_idx = ping_manager_add_target("192.168.0.1", 5000, 2000, 1);
        ESP_LOGI(TAG, "Added gateway at index: %d", gateway_idx);
        
    } else {
        ESP_LOGE(TAG, "WiFi not connected, skipping ping setup");
    }
      // Main application loop
    int loop_count = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000)); // Wait 30 seconds
        loop_count++;
        
        ESP_LOGI(TAG, "=== Main Loop %d ===", loop_count);
        
        if (wifi_manager_is_connected()) {
            // Print ping statistics every 2 minutes (4 loops)
            if (loop_count % 4 == 0) {
                ESP_LOGI(TAG, "Ping Statistics:");
                ping_target_t targets[PING_MAX_TARGETS];
                int count;
                  if (ping_manager_get_all_targets(targets, &count) == ESP_OK) {
                    for (int i = 0; i < count; i++) {
                        ESP_LOGI(TAG, "  %s: Success=%" PRIu32 ", Fail=%" PRIu32 ", Success Rate=%.1f%%", 
                                targets[i].ip_address,
                                targets[i].success_count,
                                targets[i].fail_count,
                                targets[i].success_count + targets[i].fail_count > 0 ? 
                                    (100.0 * targets[i].success_count) / (targets[i].success_count + targets[i].fail_count) : 0.0);
                    }
                } else {
                    ESP_LOGW(TAG, "Failed to get ping statistics");
                }
            }
            
            // Demonstrate dynamic target management
            if (loop_count == 6) {
                ESP_LOGI(TAG, "Adding additional target: 8.8.4.4");
                ping_manager_add_target("8.8.4.4", 20000, 3000, 1);
            }
            
            if (loop_count == 12) {
                ESP_LOGI(TAG, "Disabling gateway ping temporarily");
                ping_manager_set_target_enabled(2, false); // Assuming gateway is at index 2
            }
            
            if (loop_count == 18) {
                ESP_LOGI(TAG, "Re-enabling gateway ping");
                ping_manager_set_target_enabled(2, true);
            }
            
        } else {
            ESP_LOGW(TAG, "WiFi disconnected - continuous ping targets will be skipped");
        }
    }
}