#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

// Project modules
#include "wifi_manager.h"
#include "ping_manager.h"
#include "device_info.h"
#include "mqtt_manager.h"
#include "wol_manager.h"

static const char *TAG = "MAIN";

// Ping result callback function
void ping_result_handler(const char* name, const char* ip_address, bool success, uint32_t response_time, void* user_data)
{
    if (success) {
        ESP_LOGI(TAG, "✓ Ping to %s (%s) successful: %" PRIu32 " ms", name, ip_address, response_time);
    } else {
        ESP_LOGW(TAG, "✗ Ping to %s (%s) failed", name, ip_address);
    }
    
    // Send ping result via MQTT if connected
    mqtt_manager_send_ping_result(ip_address, success, response_time);
}

// MQTT message callback function
void mqtt_message_handler(const char* topic, const char* data, int data_len)
{
    ESP_LOGI(TAG, "MQTT message received on topic '%s': %.*s", topic, data_len, data);
      // Handle different command topics
    if (strstr(topic, "commands") != NULL) {
        if (strncmp(data, "device_info", data_len) == 0) {
            ESP_LOGI(TAG, "Executing device_info command");
            mqtt_manager_send_device_info();
        } else if (strncmp(data, "hello", data_len) == 0) {
            ESP_LOGI(TAG, "Executing hello command");
            mqtt_manager_send_hello();
        }
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
    
    // Wait a moment for WiFi to fully connect
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Initialize MQTT manager after WiFi is connected
    if (wifi_manager_is_connected()) {
        ESP_LOGI(TAG, "WiFi connected, initializing MQTT...");
        ret = mqtt_manager_init(mqtt_message_handler);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize MQTT manager");
        } else {
            ESP_LOGI(TAG, "MQTT manager initialized successfully");
        }
        
        // Wait for MQTT to connect
        int mqtt_wait_count = 0;
        while (!mqtt_manager_is_connected() && mqtt_wait_count < 30) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            mqtt_wait_count++;
            ESP_LOGI(TAG, "Waiting for MQTT connection... (%d/30)", mqtt_wait_count);
        }
          if (mqtt_manager_is_connected()) {
            ESP_LOGI(TAG, "MQTT connected successfully!");
            
            // Initialize Wake-on-LAN manager (which automatically adds devices to ping monitoring)
            ESP_LOGI(TAG, "Initializing Wake-on-LAN manager...");
            ret = wol_manager_init();
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to initialize WoL manager");
            } else {
                ESP_LOGI(TAG, "WoL manager initialized successfully - Device monitoring active");
                
                // Publish initial devices summary
                mqtt_publish_devices_summary();
            }
            
            // Send initial hello message
            mqtt_manager_send_hello();
            
            // Send status update
            mqtt_manager_send_status("ESP32 device online and ready");
        }    }
    
    // Main application loop
    int loop_count = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(120000)); // Wait 2 minutes
        loop_count++;
        
        ESP_LOGI(TAG, "=== Main Loop %d ===", loop_count);
          if (wifi_manager_is_connected() && mqtt_manager_is_connected()) {
            // Display device monitoring statistics every 2 minutes
            ESP_LOGI(TAG, "Device Monitoring Status:");
            int device_count;
            const wol_device_t* devices = wol_get_all_devices(&device_count);
            
            if (devices) {
                for (int i = 0; i < device_count; i++) {
                    const ping_target_t* ping_info = ping_manager_get_device(devices[i].name);
                    if (ping_info) {
                        ESP_LOGI(TAG, "  %s (%s): %s - Success=%" PRIu32 ", Fail=%" PRIu32 ", Success Rate=%.1f%%", 
                                devices[i].name, devices[i].ip_address,
                                wol_get_status_string(devices[i].status),
                                ping_info->success_count,
                                ping_info->fail_count,
                                ping_info->success_count + ping_info->fail_count > 0 ? 
                                    (100.0 * ping_info->success_count) / (ping_info->success_count + ping_info->fail_count) : 0.0);
                    } else {
                        ESP_LOGI(TAG, "  %s (%s): %s - No ping data", 
                                devices[i].name, devices[i].ip_address, wol_get_status_string(devices[i].status));
                    }
                }
            } else {
                ESP_LOGW(TAG, "No devices configured for monitoring");
            }
            
            // Send status update every 5 loops (10 minutes)
            if (loop_count % 5 == 0) {
                mqtt_manager_send_status("System running - Device monitoring active");
                mqtt_publish_devices_summary();
            }
        } else {
            ESP_LOGW(TAG, "WiFi or MQTT disconnected - device monitoring paused");
        }
    }
}