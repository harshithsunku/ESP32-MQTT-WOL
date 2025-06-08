#ifndef PING_MANAGER_H
#define PING_MANAGER_H

#include "ping/ping_sock.h"
#include "lwip/ip_addr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define PING_MAX_TARGETS        10
#define PING_MAX_IP_LEN         16
#define PING_DEFAULT_INTERVAL   5000    // 5 seconds
#define PING_DEFAULT_TIMEOUT    3000    // 3 seconds
#define PING_DEFAULT_COUNT      1       // Single ping per cycle

/**
 * @brief Ping target configuration structure
 */
typedef struct {
    char ip_address[PING_MAX_IP_LEN];   ///< Target IP address
    uint32_t interval_ms;               ///< Ping interval in milliseconds
    uint32_t timeout_ms;                ///< Ping timeout in milliseconds
    uint32_t count;                     ///< Number of pings per cycle
    bool enabled;                       ///< Whether this target is active
    uint64_t last_ping_time;           ///< Last ping timestamp (in microseconds)
    uint32_t success_count;            ///< Total successful pings
    uint32_t fail_count;               ///< Total failed pings
} ping_target_t;

/**
 * @brief Ping command types for the ping thread
 */
typedef enum {
    PING_CMD_ADD_TARGET,
    PING_CMD_REMOVE_TARGET,
    PING_CMD_UPDATE_TARGET,
    PING_CMD_ENABLE_TARGET,
    PING_CMD_DISABLE_TARGET,
    PING_CMD_STOP_ALL
} ping_cmd_type_t;

/**
 * @brief Ping command structure
 */
typedef struct {
    ping_cmd_type_t cmd_type;
    ping_target_t target;
    int target_index;
} ping_cmd_t;

/**
 * @brief Ping result callback function type
 * @param ip_address Target IP address
 * @param success Whether ping was successful
 * @param response_time Response time in milliseconds (0 if failed)
 * @param user_data User data passed during initialization
 */
typedef void (*ping_result_callback_t)(const char* ip_address, bool success, uint32_t response_time, void* user_data);

/**
 * @brief Initialize ping manager with continuous ping thread
 * @param result_callback Callback function for ping results (optional)
 * @param user_data User data to pass to callback (optional)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ping_manager_init(ping_result_callback_t result_callback, void* user_data);

/**
 * @brief Add a target IP to continuous ping list
 * @param ip_address Target IP address string
 * @param interval_ms Ping interval in milliseconds (0 for default)
 * @param timeout_ms Ping timeout in milliseconds (0 for default)
 * @param count Number of pings per cycle (0 for default)
 * @return Target index on success, -1 on failure
 */
int ping_manager_add_target(const char* ip_address, uint32_t interval_ms, uint32_t timeout_ms, uint32_t count);

/**
 * @brief Remove a target from continuous ping list
 * @param target_index Index of target to remove
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ping_manager_remove_target(int target_index);

/**
 * @brief Update target configuration
 * @param target_index Index of target to update
 * @param interval_ms New ping interval in milliseconds (0 to keep current)
 * @param timeout_ms New ping timeout in milliseconds (0 to keep current)
 * @param count New number of pings per cycle (0 to keep current)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ping_manager_update_target(int target_index, uint32_t interval_ms, uint32_t timeout_ms, uint32_t count);

/**
 * @brief Enable/disable a ping target
 * @param target_index Index of target
 * @param enabled True to enable, false to disable
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ping_manager_set_target_enabled(int target_index, bool enabled);

/**
 * @brief Get target statistics
 * @param target_index Index of target
 * @param target Pointer to store target information
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ping_manager_get_target_stats(int target_index, ping_target_t* target);

/**
 * @brief Get list of all targets
 * @param targets Array to store targets (must be at least PING_MAX_TARGETS size)
 * @param count Pointer to store actual number of targets
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ping_manager_get_all_targets(ping_target_t targets[], int* count);

/**
 * @brief One-time ping to a specific host (non-continuous)
 * @param target_ip Target IP address string (e.g., "8.8.8.8")
 * @param count Number of ping packets to send
 * @param timeout_ms Timeout in milliseconds
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ping_manager_ping_once(const char* target_ip, uint32_t count, uint32_t timeout_ms);

/**
 * @brief Quick ping to Google's DNS server (8.8.8.8) - one time
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ping_manager_ping_google(void);

/**
 * @brief Stop all ping operations and deinitialize
 */
void ping_manager_deinit(void);

#endif // PING_MANAGER_H
