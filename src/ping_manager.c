#include "ping_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

static const char *TAG = "ping_manager";

// Global state
static ping_target_t targets[PING_MAX_TARGETS];
static int target_count = 0;
static bool is_running = false;
static TaskHandle_t ping_task_handle = NULL;
static SemaphoreHandle_t targets_mutex = NULL;
static ping_result_callback_t result_callback = NULL;
static void* callback_user_data = NULL;

// Forward declarations
static void ping_task(void* parameters);
static bool check_host_alive(const char* ip_address, uint32_t timeout_ms);
static int find_target_by_name(const char* name);
static uint64_t get_timestamp_ms(void);

esp_err_t ping_manager_init(ping_result_callback_t callback, void* user_data) {
    ESP_LOGI(TAG, "Initializing ping manager");
    
    if (is_running) {
        ESP_LOGW(TAG, "Ping manager already initialized");
        return ESP_OK;
    }
    
    // Initialize state
    memset(targets, 0, sizeof(targets));
    target_count = 0;
    result_callback = callback;
    callback_user_data = user_data;
    
    // Create mutex for thread safety
    targets_mutex = xSemaphoreCreateMutex();
    if (targets_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }
    
    // Create ping task
    BaseType_t result = xTaskCreate(
        ping_task,
        "ping_task",
        4096,
        NULL,
        5,
        &ping_task_handle
    );
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create ping task");
        vSemaphoreDelete(targets_mutex);
        return ESP_ERR_NO_MEM;
    }
    
    is_running = true;
    ESP_LOGI(TAG, "Ping manager initialized successfully");
    return ESP_OK;
}

void ping_manager_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing ping manager");
    
    if (!is_running) {
        return;
    }
    
    is_running = false;
    
    // Delete task
    if (ping_task_handle != NULL) {
        vTaskDelete(ping_task_handle);
        ping_task_handle = NULL;
    }
    
    // Delete mutex
    if (targets_mutex != NULL) {
        vSemaphoreDelete(targets_mutex);
        targets_mutex = NULL;
    }
    
    // Clear state
    memset(targets, 0, sizeof(targets));
    target_count = 0;
    result_callback = NULL;
    callback_user_data = NULL;
    
    ESP_LOGI(TAG, "Ping manager deinitialized");
}

int ping_manager_add_device(const char* name, const char* ip_address) {
    if (!name || !ip_address) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }
    
    if (xSemaphoreTake(targets_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return -1;
    }
    
    // Check if target already exists
    int existing_index = find_target_by_name(name);
    if (existing_index >= 0) {
        ESP_LOGW(TAG, "Device '%s' already exists, updating IP address", name);
        strncpy(targets[existing_index].ip_address, ip_address, sizeof(targets[existing_index].ip_address) - 1);
        xSemaphoreGive(targets_mutex);
        return existing_index;
    }
    
    // Check if we have space
    if (target_count >= PING_MAX_TARGETS) {
        ESP_LOGE(TAG, "Maximum number of targets reached");
        xSemaphoreGive(targets_mutex);
        return -1;
    }
    
    // Add new target
    int index = target_count++;
    ping_target_t* target = &targets[index];
    
    strncpy(target->name, name, sizeof(target->name) - 1);
    strncpy(target->ip_address, ip_address, sizeof(target->ip_address) - 1);
    target->interval_ms = PING_DEFAULT_INTERVAL;
    target->timeout_ms = PING_DEFAULT_TIMEOUT;
    target->count = PING_DEFAULT_COUNT;
    target->enabled = true;
    target->is_online = false;
    target->success_count = 0;
    target->fail_count = 0;
    target->last_ping_time = 0;
    target->last_success_time = 0;
    
    xSemaphoreGive(targets_mutex);
    
    ESP_LOGI(TAG, "Added device '%s' at IP '%s' (index %d)", name, ip_address, index);
    return index;
}

