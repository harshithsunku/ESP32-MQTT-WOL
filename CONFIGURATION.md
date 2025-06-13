# Configuration Setup Guide

This guide explains how to set up your ESP32 MQTT Wake-on-LAN project with your own credentials without exposing them to version control.

## Quick Setup

### 1. Copy the Configuration Template

Copy the template configuration file to create your local secrets file:

```powershell
# From the project root directory
Copy-Item include\secrets.template.h include\secrets.h
```

### 2. Edit Your Credentials

Open `include\secrets.h` and fill in your actual credentials:

```c
// WiFi Configuration
#define WIFI_SSID          "Your_Actual_WiFi_SSID"
#define WIFI_PASSWORD      "Your_Actual_WiFi_Password"

// MQTT Configuration - HiveMQ Cloud
#define MQTT_BROKER_HOST   "your-actual-hivemq-cluster.s1.eu.hivemq.cloud"
#define MQTT_USERNAME      "your_actual_mqtt_username"
#define MQTT_PASSWORD      "your_actual_mqtt_password"
#define MQTT_CLIENT_ID     "ESP32_WoL_Device_Unique_ID"

// Device Configuration
#define DEVICE_NAME        "ESP32_WoL_Controller"
#define DEVICE_LOCATION    "Your_Device_Location"
```

### 3. Configure WoL Devices

Edit the device list in `src\wol_manager.c` in the `wol_load_device_config()` function:

```c
esp_err_t wol_load_device_config(void)
{
    ESP_LOGI(TAG, "Loading device configuration");
    
    // Replace with your actual devices
    uint8_t server_mac[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};  // Your server MAC
    uint8_t desktop_mac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}; // Your desktop MAC
    uint8_t nas_mac[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};     // Your NAS MAC
    
    wol_add_device("server1", "192.168.0.111", server_mac, "Main Server");
    wol_add_device("desktop1", "192.168.0.112", desktop_mac, "Development Desktop");
    wol_add_device("nas1", "192.168.0.113", nas_mac, "Network Storage");
    
    return ESP_OK;
}
```

### 4. Verify Git Ignore

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

- **DEVICE_NAME**: Friendly name for your ESP32 controller
- **DEVICE_LOCATION**: Physical location of your device

### Wake-on-LAN Device Configuration

Devices to monitor and control are configured in the `wol_load_device_config()` function. For each device you need:

- **Name**: Unique identifier (used in MQTT topics)
- **IP Address**: Current IP address for connectivity monitoring
- **MAC Address**: Hardware MAC address for wake-on-LAN packets
- **Description**: Human-readable description

### MQTT Topics Structure

The following topics are used:

- `esp32/hello` - Device registration and capabilities
- `esp32/device/{name}/status` - Device online/offline status
- `esp32/wol/{name}/command` - Wake-on-LAN commands ("wake", "status", "enable", "disable")
- `esp32/wol/{name}/status` - Wake command results
- `esp32/ping/stats` - Periodic monitoring statistics

## Getting Device MAC Addresses

To find MAC addresses for your devices:

### Windows:
```cmd
ipconfig /all
# Look for "Physical Address" under your network adapter
```

### Linux/macOS:
```bash
ip link show
# or
ifconfig
# Look for "ether" or "HWaddr"
```

### Network Tools:
```bash
# Scan network and show MAC addresses
nmap -sn 192.168.0.0/24
arp -a
```

## Security Best Practices

1. **Never commit secrets.h**: The `.gitignore` file ensures this file won't be tracked by Git
2. **Use unique client IDs**: Each ESP32 device should have a unique MQTT client ID
3. **Strong passwords**: Use strong, unique passwords for WiFi and MQTT
4. **TLS encryption**: Always use TLS for MQTT connections (port 8883)
5. **Network security**: Ensure your WiFi network uses WPA2/WPA3 encryption
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
#define MQTT_CLIENT_ID     "ESP32_WoL_Controller_01"

// Device Configuration
#define DEVICE_NAME        "ESP32_WoL_Controller"
#define DEVICE_LOCATION    "Office"

// Network Configuration
#define MQTT_KEEPALIVE     60
#define MQTT_TIMEOUT_MS    10000
#define WIFI_RETRY_COUNT   10

// Build complete MQTT broker URI
#define MQTT_BROKER_URI    "mqtts://" MQTT_BROKER_HOST ":8883"

#endif // SECRETS_H
```

### WoL Device Configuration Example

Configure your devices in `src/wol_manager.c`:

```c
esp_err_t wol_load_device_config(void)
{
    ESP_LOGI(TAG, "Loading device configuration");
    
    // Home server
    uint8_t server_mac[] = {0x2C, 0xF0, 0x5D, 0x12, 0x34, 0x56};
    wol_add_device("homeserver", "192.168.1.100", server_mac, "Main Home Server");
    
    // Gaming desktop
    uint8_t desktop_mac[] = {0x3C, 0x97, 0x0E, 0xAB, 0xCD, 0xEF};
    wol_add_device("gaming-pc", "192.168.1.101", desktop_mac, "Gaming Desktop");
    
    // Media center
    uint8_t htpc_mac[] = {0x1C, 0x1B, 0x0D, 0x11, 0x22, 0x33};
    wol_add_device("media-pc", "192.168.1.102", htpc_mac, "HTPC Media Center");
    
    return ESP_OK;
}
```

## Testing Your Configuration

After configuring your device, you can test it:

### 1. Monitor Device Status

Subscribe to the device status topics:

```bash
# Replace with your MQTT broker details
mosquitto_sub -h your-cluster.hivemq.cloud -p 8883 -u your_username -P your_password --capath /etc/ssl/certs -t "esp32/device/+/status"
```

### 2. Send Wake Commands

Wake a device using MQTT:

```bash
# Wake the home server
mosquitto_pub -h your-cluster.hivemq.cloud -p 8883 -u your_username -P your_password --capath /etc/ssl/certs -t "esp32/wol/homeserver/command" -m "wake"

# Check device status  
mosquitto_pub -h your-cluster.hivemq.cloud -p 8883 -u your_username -P your_password --capath /etc/ssl/certs -t "esp32/wol/homeserver/command" -m "status"
```

### 3. Monitor ESP32 Logs

Watch the ESP32 serial output for connection status and device monitoring:

```
I (12345) WIFI: WiFi connected successfully
I (12350) MQTT: Connected to MQTT broker
I (12355) WOL: Loaded 3 devices for monitoring
I (12360) PING: Starting device monitoring task
I (12365) WOL: Device homeserver is offline
I (12370) WOL: Device gaming-pc is online  
I (12375) WOL: Device media-pc is offline
```

This completes your ESP32 MQTT Wake-on-LAN project configuration!
