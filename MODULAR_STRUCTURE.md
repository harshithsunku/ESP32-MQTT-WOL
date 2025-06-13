# ESP32 MQTT Wake-on-LAN Project Architecture

This ESP32 project features a simplified, modular architecture focused on Wake-on-LAN functionality with automatic device monitoring via MQTT. The design emphasizes ease of use, reliability, and clear separation of concerns.

## Project Structure

```
ESP32-MQTT-WOL/
├── include/
│   ├── wifi_manager.h      # WiFi connection management
│   ├── ping_manager.h      # Simplified connectivity monitoring library
│   ├── wol_manager.h       # Wake-on-LAN device management
│   ├── mqtt_manager.h      # MQTT client with command handling
│   ├── device_info.h       # ESP32 device information
│   ├── secrets.h           # Local configuration (git-ignored)
│   └── secrets.template.h  # Configuration template
├── src/
│   ├── main.c              # Main application entry point
│   ├── wifi_manager.c      # WiFi management implementation
│   ├── ping_manager.c      # TCP-based connectivity checking
│   ├── wol_manager.c       # WoL device management with ping integration
│   ├── mqtt_manager.c      # MQTT client with TLS support
│   ├── device_info.c       # Device info implementation
│   └── CMakeLists.txt
└── ...
```

## Architecture Overview

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   MQTT Client   │────│  WoL Manager    │────│  Ping Manager   │
│                 │    │                 │    │   (Library)     │
│ • Commands      │    │ • Device List   │    │ • TCP Checks    │
│ • Status Pub    │    │ • Magic Packets │    │ • Callbacks     │
│ • TLS Security  │    │ • Auto Monitor  │    │ • Statistics    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 │
                    ┌─────────────────┐
                    │  WiFi Manager   │
                    │                 │
                    │ • Connection    │
                    │ • Auto-Reconnect│
                    │ • Event Handling│
                    └─────────────────┘
```

## Modules Description

### 1. WiFi Manager (`wifi_manager.h/c`)
Handles all WiFi-related functionality with robust error handling:
- WiFi station mode initialization
- Connection management with retry logic
- Event handling for connection/disconnection
- Connection status checking and auto-reconnection

**Key Functions:**
- `wifi_manager_init()` - Initialize and connect to WiFi
- `wifi_manager_is_connected()` - Check connection status
- `wifi_manager_deinit()` - Clean up WiFi resources

### 2. Ping Manager (`ping_manager.h/c`) - Simplified Library
Provides connectivity monitoring services using TCP connections:
- TCP-based host availability checking (ports 80, 22)
- Thread-safe device management with FreeRTOS mutex
- Real-time callback notifications for status changes
- Statistics collection per device
- Simple API for device add/remove/enable/disable

**Key Functions:**
- `ping_manager_init(callback, user_data)` - Initialize with result callback
- `ping_manager_add_device(name, ip)` - Add device for monitoring
- `ping_manager_remove_device(name)` - Remove device from monitoring
- `ping_manager_get_device(name)` - Get device status and statistics
- `ping_manager_set_device_enabled(name, enabled)` - Enable/disable monitoring

**Design Philosophy:**
- Library service rather than standalone application
- Integrated with WoL manager for automatic device monitoring
- No complex command queues - simple direct API
- Focus on reliability over advanced features

### 3. WoL Manager (`wol_manager.h/c`) - Main Controller
Manages Wake-on-LAN devices and integrates with ping monitoring:
- Device registration with MAC addresses and IP addresses
- Automatic ping monitoring integration
- Magic packet transmission for device wake-up
- MQTT command handling for remote control
- Device status tracking and MQTT reporting

**Key Functions:**
- `wol_manager_init()` - Initialize WoL functionality
- `wol_add_device(name, ip, mac, description)` - Add device (auto-adds to ping)
- `wol_remove_device(name)` - Remove device (auto-removes from ping)
- `wol_wake_device(name)` - Send wake-on-LAN packet
- `wol_handle_mqtt_command(device, command)` - Process MQTT commands
- `wol_update_device_status(name, online)` - Update status from ping results

**Integration Features:**
- Automatically calls `ping_manager_add_device()` when devices are added
- Automatically calls `ping_manager_remove_device()` when devices are removed
- Handles ping callback results to update device status
- Publishes status changes via MQTT

### 4. MQTT Manager (`mqtt_manager.h/c`)
Secure MQTT client with command routing and status publishing:
- TLS-encrypted connections to HiveMQ Cloud
- Automatic reconnection with exponential backoff
- Command topic subscription and routing
- Status publishing for device updates
- Certificate validation and secure communication

**Key Functions:**
- `mqtt_manager_init()` - Initialize MQTT client and connect
- `mqtt_publish(topic, message)` - Publish messages
- `mqtt_subscribe(topic)` - Subscribe to command topics
- Internal command routing to WoL manager

### 5. Device Info (`device_info.h/c`)
Provides comprehensive ESP32 hardware information:
- Chip information (model, revision, cores, features)
- Flash memory details and partition information
- MAC addresses for different interfaces
- Memory usage statistics and system information
- Hardware feature detection and capabilities

**Key Functions:**
- `device_info_print_all()` - Print complete device information
- `device_info_print_chip()` - Print only chip information
- `device_info_print_memory()` - Print only memory information
- `device_info_print_mac_addresses()` - Print only MAC addresses

### 6. Main Application (`main.c`)
Coordinates all modules and implements the main application logic:
- NVS initialization for persistent storage
- Sequential module initialization
- Error handling and recovery
- Main monitoring loop with statistics reporting

## Benefits of Simplified Architecture

1. **Ease of Use**: Ping manager as a library service simplifies integration
2. **Automatic Integration**: WoL devices are automatically monitored without manual setup
3. **Focused Functionality**: Clear separation between device monitoring and WoL control
4. **Maintainability**: Each module has a specific, well-defined responsibility
5. **Reliability**: TCP-based connectivity checking is more reliable than ICMP
6. **Thread Safety**: Proper FreeRTOS synchronization throughout
7. **MQTT Integration**: Seamless status reporting and remote control
8. **Scalability**: Easy to add new devices or extend functionality

## Data Flow

```
1. WoL Manager adds device → Ping Manager starts monitoring
2. Ping Manager checks connectivity → Callback to WoL Manager
3. WoL Manager updates status → MQTT Manager publishes status
4. MQTT command received → WoL Manager sends magic packet
5. Device status change → Automatic MQTT notification
```

## Configuration

Configuration is handled through the secure secrets system:

```c
// In secrets.h (local, git-ignored file)
#define WIFI_SSID          "Your_WiFi_Network"
#define WIFI_PASSWORD      "Your_WiFi_Password"
#define MQTT_BROKER_HOST   "your-cluster.s1.eu.hivemq.cloud"
#define MQTT_USERNAME      "your_mqtt_username"
#define MQTT_PASSWORD      "your_mqtt_password"
```

Device configuration is handled in `wol_manager.c`:

```c
// Default devices loaded at startup
uint8_t mac1[] = {0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E};
wol_add_device("server1", "192.168.0.111", mac1, "Main Server");
```

## Building and Deployment

The modular structure uses PlatformIO with ESP-IDF framework:

```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

## Future Enhancements

The modular structure makes it easy to add:
- **Web Configuration Interface**: HTTP server for device management
- **NVS Storage**: Persistent device configuration
- **OTA Updates**: Over-the-air firmware updates
- **Additional Protocols**: Support for other wake protocols
- **Home Assistant Integration**: MQTT discovery and integration
- **Device Groups**: Wake multiple devices simultaneously
- **Scheduling**: Timed wake/sleep operations
