#include "wol_manager.h"
#include "ping_manager.h"
#include "mqtt_manager.h"
#include "esp_log.h"
#include "esp_err.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
#include "cJSON.h"

static const char *TAG = "WOL_MANAGER";

// Device list storage
static wol_device_t devices[MAX_DEVICES];
static int device_count = 0;
static bool initialized = false;

// Mutex for thread safety
static SemaphoreHandle_t device_mutex = NULL;

esp_err_t wol_manager_init(void)
{
    if (initialized) {
        ESP_LOGW(TAG, "WoL manager already initialized");
        return ESP_OK;
    }

    // Create mutex for thread safety
    device_mutex = xSemaphoreCreateMutex();
    if (device_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create device mutex");
        return ESP_FAIL;
    }

    // Clear device list
    memset(devices, 0, sizeof(devices));
    device_count = 0;

    // Load device configuration
    wol_load_device_config();

    initialized = true;
    ESP_LOGI(TAG, "WoL manager initialized with %d devices", device_count);
    
    return ESP_OK;
}

esp_err_t wol_add_device(const char* name, const char* ip_address, 
                        const uint8_t* mac_address, const char* description)
{
    if (!name || !ip_address || !mac_address) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(device_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Check if device already exists
    for (int i = 0; i < device_count; i++) {
        if (strcmp(devices[i].name, name) == 0) {
            xSemaphoreGive(device_mutex);
            ESP_LOGW(TAG, "Device %s already exists, updating", name);
            // Update existing device
            strncpy(devices[i].ip_address, ip_address, sizeof(devices[i].ip_address) - 1);
            memcpy(devices[i].mac_address, mac_address, 6);
            if (description) {
                strncpy(devices[i].description, description, sizeof(devices[i].description) - 1);
            }
            return ESP_OK;
        }
    }

    // Check if we have space for new device
    if (device_count >= MAX_DEVICES) {
        xSemaphoreGive(device_mutex);
        ESP_LOGE(TAG, "Device list full, cannot add %s", name);
        return ESP_ERR_NO_MEM;
    }

    // Add new device
    wol_device_t* device = &devices[device_count];
    strncpy(device->name, name, sizeof(device->name) - 1);
    strncpy(device->ip_address, ip_address, sizeof(device->ip_address) - 1);
    memcpy(device->mac_address, mac_address, 6);
    if (description) {
        strncpy(device->description, description, sizeof(device->description) - 1);
    }
    device->status = DEVICE_STATUS_UNKNOWN;
    device->last_ping_time = 0;
    device->enabled = true;    device->wol_port = 9; // Default WoL port
    
    device_count++;
    
    xSemaphoreGive(device_mutex);
    
    ESP_LOGI(TAG, "Added device: %s (%s) - %02x:%02x:%02x:%02x:%02x:%02x", 
             name, ip_address, 
             mac_address[0], mac_address[1], mac_address[2], 
             mac_address[3], mac_address[4], mac_address[5]);
    
    // Automatically add to ping monitoring
    ping_manager_add_device(name, ip_address);
    
    return ESP_OK;
}

esp_err_t wol_remove_device(const char* name)
{
    if (!name) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(device_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Find device
    for (int i = 0; i < device_count; i++) {
        if (strcmp(devices[i].name, name) == 0) {
            // Shift remaining devices
            for (int j = i; j < device_count - 1; j++) {
                devices[j] = devices[j + 1];
            }
            device_count--;
              xSemaphoreGive(device_mutex);
            
            // Remove from ping monitoring
            ping_manager_remove_device(name);
            
            ESP_LOGI(TAG, "Removed device: %s", name);
            return ESP_OK;
        }
    }

    xSemaphoreGive(device_mutex);
    ESP_LOGW(TAG, "Device not found: %s", name);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t wol_send_packet(const uint8_t* mac_address, const char* broadcast_ip)
{
    if (!mac_address) {
        return ESP_ERR_INVALID_ARG;
    }

    // Create WoL magic packet
    uint8_t wol_packet[102];
    int packet_index = 0;

    // Fill first 6 bytes with 0xFF
    for (int i = 0; i < 6; i++) {
        wol_packet[packet_index++] = 0xFF;
    }

    // Repeat MAC address 16 times
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 6; j++) {
            wol_packet[packet_index++] = mac_address[j];
        }
    }

    // Create UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        return ESP_FAIL;
    }

    // Enable broadcast
    int broadcast_enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        ESP_LOGE(TAG, "Failed to enable broadcast");
        close(sock);
        return ESP_FAIL;
    }

    // Set destination address
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(9); // WoL port
    
    if (broadcast_ip) {
        inet_pton(AF_INET, broadcast_ip, &dest_addr.sin_addr);
    } else {
        dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    }

    // Send packet
    int bytes_sent = sendto(sock, wol_packet, sizeof(wol_packet), 0, 
                           (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    
    close(sock);

    if (bytes_sent < 0) {
        ESP_LOGE(TAG, "Failed to send WoL packet");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "WoL packet sent (%d bytes) to %02x:%02x:%02x:%02x:%02x:%02x", 
             bytes_sent,
             mac_address[0], mac_address[1], mac_address[2], 
             mac_address[3], mac_address[4], mac_address[5]);
    
    return ESP_OK;
}

esp_err_t wol_wake_device(const char* name)
{
    if (!name) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(device_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Find device
    wol_device_t* device = NULL;
    for (int i = 0; i < device_count; i++) {
        if (strcmp(devices[i].name, name) == 0) {
            device = &devices[i];
            break;
        }
    }

    if (!device) {
        xSemaphoreGive(device_mutex);
        ESP_LOGE(TAG, "Device not found: %s", name);
        return ESP_ERR_NOT_FOUND;
    }

    if (!device->enabled) {
        xSemaphoreGive(device_mutex);
        ESP_LOGW(TAG, "Device %s is disabled", name);
        return ESP_ERR_INVALID_STATE;
    }

    // Update device status to waking
    device->status = DEVICE_STATUS_WAKING;

    // Get device info for WoL
    uint8_t mac[6];
    memcpy(mac, device->mac_address, 6);
    
    xSemaphoreGive(device_mutex);

    // Send WoL packet
    esp_err_t result = wol_send_packet(mac, NULL);
    
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Wake-on-LAN sent to device: %s", name);
        
        // Publish MQTT status
        char topic[64];
        char message[128];
        snprintf(topic, sizeof(topic), "esp32/wol/%s/status", name);
        snprintf(message, sizeof(message), "{\"device\":\"%s\",\"action\":\"wake_sent\",\"timestamp\":%lu}", 
                 name, (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS / 1000));
        mqtt_publish(topic, message);
    }
    
    return result;
}

esp_err_t wol_update_device_status(const char* name, bool is_online)
{
    if (!name) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(device_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Find device
    for (int i = 0; i < device_count; i++) {
        if (strcmp(devices[i].name, name) == 0) {
            device_status_t old_status = devices[i].status;
            devices[i].status = is_online ? DEVICE_STATUS_ONLINE : DEVICE_STATUS_OFFLINE;
            
            if (is_online) {
                devices[i].last_ping_time = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;
            }

            // Log status change
            if (old_status != devices[i].status) {
                ESP_LOGI(TAG, "Device %s status changed: %s -> %s", 
                         name, wol_get_status_string(old_status), wol_get_status_string(devices[i].status));
                
                // Publish MQTT status update
                char topic[64];
                char message[256];
                snprintf(topic, sizeof(topic), "esp32/device/%s/status", name);
                snprintf(message, sizeof(message), 
                         "{\"device\":\"%s\",\"status\":\"%s\",\"ip\":\"%s\",\"timestamp\":%lu}", 
                         name, wol_get_status_string(devices[i].status), 
                         devices[i].ip_address, devices[i].last_ping_time);
                mqtt_publish(topic, message);
            }
            
            xSemaphoreGive(device_mutex);
            return ESP_OK;
        }
    }

    xSemaphoreGive(device_mutex);
    return ESP_ERR_NOT_FOUND;
}

const wol_device_t* wol_get_device(const char* name)
{
    if (!name) {
        return NULL;
    }

    for (int i = 0; i < device_count; i++) {
        if (strcmp(devices[i].name, name) == 0) {
            return &devices[i];
        }
    }
    
    return NULL;
}

const wol_device_t* wol_get_all_devices(int* device_count_out)
{
    if (device_count_out) {
        *device_count_out = device_count;
    }
    return devices;
}

esp_err_t wol_set_device_enabled(const char* name, bool enabled)
{
    if (!name) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(device_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    for (int i = 0; i < device_count; i++) {
        if (strcmp(devices[i].name, name) == 0) {
            devices[i].enabled = enabled;
            xSemaphoreGive(device_mutex);
            ESP_LOGI(TAG, "Device %s %s", name, enabled ? "enabled" : "disabled");
            return ESP_OK;
        }
    }

    xSemaphoreGive(device_mutex);
    return ESP_ERR_NOT_FOUND;
}

const char* wol_get_status_string(device_status_t status)
{
    switch (status) {
        case DEVICE_STATUS_ONLINE:  return "online";
        case DEVICE_STATUS_OFFLINE: return "offline";
        case DEVICE_STATUS_WAKING:  return "waking";
        default:                    return "unknown";
    }
}

esp_err_t wol_handle_mqtt_command(const char* device_name, const char* command)
{
    if (!device_name || !command) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "MQTT WoL command: %s -> %s", device_name, command);

    if (strcmp(command, "on") == 0 || strcmp(command, "wake") == 0) {
        return wol_wake_device(device_name);
    } else if (strcmp(command, "status") == 0) {
        const wol_device_t* device = wol_get_device(device_name);
        if (device) {
            char topic[64];
            char message[256];
            snprintf(topic, sizeof(topic), "esp32/device/%s/status", device_name);
            snprintf(message, sizeof(message), 
                     "{\"device\":\"%s\",\"status\":\"%s\",\"ip\":\"%s\",\"enabled\":%s,\"timestamp\":%lu}", 
                     device->name, wol_get_status_string(device->status), 
                     device->ip_address, device->enabled ? "true" : "false",
                     device->last_ping_time);
            mqtt_publish(topic, message);
            return ESP_OK;
        } else {
            return ESP_ERR_NOT_FOUND;
        }
    } else if (strcmp(command, "enable") == 0) {
        return wol_set_device_enabled(device_name, true);
    } else if (strcmp(command, "disable") == 0) {
        return wol_set_device_enabled(device_name, false);
    }

    ESP_LOGW(TAG, "Unknown WoL command: %s", command);
    return ESP_ERR_INVALID_ARG;
}

// Default device configuration
esp_err_t wol_load_device_config(void)
{
    ESP_LOGI(TAG, "Loading default device configuration");
    
    // Example devices - you can modify these or load from NVS/file
    uint8_t mac1[] = {0xC0, 0x18, 0x50, 0xAC, 0xE1, 0xA5};
    uint8_t mac2[] = {0x00, 0x50, 0x56, 0xAB, 0xCD, 0xEF};
    //uint8_t mac3[] = {0x08, 0x00, 0x27, 0x12, 0x34, 0x56};
    
    wol_add_device("server1", "192.168.0.111", mac1, "Main Server");
    wol_add_device("desktop1", "192.168.0.112", mac2, "Development Desktop");
    wol_add_device("nas1", "192.168.0.2", mac1, "Network Attached Storage");
    
    return ESP_OK;
}

esp_err_t wol_save_device_config(void)
{
    // TODO: Implement saving to NVS or file system
    ESP_LOGI(TAG, "Device configuration saved (placeholder)");
    return ESP_OK;
}
