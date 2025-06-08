# Enhanced Device Information Module

The `device_info` module has been significantly enhanced to provide comprehensive hardware feature detection and display capabilities for ESP32 family devices.

## Features Overview

### 1. **Chip Information (`device_info_print_chip`)**
- **Model**: ESP32, ESP32-S2, ESP32-S3, ESP32-C3, etc.
- **Revision**: Silicon revision number
- **CPU Cores**: Number of CPU cores available
- **Enhanced Feature Detection**:
  - WiFi 802.11 b/g/n support
  - Bluetooth Classic support
  - Bluetooth Low Energy (BLE) support
  - Flash type (embedded vs external)
  - Architecture-specific features:
    - ESP32: Dual-core Xtensa LX6, Hardware crypto, RTC deep sleep
    - ESP32-S2: Single-core Xtensa LX7, USB OTG, Touch sensors
    - ESP32-S3: Dual-core Xtensa LX7, USB OTG, AI acceleration
    - ESP32-C3: Single-core RISC-V, IEEE 802.15.4 (Thread/Zigbee)
- **Flash Size**: Total flash memory available
- **PSRAM**: External PSRAM availability and size

### 2. **Hardware Features & Capabilities (`device_info_print_hardware_features`)**
- **GPIO**: Total number of GPIO pins
- **Analog**: ADC units and DAC channels
- **Communication Interfaces**: SPI, I2C, UART controller counts
- **Timing**: Timer groups and PWM channels
- **Touch Sensors**: Number of capacitive touch pins (if available)
- **RTC GPIO**: Real-time clock GPIO pins for low-power operation
- **Hardware Crypto Acceleration**:
  - AES-128/192/256 encryption/decryption
  - SHA-1/224/256 hashing
  - RSA public key operations
- **Communication Protocols**:
  - CAN/TWAI controllers
  - SD/MMC host support
  - USB OTG support
  - IEEE 802.15.4 (Thread/Zigbee) support
- **Memory Capabilities**:
  - DMA capable memory
  - 32-bit aligned memory
  - Executable memory
  - RTC fast memory

### 3. **Peripheral Information (`device_info_print_peripherals`)**
- **GPIO Configuration**: Pin mapping and availability
- **ADC Configuration**:
  - Channel counts for ADC1 and ADC2
  - Resolution capabilities (up to 12-13 bits)
  - Pin assignments by chip variant
- **SPI Configuration**:
  - Number of SPI controllers
  - Maximum transfer buffer size
- **I2C Configuration**:
  - Number of I2C controllers
  - Maximum clock speed capabilities
- **UART Configuration**:
  - Number of UART controllers
  - Hardware flow control support
- **PWM/LEDC Configuration**:
  - Number of PWM channels
  - Resolution capabilities (up to 20 bits on ESP32)

### 4. **Clock & Timing Information (`device_info_print_clocks`)**
- **CPU Frequency**: Current CPU operating frequency
- **APB Frequency**: Advanced Peripheral Bus frequency
- **XTAL Frequency**: External crystal oscillator frequency
- **RTC Clocks**: Internal RC oscillator frequencies
- **Timer Capabilities**:
  - High-resolution timer precision (1μs)
  - FreeRTOS system tick rate
  - System uptime in milliseconds

### 5. **Memory Information (`device_info_print_memory`)**
- **Heap Memory**: Free and minimum free heap sizes
- **Internal RAM**: Available internal SRAM
- **External RAM**: Available PSRAM (if configured)
- **Memory Type Breakdown**: By capability flags

### 6. **MAC Addresses (`device_info_print_mac_addresses`)**
- **WiFi Station**: MAC address for WiFi client mode
- **WiFi Access Point**: MAC address for WiFi AP mode
- **Bluetooth**: MAC address for Bluetooth operations

### 7. **Partition Information**
- **Running Partition**: Current application partition details
- **Boot Partition**: Boot partition information
- **Partition Size**: Memory allocation details

### 8. **System Information**
- **IDF Version**: ESP-IDF framework version
- **Uptime**: System uptime in microseconds
- **Reset Reason**: Last reset cause
- **CPU Frequency**: Configured CPU frequency

### 9. **Security Information**
- **Secure Boot**: Status of secure boot feature
- **Flash Encryption**: Status of flash encryption

## Usage Examples

### Basic Usage
```c
#include "device_info.h"

void app_main(void)
{
    // Print all device information
    device_info_print_all();
}
```

### Selective Information Display
```c
// Print only chip and hardware features
device_info_print_chip();
device_info_print_hardware_features();

// Print only peripherals and clocks
device_info_print_peripherals();
device_info_print_clocks();

// Print only memory information
device_info_print_memory();
```

## Platform Compatibility

The enhanced device info module is designed to work across all ESP32 family devices:

- **ESP32**: Full feature support including dual-core capabilities
- **ESP32-S2**: Single-core with USB OTG and touch sensor details
- **ESP32-S3**: Dual-core with AI acceleration and USB OTG features
- **ESP32-C3**: RISC-V architecture with IEEE 802.15.4 support
- **Future variants**: Graceful degradation for unknown features

## Sample Output

```
I (1234) DEVICE_INFO: === ESP32 Device Information ===
I (1234) DEVICE_INFO: === Chip Information ===
I (1234) DEVICE_INFO:   Model: esp32
I (1234) DEVICE_INFO:   Revision: 3
I (1234) DEVICE_INFO:   Cores: 2
I (1234) DEVICE_INFO:   Features:
I (1234) DEVICE_INFO:     - WiFi 802.11 b/g/n
I (1234) DEVICE_INFO:     - Bluetooth Classic
I (1234) DEVICE_INFO:     - Bluetooth Low Energy (BLE)
I (1234) DEVICE_INFO:     - Embedded Flash
I (1234) DEVICE_INFO:     - Dual-core Xtensa LX6
I (1234) DEVICE_INFO:     - Hardware crypto acceleration
I (1234) DEVICE_INFO:     - RTC with deep sleep support
I (1234) DEVICE_INFO:   Flash Size: 4 MB
I (1234) DEVICE_INFO:   PSRAM: Available (4194304 bytes)
I (1234) DEVICE_INFO: === Hardware Features & Capabilities ===
I (1234) DEVICE_INFO:   GPIO Count: 40
I (1234) DEVICE_INFO:   ADC Units: 2
I (1234) DEVICE_INFO:   DAC Channels: 2
I (1234) DEVICE_INFO:   SPI Controllers: 4
I (1234) DEVICE_INFO:   I2C Controllers: 2
I (1234) DEVICE_INFO:   UART Controllers: 3
...
```

This enhanced module provides developers with comprehensive insight into their ESP32 hardware capabilities, enabling better optimization and feature utilization.
