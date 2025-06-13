# Simplified Ping Manager Usage

This document demonstrates how to use the simplified ping manager library for device connectivity monitoring in the ESP32 MQTT Wake-on-LAN project.

## Overview

The ping manager has been redesigned as a simple library service that:
- Uses TCP connection attempts instead of ICMP pings for better reliability
- Automatically integrates with the WoL manager
- Provides device connectivity status via callbacks
- Uses thread-safe operations with FreeRTOS

## Basic Usage

### 1. Initialize Ping Manager

```c
#include "ping_manager.h"

// Define a callback function to receive ping results
void ping_result_handler(const char* name, const char* ip_address, bool success, uint32_t response_time, void* user_data)
{
    if (success) {
        ESP_LOGI("APP", "✓ Device %s (%s) is online: %d ms", name, ip_address, response_time);
    } else {
        ESP_LOGW("APP", "✗ Device %s (%s) is offline", name, ip_address);
    }
}

// Initialize with callback
esp_err_t ret = ping_manager_init(ping_result_handler, NULL);
if (ret != ESP_OK) {
    ESP_LOGE("APP", "Failed to initialize ping manager");
}
```

### 2. Add Devices for Monitoring

```c
// Add devices with descriptive names
int server_idx = ping_manager_add_device("server1", "192.168.0.111");
int desktop_idx = ping_manager_add_device("desktop1", "192.168.0.112");
int nas_idx = ping_manager_add_device("nas1", "192.168.0.113");

if (server_idx >= 0) {
    ESP_LOGI("APP", "Added server1 for monitoring");
}
```

### 3. Manage Device Monitoring

```c
// Temporarily disable monitoring for a device
ping_manager_set_device_enabled("server1", false);

// Re-enable monitoring
ping_manager_set_device_enabled("server1", true);

// Remove a device from monitoring
ping_manager_remove_device("desktop1");
```

### 4. Get Device Status and Statistics

```c
// Get status for a specific device
const ping_target_t* device = ping_manager_get_device("server1");
if (device) {
    ESP_LOGI("APP", "Device %s: %s (Success: %d, Fail: %d)", 
             device->name,
             device->is_online ? "ONLINE" : "OFFLINE",
             device->success_count,
             device->fail_count);
}

// Check how many devices are being monitored
int total_devices = ping_manager_get_target_count();
ESP_LOGI("APP", "Monitoring %d devices", total_devices);
```

## Integration with WoL Manager

The ping manager is automatically used by the WoL manager. When you add devices to the WoL manager, they are automatically added to ping monitoring:

```c
#include "wol_manager.h"

// Initialize WoL manager (automatically initializes ping manager)
wol_manager_init();

// Add a device to WoL (automatically adds to ping monitoring)
uint8_t mac[] = {0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E};
wol_add_device("server1", "192.168.0.111", mac, "Main Server");

// The device is now being monitored for connectivity
// Status changes will be reported via MQTT automatically
```

## TCP Connection Testing

The ping manager uses TCP connection attempts to check host availability:

1. **Primary Check**: Attempts to connect to port 80 (HTTP)
2. **Fallback Check**: If port 80 fails, tries port 22 (SSH)
3. **Timeout Handling**: Configurable timeout (default 3 seconds)
4. **Non-blocking**: Uses non-blocking sockets with select()

This approach is more reliable than ICMP pings on many networks where ICMP may be blocked.

## Configuration Parameters

```c
// Constants defined in ping_manager.h
#define PING_MAX_TARGETS        20      // Maximum devices to monitor
#define PING_DEFAULT_INTERVAL   10000   // 10 seconds between checks
#define PING_DEFAULT_TIMEOUT    3000    // 3 second timeout
#define PING_DEFAULT_COUNT      1       // 1 ping per cycle
```

## Thread Safety

The ping manager is fully thread-safe:
- Uses FreeRTOS mutex for data protection
- Safe to call from multiple tasks
- Callback function is called from ping task context

## Memory Usage

- **Static Memory**: ~2KB for device storage (20 devices × ~100 bytes each)
- **Task Stack**: 4KB for ping monitoring task
- **Dynamic Memory**: Minimal (mutex and temporary socket buffers)

## Best Practices

1. **Initialize Early**: Initialize ping manager before adding devices
2. **Handle Callbacks**: Always implement the callback function for status updates
3. **Error Checking**: Check return values for all API calls
4. **Resource Cleanup**: Call `ping_manager_deinit()` when shutting down
5. **Network Ready**: Ensure WiFi is connected before initializing

