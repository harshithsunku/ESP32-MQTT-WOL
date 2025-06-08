#ifndef SECRETS_H
#define SECRETS_H

/**
 * @file secrets.h
 * @brief Configuration template for ESP32 MQTT project
 * 
 * INSTRUCTIONS:
 * 1. Copy this file to 'secrets.h' in the same directory
 * 2. Replace all placeholder values with your actual credentials
 * 3. Never commit secrets.h to Git (it's in .gitignore)
 * 
 * SECURITY WARNING:
 * This file will contain sensitive information like WiFi passwords and MQTT credentials.
 * Make sure secrets.h is in your .gitignore file!
 */

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// WiFi Configuration
// ============================================================================
#define WIFI_SSID          "YOUR_WIFI_SSID"        // Replace with your WiFi network name
#define WIFI_PASSWORD      "YOUR_WIFI_PASSWORD"    // Replace with your WiFi password

// ============================================================================
// MQTT Configuration - HiveMQ Cloud (or your MQTT broker)
// ============================================================================
#define MQTT_BROKER_HOST   "your-broker.hivemq.cloud"  // Replace with your MQTT broker URL
#define MQTT_BROKER_PORT   8883                         // Standard MQTTS port (secure)
#define MQTT_USERNAME      "your_mqtt_username"         // Replace with your MQTT username
#define MQTT_PASSWORD      "your_mqtt_password"         // Replace with your MQTT password
#define MQTT_CLIENT_ID     "ESP32_IoT_Device"          // You can customize this

// ============================================================================
// Device Configuration
// ============================================================================
#define DEVICE_NAME        "ESP32_MQTT_Monitor"    // Customize your device name
#define DEVICE_LOCATION    "Your_Location"         // Set your device location

// ============================================================================
// MQTT Topics Configuration
// ============================================================================
#define MQTT_TOPIC_PREFIX  "esp32"                 // Customize your topic prefix

// Derived topic names (you can modify these)
#define MQTT_TOPIC_HELLO       MQTT_TOPIC_PREFIX "/hello"
#define MQTT_TOPIC_STATUS      MQTT_TOPIC_PREFIX "/status"
#define MQTT_TOPIC_DEVICE_INFO MQTT_TOPIC_PREFIX "/device_info"
#define MQTT_TOPIC_PING_RESULTS MQTT_TOPIC_PREFIX "/ping"
#define MQTT_TOPIC_COMMANDS    MQTT_TOPIC_PREFIX "/commands"

// ============================================================================
// Network Configuration
// ============================================================================
#define MQTT_KEEPALIVE     60       // MQTT keepalive interval in seconds
#define MQTT_TIMEOUT_MS    10000    // MQTT operation timeout in milliseconds
#define WIFI_RETRY_COUNT   10       // Number of WiFi connection retries

// ============================================================================
// Build complete MQTT broker URI
// ============================================================================
#define MQTT_BROKER_URI    "mqtts://" MQTT_BROKER_HOST ":8883"

#ifdef __cplusplus
}
#endif

#endif // SECRETS_H
