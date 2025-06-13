#ifndef PING_MANAGER_H
#define PING_MANAGER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Constants
#define PING_MAX_TARGETS        20
#define PING_DEFAULT_INTERVAL   10000  // 10 seconds
#define PING_DEFAULT_TIMEOUT    3000   // 3 seconds  
#define PING_DEFAULT_COUNT      1      // 1 ping per cycle

// Ping target structure
typedef struct {
    char name[32];                   // Device name for identification
    char ip_address[16];             // IP address to ping
    uint32_t interval_ms;            // Ping interval in milliseconds
    uint32_t timeout_ms;             // Ping timeout in milliseconds
    uint32_t count;                  // Number of pings per cycle
    bool enabled;                    // Whether this target is active
    bool is_online;                  // Current online status
    uint32_t success_count;          // Total successful pings
    uint32_t fail_count;             // Total failed pings
    uint64_t last_ping_time;         // Last ping timestamp
    uint64_t last_success_time;      // Last successful ping timestamp
} ping_target_t;

// Ping result callback function type
typedef void (*ping_result_callback_t)(const char* name, const char* ip_address, bool success, uint32_t response_time, void* user_data);

// Function prototypes

/**
 * @brief Initialize the ping manager
 * @param callback Optional callback function for ping results
 * @param user_data Optional user data for callback
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ping_manager_init(ping_result_callback_t callback, void* user_data);

/**
 * @brief Deinitialize the ping manager
 */
void ping_manager_deinit(void);

/**
 * @brief Add a device for monitoring
 * @param name Device name for identification
 * @param ip_address IP address to ping
 * @return Target index on success, -1 on failure
 */
int ping_manager_add_device(const char* name, const char* ip_address);

/**
 * @brief Remove a ping target by name
 * @param name Device name to remove
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ping_manager_remove_device(const char* name);

/**
 * @brief Enable or disable a ping target by name
 * @param name Device name to modify
 * @param enabled True to enable, false to disable
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ping_manager_set_device_enabled(const char* name, bool enabled);

/**
 * @brief Get ping target information by name
 * @param name Device name to query
 * @return Pointer to target structure, NULL on failure
 */
const ping_target_t* ping_manager_get_device(const char* name);

/**
 * @brief Get total number of ping targets
 * @return Number of configured targets
 */
int ping_manager_get_target_count(void);

/**
 * @brief Check if ping manager is running
 * @return True if running, false otherwise
 */
bool ping_manager_is_running(void);

#ifdef __cplusplus
}
#endif

#endif // PING_MANAGER_H
