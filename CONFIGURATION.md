# Configuration Setup Guide

This guide explains how to set up your ESP32 MQTT project with your own credentials without exposing them to version control.

## Quick Setup

### 1. Copy the Configuration Template

Copy the template configuration file to create your local secrets file:

```bash
# From the project root directory
cp include/config_template.h include/secrets.h
```

### 2. Edit Your Credentials

Open `include/secrets.h` and fill in your actual credentials:

```c
// WiFi Configuration
#define WIFI_SSID          "Your_Actual_WiFi_SSID"
#define WIFI_PASSWORD      "Your_Actual_WiFi_Password"

// MQTT Configuration - HiveMQ Cloud
#define MQTT_BROKER_HOST   "your-actual-hivemq-cluster.s1.eu.hivemq.cloud"
#define MQTT_USERNAME      "your_actual_mqtt_username"
#define MQTT_PASSWORD      "your_actual_mqtt_password"
#define MQTT_CLIENT_ID     "ESP32_IoT_Device_Unique_ID"

// Device Configuration
#define DEVICE_NAME        "Your_Device_Name"
#define DEVICE_LOCATION    "Your_Device_Location"
```

### 3. Verify Git Ignore

Make sure `include/secrets.h` is listed in your `.gitignore` file (it should be already):

```gitignore
# Secrets and sensitive configuration files
include/secrets.h
secrets.h
config.local
.env.local
```

## Configuration Options

### WiFi Settings

- **WIFI_SSID**: Your WiFi network name
- **WIFI_PASSWORD**: Your WiFi network password
- **WIFI_RETRY_COUNT**: Number of connection retry attempts (default: 10)

### MQTT Settings

- **MQTT_BROKER_HOST**: Your HiveMQ Cloud cluster hostname
- **MQTT_BROKER_PORT**: MQTT broker port (8883 for secure connection)
- **MQTT_USERNAME**: Your MQTT username
- **MQTT_PASSWORD**: Your MQTT password
- **MQTT_CLIENT_ID**: Unique identifier for your ESP32 device

### Device Settings

- **DEVICE_NAME**: Friendly name for your device
- **DEVICE_LOCATION**: Physical location of your device

### MQTT Topics

- **MQTT_TOPIC_PREFIX**: Base prefix for all MQTT topics (default: "esp32")

The following topics are automatically derived:
- `{prefix}/hello` - Hello messages
- `{prefix}/status` - Status updates
- `{prefix}/device_info` - Device information
- `{prefix}/ping` - Ping monitoring results
- `{prefix}/commands` - Remote commands

## Security Best Practices

1. **Never commit secrets.h**: The `.gitignore` file ensures this file won't be tracked by Git
2. **Use unique client IDs**: Each ESP32 device should have a unique MQTT client ID
3. **Use strong passwords**: Choose strong passwords for both WiFi and MQTT
4. **Rotate credentials**: Periodically update your credentials for security

## HiveMQ Cloud Setup

1. Create a free account at [HiveMQ Cloud](https://www.hivemq.com/cloud/)
2. Create a new cluster
3. Note down your cluster hostname (e.g., `xxxxx.s1.eu.hivemq.cloud`)
4. Create MQTT credentials (username/password)
5. Use port 8883 for secure MQTT over TLS

## Troubleshooting

### Connection Issues

1. **WiFi not connecting**:
   - Verify SSID and password in `secrets.h`
   - Check WiFi signal strength
   - Ensure WPA2-PSK authentication

2. **MQTT not connecting**:
   - Verify HiveMQ cluster hostname
   - Check MQTT username and password
   - Ensure port 8883 is used for secure connection
   - Verify internet connectivity

3. **Certificate issues**:
   - The project uses ESP32's built-in certificate bundle
   - No additional certificates needed for HiveMQ Cloud

### Build Issues

1. **Missing secrets.h**:
   ```
   fatal error: secrets.h: No such file or directory
   ```
   - Copy `config_template.h` to `secrets.h` and fill in your credentials

2. **Compilation errors**:
   - Ensure all required macros are defined in `secrets.h`
   - Check for syntax errors in the configuration

## Example Configuration

Here's an example of a complete `secrets.h` file:

```c
#ifndef SECRETS_H
#define SECRETS_H

// WiFi Configuration
#define WIFI_SSID          "MyHomeNetwork"
#define WIFI_PASSWORD      "MySecurePassword123"

// MQTT Configuration - HiveMQ Cloud
#define MQTT_BROKER_HOST   "abcd1234.s1.eu.hivemq.cloud"
#define MQTT_BROKER_PORT   8883
#define MQTT_USERNAME      "esp32user"
#define MQTT_PASSWORD      "SecureMqttPass456"
#define MQTT_CLIENT_ID     "ESP32_Kitchen_Monitor"

// Device Configuration
#define DEVICE_NAME        "Kitchen_ESP32"
#define DEVICE_LOCATION    "Kitchen"

// MQTT Topics Configuration
#define MQTT_TOPIC_PREFIX  "home/kitchen"

// Derived topics (don't change these unless needed)
#define MQTT_TOPIC_HELLO       MQTT_TOPIC_PREFIX "/hello"
#define MQTT_TOPIC_STATUS      MQTT_TOPIC_PREFIX "/status"
#define MQTT_TOPIC_DEVICE_INFO MQTT_TOPIC_PREFIX "/device_info"
#define MQTT_TOPIC_PING_RESULTS MQTT_TOPIC_PREFIX "/ping"
#define MQTT_TOPIC_COMMANDS    MQTT_TOPIC_PREFIX "/commands"

// Network Configuration
#define MQTT_KEEPALIVE     60
#define MQTT_TIMEOUT_MS    10000
#define WIFI_RETRY_COUNT   10

// Build complete MQTT broker URI
#define MQTT_BROKER_URI    "mqtts://" MQTT_BROKER_HOST ":8883"

#endif // SECRETS_H
```
