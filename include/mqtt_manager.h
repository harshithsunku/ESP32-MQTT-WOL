#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "esp_err.h"
#include "mqtt_client.h"
#include "secrets.h"  // Include secrets for configuration

#ifdef __cplusplus
extern "C" {
#endif

// MQTT Quality of Service levels
typedef enum {
    MQTT_QOS_0 = 0,  // At most once
    MQTT_QOS_1 = 1,  // At least once
    MQTT_QOS_2 = 2   // Exactly once
} mqtt_qos_t;

// MQTT Message callback type
typedef void (*mqtt_message_callback_t)(const char* topic, const char* data, int data_len);

/**
 * @brief Initialize MQTT manager
 * @param message_callback Callback function for received messages (can be NULL)
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t mqtt_manager_init(mqtt_message_callback_t message_callback);

/**
 * @brief Deinitialize MQTT manager
 */
void mqtt_manager_deinit(void);

/**
 * @brief Check if MQTT client is connected
 * @return true if connected, false otherwise
 */
bool mqtt_manager_is_connected(void);

/**
 * @brief Send a hello message
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t mqtt_manager_send_hello(void);

/**
 * @brief Send device status message
 * @param status Status message to send
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t mqtt_manager_send_status(const char* status);

/**
 * @brief Send device information
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t mqtt_manager_send_device_info(void);

/**
 * @brief Send ping result
 * @param ip_address Target IP address
 * @param success Ping success status
 * @param response_time Response time in milliseconds
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t mqtt_manager_send_ping_result(const char* ip_address, bool success, uint32_t response_time);

/**
 * @brief Publish a message to a topic
 * @param topic MQTT topic
 * @param data Message data
 * @param qos Quality of Service level
 * @param retain Retain message flag
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t mqtt_manager_publish(const char* topic, const char* data, mqtt_qos_t qos, bool retain);

/**
 * @brief Subscribe to a topic
 * @param topic MQTT topic to subscribe to
 * @param qos Quality of Service level
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t mqtt_manager_subscribe(const char* topic, mqtt_qos_t qos);

/**
 * @brief Publish MQTT message to a topic
 * @param topic MQTT topic
 * @param message Message to publish
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t mqtt_publish(const char* topic, const char* message);

/**
 * @brief Subscribe to WoL command topics
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t mqtt_subscribe_wol_commands(void);

/**
 * @brief Handle WoL MQTT commands
 * @param topic MQTT topic
 * @param data Message data
 * @param data_len Length of message data
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t mqtt_handle_wol_command(const char* topic, const char* data, int data_len);

/**
 * @brief Publish device status to MQTT
 * @param device_name Device name
 * @param is_online Online status
 * @param ip_address Device IP address
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t mqtt_publish_device_status(const char* device_name, bool is_online, const char* ip_address);

/**
 * @brief Publish all devices status summary
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t mqtt_publish_devices_summary(void);

#ifdef __cplusplus
}
#endif

#endif // MQTT_MANAGER_H
