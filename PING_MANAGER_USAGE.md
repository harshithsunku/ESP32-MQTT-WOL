# Ping Manager Usage Examples

This document demonstrates how to use the advanced ping manager with continuous ping capabilities.

## Basic Usage

### 1. Initialize Ping Manager

```c
#include "ping_manager.h"

// Define a callback function to receive ping results
void ping_result_handler(const char* ip_address, bool success, uint32_t response_time, void* user_data)
{
    if (success) {
        ESP_LOGI("APP", "✓ Ping to %s successful: %d ms", ip_address, response_time);
    } else {
        ESP_LOGW("APP", "✗ Ping to %s failed", ip_address);
    }
}

// Initialize with callback
esp_err_t ret = ping_manager_init(ping_result_handler, NULL);
if (ret != ESP_OK) {
    ESP_LOGE("APP", "Failed to initialize ping manager");
}
```

### 2. Add Continuous Ping Targets

```c
// Add Google DNS with 10 second interval
int google_idx = ping_manager_add_target("8.8.8.8", 10000, 3000, 1);

// Add Cloudflare DNS with 15 second interval  
int cloudflare_idx = ping_manager_add_target("1.1.1.1", 15000, 3000, 1);

// Add local gateway with 5 second interval
int gateway_idx = ping_manager_add_target("192.168.0.1", 5000, 2000, 1);

// Add custom server with 30 second interval
int server_idx = ping_manager_add_target("192.168.1.100", 30000, 5000, 2);
```

### 3. Manage Targets Dynamically

```c
// Temporarily disable a target
ping_manager_set_target_enabled(gateway_idx, false);

// Re-enable the target
ping_manager_set_target_enabled(gateway_idx, true);

// Update target configuration
ping_manager_update_target(google_idx, 20000, 4000, 2); // New interval, timeout, count

// Remove a target
ping_manager_remove_target(server_idx);
```

### 4. Get Statistics

```c
// Get statistics for a specific target
ping_target_t target_stats;
if (ping_manager_get_target_stats(google_idx, &target_stats) == ESP_OK) {
    ESP_LOGI("APP", "Target %s: Success=%d, Fail=%d", 
             target_stats.ip_address,
             target_stats.success_count, 
             target_stats.fail_count);
}

// Get all targets
ping_target_t all_targets[PING_MAX_TARGETS];
int count;
if (ping_manager_get_all_targets(all_targets, &count) == ESP_OK) {
    for (int i = 0; i < count; i++) {
        float success_rate = 0.0;
        int total = all_targets[i].success_count + all_targets[i].fail_count;
        if (total > 0) {
            success_rate = (100.0 * all_targets[i].success_count) / total;
        }
        ESP_LOGI("APP", "%s: %.1f%% success rate", 
                 all_targets[i].ip_address, success_rate);
    }
}
```

### 5. One-time Pings

```c
// One-time ping (doesn't affect continuous ping targets)
ping_manager_ping_once("192.168.1.1", 3, 2000);

// Quick ping to Google
ping_manager_ping_google();
```

## Advanced Configuration

### Target Configuration Parameters

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