esp_err_t ping_manager_remove_device(const char* name) {
    if (!name) {
        ESP_LOGE(TAG, "Invalid parameter");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(targets_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_ERR_TIMEOUT;
    }
    
    int index = find_target_by_name(name);
    if (index < 0) {
        ESP_LOGW(TAG, "Device '%s' not found", name);
        xSemaphoreGive(targets_mutex);
        return ESP_ERR_NOT_FOUND;
    }
    
    // Shift remaining targets down
    for (int i = index; i < target_count - 1; i++) {
        targets[i] = targets[i + 1];
    }
    
    target_count--;
    memset(&targets[target_count], 0, sizeof(ping_target_t));
    
    xSemaphoreGive(targets_mutex);
    
    ESP_LOGI(TAG, "Removed device '%s'", name);
    return ESP_OK;
}

esp_err_t ping_manager_set_device_enabled(const char* name, bool enabled) {
    if (!name) {
        ESP_LOGE(TAG, "Invalid parameter");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(targets_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_ERR_TIMEOUT;
    }
    
    int index = find_target_by_name(name);
    if (index < 0) {
        ESP_LOGW(TAG, "Device '%s' not found", name);
        xSemaphoreGive(targets_mutex);
        return ESP_ERR_NOT_FOUND;
    }
    
    targets[index].enabled = enabled;
    xSemaphoreGive(targets_mutex);
    
    ESP_LOGI(TAG, "Device '%s' %s", name, enabled ? "enabled" : "disabled");
    return ESP_OK;
}

const ping_target_t* ping_manager_get_device(const char* name) {
    if (!name) {
        return NULL;
    }
    
    if (xSemaphoreTake(targets_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return NULL;
    }
    
    int index = find_target_by_name(name);
    ping_target_t* result = (index >= 0) ? &targets[index] : NULL;
    
    xSemaphoreGive(targets_mutex);
    return result;
}

int ping_manager_get_target_count(void) {
    return target_count;
}

bool ping_manager_is_running(void) {
    return is_running;
}

// Private functions
static void ping_task(void* parameters) {
    ESP_LOGI(TAG, "Ping task started");
    
    while (is_running) {
        uint64_t current_time = get_timestamp_ms();
        
        if (xSemaphoreTake(targets_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            for (int i = 0; i < target_count; i++) {
                ping_target_t* target = &targets[i];
                
                if (!target->enabled) {
                    continue;
                }
                
                // Check if it's time to ping this target
                if (current_time - target->last_ping_time >= target->interval_ms) {
                    target->last_ping_time = current_time;
                    
                    // Perform ping check
                    uint64_t start_time = get_timestamp_ms();
                    bool success = check_host_alive(target->ip_address, target->timeout_ms);
                    uint64_t end_time = get_timestamp_ms();
                    uint32_t response_time = (uint32_t)(end_time - start_time);
                    
                    // Update statistics
                    bool status_changed = (target->is_online != success);
                    target->is_online = success;
                    
                    if (success) {
                        target->success_count++;
                        target->last_success_time = current_time;
                    } else {
                        target->fail_count++;
                    }
                    
                    // Log status changes
                    if (status_changed) {
                        ESP_LOGI(TAG, "Device '%s' (%s) status changed: %s",
                                target->name, target->ip_address,
                                success ? "ONLINE" : "OFFLINE");
                    }
                    
                    // Call callback if provided
                    if (result_callback) {
                        result_callback(target->name, target->ip_address, success, response_time, callback_user_data);
                    }
                }
            }
            xSemaphoreGive(targets_mutex);
        }
        
        // Wait before next iteration
        vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
    }
    
    ESP_LOGI(TAG, "Ping task ended");
    vTaskDelete(NULL);
}

static bool check_host_alive(const char* ip_address, uint32_t timeout_ms) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }
    
    // Set socket timeout
    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    // Set non-blocking mode
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80); // Try HTTP port first
    
    if (inet_pton(AF_INET, ip_address, &addr.sin_addr) <= 0) {
        close(sock);
        return false;
    }
    
    // Attempt connection
    int result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    bool is_alive = false;
    
    if (result == 0) {
        // Connected immediately
        is_alive = true;
    } else if (errno == EINPROGRESS) {
        // Connection in progress, wait for completion
        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(sock, &write_fds);
        
        struct timeval select_timeout;
        select_timeout.tv_sec = timeout_ms / 1000;
        select_timeout.tv_usec = (timeout_ms % 1000) * 1000;
        
        int select_result = select(sock + 1, NULL, &write_fds, NULL, &select_timeout);
        if (select_result > 0 && FD_ISSET(sock, &write_fds)) {
            int error = 0;
            socklen_t len = sizeof(error);
            if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && error == 0) {
                is_alive = true;
            }
        }
    }
    
    close(sock);
    
    // If HTTP port failed, try SSH port (22) as backup
    if (!is_alive) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock >= 0) {
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
            flags = fcntl(sock, F_GETFL, 0);
            fcntl(sock, F_SETFL, flags | O_NONBLOCK);
            
            addr.sin_port = htons(22); // SSH port
            result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
            
            if (result == 0) {
                is_alive = true;            } else if (errno == EINPROGRESS) {
                fd_set write_fds;
                FD_ZERO(&write_fds);
                FD_SET(sock, &write_fds);
                
                struct timeval ssh_timeout;
                ssh_timeout.tv_sec = timeout_ms / 1000;
                ssh_timeout.tv_usec = (timeout_ms % 1000) * 1000;
                
                if (select(sock + 1, NULL, &write_fds, NULL, &ssh_timeout) > 0 && FD_ISSET(sock, &write_fds)) {
                    int error = 0;
                    socklen_t len = sizeof(error);
                    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && error == 0) {
                        is_alive = true;
                    }
                }
            }
            close(sock);
        }
    }
    
    return is_alive;
}

static int find_target_by_name(const char* name) {
    for (int i = 0; i < target_count; i++) {
        if (strcmp(targets[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static uint64_t get_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}