#ifndef CONFIG_TEMPLATE_H
#define CONFIG_TEMPLATE_H

/**
 * @file config_template.h
 * @brief Template configuration file for ESP32 MQTT project
 * 
 * IMPORTANT: 
 * 1. Copy this file to 'secrets.h' in the same directory
 * 2. Fill in your actual credentials in 'secrets.h'
 * 3. Never commit 'secrets.h' to version control
 */

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// WiFi Configuration
// ============================================================================
#define WIFI_SSID_TEMPLATE          "Your_WiFi_SSID_Here"
#define WIFI_PASSWORD_TEMPLATE      "Your_WiFi_Password_Here"

// ============================================================================
// MQTT Configuration
// ============================================================================
// HiveMQ Cloud broker details
#define MQTT_BROKER_HOST_TEMPLATE   "your-hivemq-cluster.s1.eu.hivemq.cloud"
#define MQTT_BROKER_PORT_TEMPLATE   8883
#define MQTT_USERNAME_TEMPLATE      "your_mqtt_username"
#define MQTT_PASSWORD_TEMPLATE      "your_mqtt_password"
#define MQTT_CLIENT_ID_TEMPLATE     "ESP32_IoT_Device_001"

// ============================================================================
// Device Configuration
// ============================================================================
#define DEVICE_NAME_TEMPLATE        "ESP32_IoT_Device"
#define DEVICE_LOCATION_TEMPLATE    "Home_Lab"

// ============================================================================
// MQTT Topics Configuration
// ============================================================================
#define MQTT_TOPIC_PREFIX_TEMPLATE  "esp32"

// Derived topic names (usually don't need to change these)
#define MQTT_TOPIC_HELLO_TEMPLATE       MQTT_TOPIC_PREFIX_TEMPLATE "/hello"
#define MQTT_TOPIC_STATUS_TEMPLATE      MQTT_TOPIC_PREFIX_TEMPLATE "/status"
#define MQTT_TOPIC_DEVICE_INFO_TEMPLATE MQTT_TOPIC_PREFIX_TEMPLATE "/device_info"
#define MQTT_TOPIC_PING_RESULTS_TEMPLATE MQTT_TOPIC_PREFIX_TEMPLATE "/ping"
#define MQTT_TOPIC_COMMANDS_TEMPLATE    MQTT_TOPIC_PREFIX_TEMPLATE "/commands"

// ============================================================================
// Network Configuration
// ============================================================================
#define MQTT_KEEPALIVE_TEMPLATE     60
#define MQTT_TIMEOUT_MS_TEMPLATE    10000
#define WIFI_RETRY_COUNT_TEMPLATE   10

#ifdef __cplusplus
}
#endif

#endif // CONFIG_TEMPLATE_H
