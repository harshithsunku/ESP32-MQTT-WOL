# ESP32 MQTT Wake-on-LAN Project

A simplified, modular ESP32 IoT project featuring WiFi connectivity, MQTT-based Wake-on-LAN functionality, and automatic device monitoring. Built with ESP-IDF/PlatformIO and designed for easy deployment and maintenance.

## 🚀 Features

### Core Modules
- **📡 WiFi Manager**: Robust WiFi connection management with auto-reconnection
- **🏓 Simplified Ping Manager**: TCP-based device connectivity monitoring library
- **⚡ WoL Manager**: Wake-on-LAN packet transmission with MQTT control
- **📡 MQTT Manager**: Secure MQTT connectivity with command handling
- **🔍 Device Information**: Hardware feature detection and system diagnostics
- **🔐 Secure Configuration**: Environment-based configuration system for credentials

### Key Capabilities
- ✅ **Simplified Architecture**: Ping manager as a library service for WoL monitoring
- ✅ **Automatic Device Monitoring**: WoL devices are automatically added to ping monitoring
- ✅ **MQTT Wake-on-LAN Control**: Remote device wake commands via MQTT
- ✅ **TCP Connection Testing**: Reliable host availability checking using port connections
- ✅ **Real-time Status Updates**: MQTT status reporting for device online/offline changes
- ✅ **Thread-Safe Operations**: FreeRTOS-based synchronization with mutex protection
- ✅ **Secure MQTT Communication**: TLS-encrypted MQTT with certificate validation
- ✅ **Easy Device Management**: Simple API for adding/removing monitored devices
- ✅ **Statistics Tracking**: Success/failure rates and response time monitoring
- ✅ **Cross-Platform Support**: Works with all ESP32 family devices (ESP32, S2, S3, C3)
- ✅ **Git-Safe Credentials**: Secure configuration system that keeps secrets out of version control

## 📋 Table of Contents

- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Quick Start](#quick-start)
- [Secure Configuration Setup](#secure-configuration-setup)
- [Project Structure](#project-structure)
- [Module Documentation](#module-documentation)
- [MQTT Integration](#mqtt-integration)
- [Configuration](#configuration)
- [Usage Examples](#usage-examples)
- [API Reference](#api-reference)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)

## 🛠️ Hardware Requirements

### Supported ESP32 Variants
- **ESP32**: Dual-core Xtensa LX6 with WiFi + Bluetooth
- **ESP32-S2**: Single-core Xtensa LX7 with WiFi + USB OTG
- **ESP32-S3**: Dual-core Xtensa LX7 with WiFi + Bluetooth + AI acceleration
- **ESP32-C3**: Single-core RISC-V with WiFi + Bluetooth + IEEE 802.15.4

### Minimum Requirements
- ESP32 development board (any variant)
- USB cable for programming
- WiFi network access
- 4MB+ Flash memory (recommended)

### Optional Hardware
- External PSRAM (for enhanced performance)
- Status LEDs (for visual feedback)
- External sensors (expandable architecture)

## 💻 Software Requirements

### Development Environment
- **PlatformIO** (recommended) or **ESP-IDF** (v4.4+)
- **Visual Studio Code** with PlatformIO extension
- **Git** for version control

### Dependencies
- ESP-IDF framework (automatically managed by PlatformIO)
- FreeRTOS (included with ESP-IDF)
- LwIP networking stack (included)

## 📁 Project Structure

```
ESP32-MQTT-WOL/
├── 📄 README.md                    # This comprehensive guide
├── 📄 LICENSE                      # Project license
## 📁 Project Structure

```
ESP32-MQTT-WOL/
├── 📄 README.md                    # This comprehensive guide
├── 📄 platformio.ini               # PlatformIO configuration
├── 📄 CMakeLists.txt               # CMake build configuration
├── 📄 sdkconfig.esp32dev           # ESP-IDF SDK configuration
├── 📄 MODULAR_STRUCTURE.md         # Detailed module architecture
├── 📄 PING_MANAGER_USAGE.md        # Ping manager documentation
├── 📄 DEVICE_INFO_FEATURES.md      # Device info capabilities
├── 📄 CONFIGURATION.md             # Configuration setup guide
├── 📂 include/                     # Header files
│   ├── 📄 wifi_manager.h           # WiFi management API
│   ├── 📄 ping_manager.h           # Simplified ping library API
│   ├── 📄 wol_manager.h            # Wake-on-LAN management API
│   ├── 📄 mqtt_manager.h           # MQTT client management API
│   ├── 📄 device_info.h            # Device diagnostics API
│   ├── 📄 secrets.h                # Your local configuration (git-ignored)
│   └── 📄 secrets.template.h       # Configuration template
├── 📂 src/                         # Source files
│   ├── 📄 main.c                   # Main application entry point
│   ├── 📄 wifi_manager.c           # WiFi management implementation
│   ├── 📄 ping_manager.c           # TCP-based connectivity checking
│   ├── 📄 wol_manager.c            # Wake-on-LAN device management
│   ├── 📄 mqtt_manager.c           # MQTT client implementation
│   ├── 📄 device_info.c            # Hardware diagnostics
│   └── 📄 CMakeLists.txt           # Source build configuration
├── 📂 lib/                         # External libraries (if any)
└── 📂 test/                        # Unit tests (expandable)
```
```

## 🚀 Quick Start

### 1. Clone the Repository
```bash
git clone https://github.com/yourusername/ESP32-MQTT-WOL.git
cd ESP32-MQTT-WOL
```

### 2. Setup Secure Configuration
```bash
# Copy the configuration template
cp include/config_template.h include/secrets.h
```

Edit `include/secrets.h` with your actual credentials:
```c
// WiFi Configuration
#define WIFI_SSID          "YourWiFiSSID"
#define WIFI_PASSWORD      "YourWiFiPassword"

// MQTT Configuration (HiveMQ Cloud)
#define MQTT_BROKER_HOST   "your-cluster.s1.eu.hivemq.cloud"
#define MQTT_USERNAME      "your_username"
#define MQTT_PASSWORD      "your_password"
#define MQTT_CLIENT_ID     "ESP32_Device_001"
```

### 3. Open in PlatformIO
```bash
# Open in VS Code with PlatformIO
code .
```

### 4. Build and Flash
```bash
# Using PlatformIO CLI
pio run --target upload

# Or use VS Code PlatformIO extension
# Press Ctrl+Shift+P -> "PlatformIO: Upload"
```

### 5. Monitor Output
```bash
# Using PlatformIO CLI
pio device monitor

# Or use VS Code PlatformIO extension
# Press Ctrl+Shift+P -> "PlatformIO: Monitor"
```

## 🔐 Secure Configuration Setup

This project uses a secure configuration system that keeps your credentials safe from accidental Git commits.

### Configuration Files

- **`include/config_template.h`**: Template with placeholder values (safe to commit)
- **`include/secrets.h`**: Your actual credentials (NEVER commit this file)

### First-Time Setup

1. **Copy the template**:
   ```bash
   cp include/config_template.h include/secrets.h
   ```

2. **Edit your secrets**:
   Open `include/secrets.h` and replace all placeholder values with your actual credentials.

3. **Verify Git ignore**:
   The `.gitignore` file ensures `secrets.h` is never committed to version control.

### Detailed Configuration Guide

For complete configuration instructions, see [CONFIGURATION.md](./CONFIGURATION.md).

### Security Benefits

- ✅ **Git-safe**: Credentials are never committed to version control
- ✅ **Template-based**: Easy setup for new developers
- ✅ **Environment-specific**: Different configurations for different deployments
- ✅ **Team-friendly**: Each developer can have their own local configuration

### 5. Monitor Serial Output
```bash
# View real-time logs
pio device monitor

# Or use VS Code terminal
# Press Ctrl+Shift+P -> "PlatformIO: Serial Monitor"
```

## 📚 Module Documentation

### 🔌 WiFi Manager (`wifi_manager.h/.c`)
Handles all WiFi-related operations with robust error handling and auto-reconnection.

**Key Features:**
- Automatic connection management
- Event-driven architecture
- Connection status monitoring
- Configurable retry mechanisms

**Basic Usage:**
```c
#include "wifi_manager.h"

void app_main(void) {
    // Initialize WiFi
    esp_err_t ret = wifi_manager_init();
    if (ret == ESP_OK) {
        ESP_LOGI("APP", "WiFi connected successfully");
    }
    
    // Check connection status
    if (wifi_manager_is_connected()) {
        ESP_LOGI("APP", "WiFi is connected");
    }
    
    // Cleanup when done
    wifi_manager_deinit();
}
```

### 🏓 Simplified Ping Manager (`ping_manager.h/.c`)
Simple connectivity monitoring library using TCP connections to check host availability.

**Key Features:**
- TCP connection-based host checking (ports 80 and 22)
- Automatic integration with WoL manager
- Real-time callback notifications
- Thread-safe operations
- Statistics tracking per device
- Configurable intervals and timeouts

**Basic Usage:**
```c
#include "ping_manager.h"

// Callback function for ping results
void ping_result_handler(const char* name, const char* ip_address, bool success, uint32_t response_time, void* user_data) {
    if (success) {
        ESP_LOGI("APP", "Device %s (%s) is online: %d ms", name, ip_address, response_time);
    } else {
        ESP_LOGW("APP", "Device %s (%s) is offline", name, ip_address);
    }
}

void app_main(void) {
    // Initialize ping manager with callback
    ping_manager_init(ping_result_handler, NULL);
    
    // Add devices for monitoring (typically done by WoL manager)
    ping_manager_add_device("server1", "192.168.0.111");
    ping_manager_add_device("desktop1", "192.168.0.112");
    
    // Get device status
    const ping_target_t* device = ping_manager_get_device("server1");
    if (device) {
        ESP_LOGI("APP", "Device %s is %s", device->name, device->is_online ? "online" : "offline");
    }
}
```

### ⚡ Wake-on-LAN Manager (`wol_manager.h/.c`)
Manages WoL devices and automatically integrates with ping monitoring for device status tracking.

**Key Features:**
- Automatic device monitoring integration
- MQTT command handling for remote wake
- Device status tracking and reporting
- Magic packet transmission
- Enable/disable device management

**Basic Usage:**
```c
#include "wol_manager.h"

void app_main(void) {
    // Initialize WoL manager
    wol_manager_init();
    
    // Add devices (automatically added to ping monitoring)
    uint8_t mac[] = {0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E};
    wol_add_device("server1", "192.168.0.111", mac, "Main Server");
    
    // Wake a device
    wol_wake_device("server1");
    
    // Handle MQTT commands
    wol_handle_mqtt_command("server1", "wake");
}
```

### 📡 MQTT Manager (`mqtt_manager.h/.c`)
Secure MQTT client with TLS encryption and command handling integration.

**Key Features:**
- TLS-encrypted connections
- Automatic reconnection
- Command routing to appropriate managers
- Status publishing
- Certificate validation

**Basic Usage:**
```c
#include "mqtt_manager.h"

void app_main(void) {
    // Initialize MQTT (requires WiFi to be connected)
    mqtt_manager_init();
    
    // Publish a message
    mqtt_publish("esp32/status", "online");
}
```
#include "ping_manager.h"

// Callback function for ping results
void my_ping_callback(const char* ip, bool success, uint32_t response_time, void* user_data) {
    if (success) {
        ESP_LOGI("PING", "✓ %s responded in %d ms", ip, response_time);
    } else {
        ESP_LOGW("PING", "✗ %s failed to respond", ip);
    }
}

void app_main(void) {
    // Initialize ping manager with callback
    ping_manager_init(my_ping_callback, NULL);
    
    // Add targets with different intervals
    ping_manager_add_target("8.8.8.8", 5000, 3000, 1);      // Google DNS every 5s
    ping_manager_add_target("1.1.1.1", 10000, 3000, 1);     // Cloudflare every 10s
    ping_manager_add_target("192.168.1.1", 2000, 1000, 1);  // Router every 2s
    
    // Let it run for a while...
    vTaskDelay(pdMS_TO_TICKS(60000)); // 1 minute
    
    // Cleanup
    ping_manager_deinit();
}
```

### 🔍 Device Information (`device_info.h/.c`)
Comprehensive hardware and system diagnostics with detailed feature detection.

**Key Features:**
- Complete chip information and capabilities
- Hardware feature enumeration
- Memory usage statistics
- Peripheral configuration details
- Clock and timing information
- Security status reporting

**Basic Usage:**
```c
#include "device_info.h"

void app_main(void) {
    // Print all device information
    device_info_print_all();
    
    // Or print specific sections
    device_info_print_chip();
    device_info_print_hardware_features();
    device_info_print_memory();
}
```

## 📡 MQTT Integration

The ESP32 connects to HiveMQ Cloud broker using secure MQTT over TLS (MQTTS) and provides comprehensive Wake-on-LAN functionality.

### Features

- **🔐 Secure Connection**: TLS-encrypted MQTT communication with certificate validation
- **📤 Hello Messages**: Automatic device registration and status broadcasting
- **⚡ Wake-on-LAN Control**: Remote device wake commands via MQTT
- **📊 Device Status**: Real-time online/offline status reporting for monitored devices
- **🏓 Connectivity Monitoring**: TCP-based host availability checking
- **📱 Remote Commands**: Wake devices remotely via MQTT topics
- **🔄 Auto-Reconnection**: Robust connection handling with automatic recovery

### MQTT Topics Structure

| Topic | Purpose | QoS | Payload Format |
|-------|---------|-----|----------------|
| `esp32/hello` | Device registration | 1 | JSON: Device info and capabilities |
| `esp32/device/{name}/status` | Device status updates | 1 | JSON: Online/offline status |
| `esp32/wol/{name}/command` | Wake-on-LAN commands | 1 | String: "wake", "status", "enable", "disable" |
| `esp32/wol/{name}/status` | Wake command results | 1 | JSON: Action confirmation |
| `esp32/ping/stats` | Monitoring statistics | 0 | JSON: Success rates and response times |
### Example Messages

**Hello Message**:
```json
{
  "message": "ESP32 WoL Controller Online",
  "device_mac": "aa:bb:cc:dd:ee:ff",
  "device_name": "ESP32_WoL_001",
  "capabilities": ["wake-on-lan", "device-monitoring", "mqtt-control"],
  "managed_devices": 3,
  "timestamp": 1234567890
}
```

**Device Status Update**:
```json
{
  "device": "server1",
  "status": "online",
  "ip": "192.168.0.111",
  "timestamp": 1234567890
}
```

**Wake-on-LAN Command**:
```json
{
  "device": "server1",
  "action": "wake_sent",
  "timestamp": 1234567890
}
```

**Ping Statistics**:
```json
{
  "total_devices": 3,
  "online_devices": 2,
  "devices": [
    {
      "name": "server1",
      "ip": "192.168.0.111",
      "status": "online",
      "success_rate": 98.5,
      "avg_response_time": 12
    },
    {
      "name": "desktop1", 
      "ip": "192.168.0.112",
      "status": "offline",
      "success_rate": 0.0,
      "last_seen": 1234567800
    }
  ]
}
  "timestamp": 1234567890
}
```

### Remote Commands

Send commands to the `esp32/commands` topic:

- `ping_google` - Execute one-time ping to Google DNS
- `device_info` - Send detailed device information
- `hello` - Send hello message

### HiveMQ Cloud Setup

1. **Create Account**: Sign up at [HiveMQ Cloud](https://www.hivemq.com/cloud/)
2. **Create Cluster**: Set up a new MQTT cluster
3. **Get Credentials**: Note your cluster hostname and create MQTT user credentials
4. **Configure ESP32**: Update your `secrets.h` with the cluster details

Example configuration:
```c
#define MQTT_BROKER_HOST   "abcd1234.s1.eu.hivemq.cloud"
#define MQTT_USERNAME      "your_username"
#define MQTT_PASSWORD      "your_password"
```

### Testing MQTT Connection

Monitor the serial output for MQTT connection status:

```
I (4430) MQTT_MANAGER: Initializing MQTT client...
I (4430) MQTT_MANAGER: Broker: mqtts://your-cluster.s1.eu.hivemq.cloud:8883
I (4430) MQTT_MANAGER: Username: your_username
I (4440) MQTT_MANAGER: Client ID: ESP32_IoT_Device
I (5650) MQTT_MANAGER: MQTT connected to broker
```

Use an MQTT client like [MQTT Explorer](http://mqtt-explorer.com/) to subscribe to topics and send commands.

## ⚙️ Configuration

### WiFi Configuration
Located in `src/wifi_manager.c`:
```c
#define WIFI_SSID "YourNetworkName"
#define WIFI_PASSWORD "YourNetworkPassword"
#define WIFI_MAXIMUM_RETRY 5
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
```

### Ping Manager Configuration
Located in `include/ping_manager.h`:
```c
#define PING_MAX_TARGETS 10          // Maximum simultaneous targets
#define PING_DEFAULT_INTERVAL 5000   // Default interval (5 seconds)
#define PING_DEFAULT_TIMEOUT 3000    // Default timeout (3 seconds)
#define PING_DEFAULT_COUNT 1         // Pings per cycle
```

### System Configuration
Located in `platformio.ini`:
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = espidf
monitor_speed = 115200
build_flags = 
    -DCONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ=240
    -DCONFIG_FREERTOS_HZ=1000
```

## 💡 Usage Examples

### Example 1: Basic Network Monitoring
```c
#include "wifi_manager.h"
#include "ping_manager.h"
#include "device_info.h"

void ping_callback(const char* ip, bool success, uint32_t response_time, void* user_data) {
    printf("[%s] %s: %s (%d ms)\n", 
           success ? "✓" : "✗", 
           ip, 
           success ? "OK" : "FAIL", 
           response_time);
}

void app_main(void) {
    // Initialize NVS
    nvs_flash_init();
    
    // Show device capabilities
    device_info_print_all();
    
    // Connect to WiFi
    ESP_ERROR_CHECK(wifi_manager_init());
    
    // Start network monitoring
    ESP_ERROR_CHECK(ping_manager_init(ping_callback, NULL));
    
    // Monitor critical network infrastructure
    ping_manager_add_target("8.8.8.8", 5000, 3000, 1);       // Internet connectivity
    ping_manager_add_target("192.168.1.1", 2000, 1000, 1);   // Local router
    ping_manager_add_target("192.168.1.100", 10000, 2000, 1); // Important server
    
    // Keep running
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

### Example 2: Dynamic Ping Management
```c
void demonstrate_dynamic_management(void) {
    // Add a target
    int target_id = ping_manager_add_target("example.com", 5000, 3000, 1);
    
    // Update its configuration
    ping_manager_update_target(target_id, 2000, 1000, 1); // Faster ping
    
    // Temporarily disable it
    ping_manager_set_target_enabled(target_id, false);
    
    // Re-enable after some time
    vTaskDelay(pdMS_TO_TICKS(30000)); // 30 seconds
    ping_manager_set_target_enabled(target_id, true);
    
    // Get statistics
    ping_target_t stats;
    if (ping_manager_get_target_stats(target_id, &stats) == ESP_OK) {
        printf("Target %s: Success=%d, Fail=%d\n", 
               stats.ip_address, stats.success_count, stats.fail_count);
    }
    
    // Remove when done
    ping_manager_remove_target(target_id);
}
```

### Example 3: System Diagnostics
```c
void perform_system_diagnostics(void) {
    ESP_LOGI("DIAG", "Starting comprehensive system diagnostics...");
    
    // Check chip capabilities
    device_info_print_chip();
    
    // Analyze memory usage
    device_info_print_memory();
    
    // Review hardware features
    device_info_print_hardware_features();
    
    // Check peripheral configuration
    device_info_print_peripherals();
    
    // Monitor system clocks
    device_info_print_clocks();
    
    ESP_LOGI("DIAG", "System diagnostics completed");
}
```

## 📖 API Reference

### WiFi Manager API
```c
esp_err_t wifi_manager_init(void);
bool wifi_manager_is_connected(void);
void wifi_manager_deinit(void);
```

### Ping Manager API
```c
// Initialization
esp_err_t ping_manager_init(ping_result_callback_t callback, void* user_data);
void ping_manager_deinit(void);

// Target Management
int ping_manager_add_target(const char* ip, uint32_t interval, uint32_t timeout, uint32_t count);
esp_err_t ping_manager_remove_target(int target_index);
esp_err_t ping_manager_update_target(int target_index, uint32_t interval, uint32_t timeout, uint32_t count);
esp_err_t ping_manager_set_target_enabled(int target_index, bool enabled);

// Statistics and Information
esp_err_t ping_manager_get_target_stats(int target_index, ping_target_t* target);
esp_err_t ping_manager_get_all_targets(ping_target_t targets[], int* count);

// One-time Operations
esp_err_t ping_manager_ping_once(const char* target_ip, uint32_t count, uint32_t timeout_ms);
esp_err_t ping_manager_ping_google(void);
```

### Device Info API
```c
void device_info_print_all(void);
void device_info_print_chip(void);
void device_info_print_memory(void);
void device_info_print_mac_addresses(void);
void device_info_print_hardware_features(void);
void device_info_print_peripherals(void);
void device_info_print_clocks(void);
```

## 🔧 Troubleshooting

### Common Issues

#### WiFi Connection Problems
```bash
# Check WiFi credentials in wifi_manager.c
# Verify network accessibility
# Check serial monitor for connection logs
```

#### Compilation Errors
```bash
# Clean and rebuild
pio run --target clean
pio run

# Check ESP-IDF version compatibility
pio platform update espressif32
```

#### Memory Issues
```bash
# Monitor memory usage
device_info_print_memory();

# Increase stack size if needed (platformio.ini)
build_flags = -DCONFIG_ESP_MAIN_TASK_STACK_SIZE=8192
```

#### Ping Failures
```bash
# Check network connectivity
# Verify target IP addresses
# Adjust timeout values
# Monitor callback messages
```

### Debug Tips

1. **Enable Debug Logging**:
   ```c
   esp_log_level_set("*", ESP_LOG_DEBUG);
   ```

2. **Monitor Memory Usage**:
   ```c
   ESP_LOGI("MEM", "Free heap: %d bytes", esp_get_free_heap_size());
   ```

3. **Check Task Status**:
   ```c
   vTaskList(task_list_buffer); // FreeRTOS task information
   ```

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

### Development Workflow
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

### Code Style
- Follow ESP-IDF coding standards
- Use descriptive variable names
- Add comprehensive comments
- Include error handling

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Espressif Systems** for the excellent ESP-IDF framework
- **PlatformIO** for the outstanding development environment
- **FreeRTOS** for the robust real-time operating system
- **Community contributors** for ideas and feedback

## 📞 Support

- **Documentation**: Check the `docs/` folder for detailed guides
- **Issues**: Report bugs on [GitHub Issues](https://github.com/yourusername/ESP32-MQTT-WOL/issues)
- **Discussions**: Join our [GitHub Discussions](https://github.com/yourusername/ESP32-MQTT-WOL/discussions)

---

**Made with ❤️ for the ESP32 community**

> 💡 **Tip**: Star this repository if you find it helpful!
