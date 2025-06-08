# ESP32 Modular IoT Project

A comprehensive, modular ESP32 IoT project featuring WiFi connectivity, advanced ping monitoring, device diagnostics, and MQTT-based Wake-on-LAN functionality. Built with ESP-IDF framework and designed for maximum flexibility and reusability.

## 🚀 Features

### Core Modules
- **📡 WiFi Manager**: Robust WiFi connection management with auto-reconnection
- **🏓 Advanced Ping Manager**: Continuous multi-target ping monitoring with thread-safe operations
- **🔍 Device Information**: Comprehensive hardware feature detection and system diagnostics
- **🌐 MQTT Integration**: Wake-on-LAN functionality via MQTT commands (planned)

### Key Capabilities
- ✅ **Modular Architecture**: Clean separation of concerns with dedicated modules
- ✅ **Multi-Target Ping Monitoring**: Monitor up to 10 different IP addresses simultaneously
- ✅ **Real-time Callbacks**: Instant ping result notifications with customizable callbacks
- ✅ **Thread-Safe Operations**: FreeRTOS-based synchronization with mutex and queues
- ✅ **Dynamic Configuration**: Add/remove/modify ping targets at runtime
- ✅ **Comprehensive Diagnostics**: Detailed hardware and system information reporting
- ✅ **Statistics Tracking**: Success/failure rates and response time monitoring
- ✅ **Cross-Platform Support**: Works with all ESP32 family devices (ESP32, S2, S3, C3)

## 📋 Table of Contents

- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Project Structure](#project-structure)
- [Quick Start](#quick-start)
- [Module Documentation](#module-documentation)
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
├── 📄 platformio.ini               # PlatformIO configuration
├── 📄 CMakeLists.txt               # CMake build configuration
├── 📄 sdkconfig.esp32dev           # ESP-IDF SDK configuration
├── 📄 MODULAR_STRUCTURE.md         # Detailed module architecture
├── 📄 PING_MANAGER_USAGE.md        # Ping manager documentation
├── 📄 DEVICE_INFO_FEATURES.md      # Device info capabilities
├── 📂 include/                     # Header files
│   ├── 📄 wifi_manager.h           # WiFi management API
│   ├── 📄 ping_manager.h           # Ping monitoring API
│   └── 📄 device_info.h            # Device diagnostics API
├── 📂 src/                         # Source files
│   ├── 📄 main.c                   # Main application entry point
│   ├── 📄 wifi_manager.c           # WiFi management implementation
│   ├── 📄 ping_manager.c           # Advanced ping monitoring
│   ├── 📄 device_info.c            # Hardware diagnostics
│   └── 📄 CMakeLists.txt           # Source build configuration
├── 📂 lib/                         # External libraries (if any)
└── 📂 test/                        # Unit tests (expandable)
```

## 🚀 Quick Start

### 1. Clone the Repository
```bash
git clone https://github.com/yourusername/ESP32-MQTT-WOL.git
cd ESP32-MQTT-WOL
```

### 2. Open in PlatformIO
```bash
# Open in VS Code with PlatformIO
code .
```

### 3. Configure WiFi Credentials
Edit `src/wifi_manager.c` and update your WiFi credentials:
```c
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"
```

### 4. Build and Flash
```bash
# Using PlatformIO CLI
pio run --target upload

# Or use VS Code PlatformIO extension
# Press Ctrl+Shift+P -> "PlatformIO: Upload"
```

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

### 🏓 Advanced Ping Manager (`ping_manager.h/.c`)
Sophisticated ping monitoring system with multi-target support and real-time callbacks.

**Key Features:**
- Monitor up to 10 IP addresses simultaneously
- Individual intervals and timeouts per target
- Real-time callback notifications
- Thread-safe operations
- Statistics tracking
- Dynamic target management

**Basic Usage:**
```c
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