## Example: Complete Integration

```c
#include "wifi_manager.h"
#include "ping_manager.h"
#include "wol_manager.h"
#include "mqtt_manager.h"

void ping_status_callback(const char* name, const char* ip_address, bool success, uint32_t response_time, void* user_data) {
    // Update WoL manager with device status
    wol_update_device_status(name, success);
    
    // Log status changes
    ESP_LOGI("PING", "Device %s (%s): %s", name, ip_address, success ? "ONLINE" : "OFFLINE");
}

void app_main(void) {
    // Initialize NVS
    nvs_flash_init();
    
    // Connect to WiFi
    wifi_manager_init();
    
    // Initialize managers in order
    ping_manager_init(ping_status_callback, NULL);
    wol_manager_init();  // This will add devices to ping monitoring
    mqtt_manager_init();
    
    // Main loop - device monitoring is automatic
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(60000)); // Check every minute
        
        // Optional: Log statistics
        int device_count = ping_manager_get_target_count();
        ESP_LOGI("APP", "Monitoring %d devices", device_count);
    }
}
```

- **interval_ms**: Time between ping cycles (minimum recommended: 1000ms)
- **timeout_ms**: Maximum time to wait for ping response
- **count**: Number of ping packets per cycle
- **enabled**: Whether the target is active

### Data Structure

```c
typedef struct {
    char ip_address[16];        // Target IP address
    uint32_t interval_ms;       // Ping interval in milliseconds
    uint32_t timeout_ms;        // Ping timeout in milliseconds
    uint32_t count;             // Number of pings per cycle
    bool enabled;               // Whether this target is active
    uint64_t last_ping_time;    // Last ping timestamp (microseconds)
    uint32_t success_count;     // Total successful pings
    uint32_t fail_count;        // Total failed pings
} ping_target_t;
```

## Complete Example

```c
#include "ping_manager.h"
#include "esp_log.h"

static const char *TAG = "PING_EXAMPLE";

void ping_callback(const char* ip, bool success, uint32_t time, void* data)
{
    if (success) {
        ESP_LOGI(TAG, "✓ %s responded in %d ms", ip, time);
    } else {
        ESP_LOGW(TAG, "✗ %s timeout", ip);
    }
}

void setup_continuous_ping(void)
{
    // Initialize ping manager
    ESP_ERROR_CHECK(ping_manager_init(ping_callback, NULL));
    
    // Add monitoring targets
    int dns1 = ping_manager_add_target("8.8.8.8", 10000, 3000, 1);      // Google DNS
    int dns2 = ping_manager_add_target("1.1.1.1", 12000, 3000, 1);      // Cloudflare DNS
    int gw = ping_manager_add_target("192.168.0.1", 5000, 2000, 1);     // Gateway
    
    ESP_LOGI(TAG, "Added ping targets: DNS1=%d, DNS2=%d, Gateway=%d", dns1, dns2, gw);
    
    // Monitor and report statistics periodically
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(60000)); // Wait 1 minute
        
        ping_target_t targets[PING_MAX_TARGETS];
        int count;
        
        if (ping_manager_get_all_targets(targets, &count) == ESP_OK) {
            ESP_LOGI(TAG, "=== Ping Statistics ===");
            for (int i = 0; i < count; i++) {
                int total = targets[i].success_count + targets[i].fail_count;
                if (total > 0) {
                    float rate = (100.0 * targets[i].success_count) / total;
                    ESP_LOGI(TAG, "%s: %.1f%% (%d/%d)", 
                             targets[i].ip_address, rate, 
                             targets[i].success_count, total);
                }
            }
        }
    }
}
```

## Benefits

1. **Continuous Monitoring**: Automatic ping monitoring in background
2. **Multiple Targets**: Monitor up to 10 different IP addresses simultaneously
3. **Individual Intervals**: Each target can have different ping intervals
4. **Real-time Callbacks**: Immediate notification of ping results
5. **Statistics Tracking**: Automatic success/failure rate tracking
6. **Dynamic Management**: Add/remove/modify targets at runtime
7. **Thread-safe**: All operations are thread-safe using FreeRTOS synchronization

## Resource Usage

- **RAM**: ~2KB for ping targets data structure
- **Task Stack**: 4KB for ping task
- **CPU**: Minimal overhead, ping operations run in background
- **Network**: Configurable based on ping intervals and target count
