#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/event_groups.h"

// WiFi configuration
#define WIFI_SSID      "HighSpeedFiberHome"
#define WIFI_PASS      "123456789000"
#define MAXIMUM_RETRY  5

// Event bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/**
 * @brief Initialize WiFi in station mode and connect to AP
 * @return ESP_OK on success, error code on failure
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Check if WiFi is connected
 * @return true if connected, false otherwise
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Deinitialize WiFi manager
 */
void wifi_manager_deinit(void);

#endif // WIFI_MANAGER_H
