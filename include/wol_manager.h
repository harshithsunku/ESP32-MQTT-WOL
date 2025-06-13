#ifndef WOL_MANAGER_H
#define WOL_MANAGER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of devices that can be managed
#define MAX_DEVICES 20

// Device status definitions
typedef enum {
    DEVICE_STATUS_UNKNOWN = 0,
    DEVICE_STATUS_ONLINE,
    DEVICE_STATUS_OFFLINE,
    DEVICE_STATUS_WAKING
} device_status_t;

// Device information structure
typedef struct {
    char name[32];              // Device name (e.g., "server1", "desktop1")
    char ip_address[16];        // IP address (e.g., "192.168.0.111")
    uint8_t mac_address[6];     // MAC address for WoL
    char description[64];       // Device description
    device_status_t status;     // Current device status
    uint32_t last_ping_time;    // Last successful ping timestamp
    bool enabled;               // Whether device monitoring is enabled
    uint16_t wol_port;          // WoL port (usually 9)
} wol_device_t;

/**
 * @brief Initialize the Wake-on-LAN manager
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wol_manager_init(void);

/**
 * @brief Add a device to the monitoring list
 * 
 * @param name Device name (unique identifier)
 * @param ip_address IP address string
 * @param mac_address MAC address (6 bytes)
 * @param description Device description
 * @return esp_err_t ESP_OK on success, ESP_ERR_NO_MEM if list is full
 */
esp_err_t wol_add_device(const char* name, const char* ip_address, 
                        const uint8_t* mac_address, const char* description);

/**
 * @brief Remove a device from the monitoring list
 * 
 * @param name Device name to remove
 * @return esp_err_t ESP_OK on success, ESP_ERR_NOT_FOUND if device not found
 */
esp_err_t wol_remove_device(const char* name);

/**
 * @brief Send Wake-on-LAN packet to a specific device
 * 
 * @param name Device name to wake up
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wol_wake_device(const char* name);

/**
 * @brief Send Wake-on-LAN packet using MAC address directly
 * 
 * @param mac_address MAC address (6 bytes)
 * @param broadcast_ip Broadcast IP address (can be NULL for default)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wol_send_packet(const uint8_t* mac_address, const char* broadcast_ip);

/**
 * @brief Update device status (called by ping manager)
 * 
 * @param name Device name
 * @param is_online Whether device responded to ping
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wol_update_device_status(const char* name, bool is_online);

/**
 * @brief Get device information by name
 * 
 * @param name Device name
 * @return wol_device_t* Pointer to device info, NULL if not found
 */
const wol_device_t* wol_get_device(const char* name);

/**
 * @brief Get all devices list
 * 
 * @param device_count Pointer to store the number of devices
 * @return const wol_device_t* Pointer to devices array
 */
const wol_device_t* wol_get_all_devices(int* device_count);

/**
 * @brief Enable/disable device monitoring
 * 
 * @param name Device name
 * @param enabled Enable or disable monitoring
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wol_set_device_enabled(const char* name, bool enabled);

/**
 * @brief Get device status as string
 * 
 * @param status Device status enum
 * @return const char* Status string
 */
const char* wol_get_status_string(device_status_t status);

/**
 * @brief Load device list from configuration
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wol_load_device_config(void);

/**
 * @brief Save device list to configuration
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wol_save_device_config(void);

/**
 * @brief Process MQTT WoL command
 * 
 * @param device_name Device name from MQTT message
 * @param command Command string ("on", "wake", "status")
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wol_handle_mqtt_command(const char* device_name, const char* command);

#ifdef __cplusplus
}
#endif

#endif // WOL_MANAGER_H
