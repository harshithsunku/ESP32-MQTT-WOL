#include "ping_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "esp_netif.h"
#include <inttypes.h>
#include <string.h>

static const char *TAG = "PING_MANAGER";

// Task parameters
#define PING_TASK_STACK_SIZE    4096
#define PING_TASK_PRIORITY      5
#define PING_QUEUE_SIZE         10
#define PING_TASK_DELAY_MS      100

// Static variables
static ping_target_t ping_targets[PING_MAX_TARGETS];
static int active_target_count = 0;
static TaskHandle_t ping_task_handle = NULL;
static QueueHandle_t ping_cmd_queue = NULL;
static SemaphoreHandle_t ping_mutex = NULL;
static bool ping_manager_running = false;
static ping_result_callback_t result_callback = NULL;
static void* callback_user_data = NULL;

// Forward declarations
static void ping_task(void* parameter);
static esp_err_t execute_ping_command(ping_cmd_t* cmd);
static int find_free_target_slot(void);
static int find_target_by_ip(const char* ip_address);
static esp_err_t perform_single_ping(ping_target_t* target);
static void ping_success_cb(esp_ping_handle_t hdl, void *args);
static void ping_timeout_cb(esp_ping_handle_t hdl, void *args);
static void ping_end_cb(esp_ping_handle_t hdl, void *args);

// Ping callback context
typedef struct {
    ping_target_t* target;
    bool success;
    uint32_t response_time;
} ping_context_t;

static void ping_success_cb(esp_ping_handle_t hdl, void *args)
{
    ping_context_t* ctx = (ping_context_t*)args;
    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time, recv_len;
    ip_addr_t target_addr;
    
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    
    ESP_LOGD(TAG, "%" PRIu32 " bytes from " IPSTR " icmp_seq=%d ttl=%d time=%" PRIu32 " ms",
           recv_len, IP2STR(&target_addr.u_addr.ip4), seqno, ttl, elapsed_time);
    
    if (ctx) {
        ctx->success = true;
        ctx->response_time = elapsed_time;
    }
}

static void ping_timeout_cb(esp_ping_handle_t hdl, void *args)
{
    ping_context_t* ctx = (ping_context_t*)args;
    uint16_t seqno;
    ip_addr_t target_addr;
    
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    
    ESP_LOGD(TAG, "From " IPSTR " icmp_seq=%d timeout", IP2STR(&target_addr.u_addr.ip4), seqno);
    
    if (ctx) {
        ctx->success = false;
        ctx->response_time = 0;
    }
}

static void ping_end_cb(esp_ping_handle_t hdl, void *args)
{
    ping_context_t* ctx = (ping_context_t*)args;
    uint32_t transmitted, received;
    ip_addr_t target_addr;
    
    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    
    if (ctx && ctx->target) {
        if (received > 0) {
            ctx->target->success_count++;
            if (result_callback) {
                result_callback(ctx->target->ip_address, true, ctx->response_time, callback_user_data);
            }
        } else {
            ctx->target->fail_count++;
            if (result_callback) {
                result_callback(ctx->target->ip_address, false, 0, callback_user_data);
            }
        }
          ESP_LOGD(TAG, "Ping to %s: %s (Success: %" PRIu32 ", Fails: %" PRIu32 ")", 
                ctx->target->ip_address, 
                received > 0 ? "Success" : "Failed",
                ctx->target->success_count,
                ctx->target->fail_count);
    }
}

static esp_err_t perform_single_ping(ping_target_t* target)
{
    if (!target || !target->enabled) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ip_addr_t target_addr;
    if (!ipaddr_aton(target->ip_address, &target_addr)) {
        ESP_LOGE(TAG, "Invalid IP address: %s", target->ip_address);
        return ESP_ERR_INVALID_ARG;
    }
    
    ping_context_t context = {
        .target = target,
        .success = false,
        .response_time = 0
    };
    
    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.target_addr = target_addr;
    ping_config.count = target->count;
    ping_config.interval_ms = 1000;
    ping_config.timeout_ms = target->timeout_ms;
    ping_config.data_size = 64;
    
    esp_ping_callbacks_t cbs = {
        .on_ping_success = ping_success_cb,
        .on_ping_timeout = ping_timeout_cb,
        .on_ping_end = ping_end_cb,
        .cb_args = &context
    };
    
    esp_ping_handle_t ping;
    esp_err_t ret = esp_ping_new_session(&ping_config, &cbs, &ping);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ping session for %s: %s", target->ip_address, esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_ping_start(ping);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start ping for %s: %s", target->ip_address, esp_err_to_name(ret));
        esp_ping_delete_session(ping);
        return ret;
    }
    
    // Wait for ping completion
    uint32_t wait_time = (target->count * 1000) + target->timeout_ms + 1000;
    vTaskDelay(pdMS_TO_TICKS(wait_time));
    
    esp_ping_stop(ping);
    esp_ping_delete_session(ping);
    
    target->last_ping_time = esp_timer_get_time();
    
    return ESP_OK;
}

static void ping_task(void* parameter)
{
    ping_cmd_t cmd;
    TickType_t last_wake_time = xTaskGetTickCount();
    
    ESP_LOGI(TAG, "Ping task started");
    
    while (ping_manager_running) {
        // Process commands
        while (xQueueReceive(ping_cmd_queue, &cmd, 0) == pdTRUE) {
            execute_ping_command(&cmd);
        }
        
        // Check and ping targets
        if (xSemaphoreTake(ping_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            uint64_t current_time = esp_timer_get_time();
            
            for (int i = 0; i < PING_MAX_TARGETS; i++) {
                ping_target_t* target = &ping_targets[i];
                
                if (!target->enabled || strlen(target->ip_address) == 0) {
                    continue;
                }
                
                // Check if it's time to ping this target
                uint64_t time_since_last = current_time - target->last_ping_time;
                if (time_since_last >= (target->interval_ms * 1000)) {
                    ESP_LOGD(TAG, "Pinging target %s", target->ip_address);
                    perform_single_ping(target);
                }
            }
            
            xSemaphoreGive(ping_mutex);
        }
        
        // Sleep for a short time
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(PING_TASK_DELAY_MS));
    }
    
    ESP_LOGI(TAG, "Ping task stopped");
    vTaskDelete(NULL);
}

static esp_err_t execute_ping_command(ping_cmd_t* cmd)
{
    if (!cmd) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret = ESP_OK;
    
    if (xSemaphoreTake(ping_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        switch (cmd->cmd_type) {
            case PING_CMD_ADD_TARGET: {
                int slot = find_free_target_slot();
                if (slot >= 0) {
                    memcpy(&ping_targets[slot], &cmd->target, sizeof(ping_target_t));
                    ping_targets[slot].enabled = true;
                    ping_targets[slot].last_ping_time = 0;
                    ping_targets[slot].success_count = 0;
                    ping_targets[slot].fail_count = 0;
                    active_target_count++;
                    ESP_LOGI(TAG, "Added target %s at slot %d", cmd->target.ip_address, slot);
                } else {
                    ret = ESP_ERR_NO_MEM;
                }
                break;
            }
            case PING_CMD_REMOVE_TARGET: {
                if (cmd->target_index >= 0 && cmd->target_index < PING_MAX_TARGETS) {
                    if (strlen(ping_targets[cmd->target_index].ip_address) > 0) {
                        ESP_LOGI(TAG, "Removed target %s from slot %d", 
                                ping_targets[cmd->target_index].ip_address, cmd->target_index);
                        memset(&ping_targets[cmd->target_index], 0, sizeof(ping_target_t));
                        active_target_count--;
                    }
                } else {
                    ret = ESP_ERR_INVALID_ARG;
                }
                break;
            }
            case PING_CMD_UPDATE_TARGET: {
                if (cmd->target_index >= 0 && cmd->target_index < PING_MAX_TARGETS) {
                    ping_target_t* target = &ping_targets[cmd->target_index];
                    if (strlen(target->ip_address) > 0) {
                        if (cmd->target.interval_ms > 0) target->interval_ms = cmd->target.interval_ms;
                        if (cmd->target.timeout_ms > 0) target->timeout_ms = cmd->target.timeout_ms;
                        if (cmd->target.count > 0) target->count = cmd->target.count;
                        ESP_LOGI(TAG, "Updated target %s", target->ip_address);
                    }
                } else {
                    ret = ESP_ERR_INVALID_ARG;
                }
                break;
            }
            case PING_CMD_ENABLE_TARGET:
            case PING_CMD_DISABLE_TARGET: {
                if (cmd->target_index >= 0 && cmd->target_index < PING_MAX_TARGETS) {
                    ping_targets[cmd->target_index].enabled = (cmd->cmd_type == PING_CMD_ENABLE_TARGET);
                    ESP_LOGI(TAG, "%s target %s", 
                            ping_targets[cmd->target_index].enabled ? "Enabled" : "Disabled",
                            ping_targets[cmd->target_index].ip_address);
                } else {
                    ret = ESP_ERR_INVALID_ARG;
                }
                break;
            }
            case PING_CMD_STOP_ALL: {
                ping_manager_running = false;
                break;
            }
        }
        
        xSemaphoreGive(ping_mutex);
    } else {
        ret = ESP_ERR_TIMEOUT;
    }
    
    return ret;
}

static int find_free_target_slot(void)
{
    for (int i = 0; i < PING_MAX_TARGETS; i++) {
        if (strlen(ping_targets[i].ip_address) == 0) {
            return i;
        }
    }
    return -1;
}

static int find_target_by_ip(const char* ip_address)
{
    if (!ip_address) return -1;
    
    for (int i = 0; i < PING_MAX_TARGETS; i++) {
        if (strcmp(ping_targets[i].ip_address, ip_address) == 0) {
            return i;
        }
    }
    return -1;
}

// Public API implementations
esp_err_t ping_manager_init(ping_result_callback_t callback, void* user_data)
{
    if (ping_manager_running) {
        ESP_LOGW(TAG, "Ping manager already running");
        return ESP_OK;
    }
    
    // Initialize target array
    memset(ping_targets, 0, sizeof(ping_targets));
    active_target_count = 0;
    result_callback = callback;
    callback_user_data = user_data;
    
    // Create mutex
    ping_mutex = xSemaphoreCreateMutex();
    if (!ping_mutex) {
        ESP_LOGE(TAG, "Failed to create ping mutex");
        return ESP_FAIL;
    }
    
    // Create command queue
    ping_cmd_queue = xQueueCreate(PING_QUEUE_SIZE, sizeof(ping_cmd_t));
    if (!ping_cmd_queue) {
        ESP_LOGE(TAG, "Failed to create ping command queue");
        vSemaphoreDelete(ping_mutex);
        return ESP_FAIL;
    }
    
    // Start ping task
    ping_manager_running = true;
    BaseType_t task_result = xTaskCreate(
        ping_task,
        "ping_task",
        PING_TASK_STACK_SIZE,
        NULL,
        PING_TASK_PRIORITY,
        &ping_task_handle
    );
    
    if (task_result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create ping task");
        ping_manager_running = false;
        vQueueDelete(ping_cmd_queue);
        vSemaphoreDelete(ping_mutex);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Ping manager initialized with continuous ping thread");
    return ESP_OK;
}

int ping_manager_add_target(const char* ip_address, uint32_t interval_ms, uint32_t timeout_ms, uint32_t count)
{
    if (!ping_manager_running || !ip_address) {
        ESP_LOGE(TAG, "Ping manager not running or invalid IP");
        return -1;
    }
    
    // Check if target already exists
    int existing = find_target_by_ip(ip_address);
    if (existing >= 0) {
        ESP_LOGW(TAG, "Target %s already exists at index %d", ip_address, existing);
        return existing;
    }
    
    ping_cmd_t cmd = {
        .cmd_type = PING_CMD_ADD_TARGET,
        .target = {
            .interval_ms = interval_ms > 0 ? interval_ms : PING_DEFAULT_INTERVAL,
            .timeout_ms = timeout_ms > 0 ? timeout_ms : PING_DEFAULT_TIMEOUT,
            .count = count > 0 ? count : PING_DEFAULT_COUNT,
            .enabled = true,
            .last_ping_time = 0,
            .success_count = 0,
            .fail_count = 0
        }
    };
    
    strncpy(cmd.target.ip_address, ip_address, PING_MAX_IP_LEN - 1);
    cmd.target.ip_address[PING_MAX_IP_LEN - 1] = '\0';
    
    if (xQueueSend(ping_cmd_queue, &cmd, pdMS_TO_TICKS(1000)) == pdTRUE) {
        // Find the slot that was assigned (simple approach - find by IP after adding)
        vTaskDelay(pdMS_TO_TICKS(100)); // Give time for command to be processed
        return find_target_by_ip(ip_address);
    }
    
    ESP_LOGE(TAG, "Failed to queue add target command");
    return -1;
}

esp_err_t ping_manager_remove_target(int target_index)
{
    if (!ping_manager_running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ping_cmd_t cmd = {
        .cmd_type = PING_CMD_REMOVE_TARGET,
        .target_index = target_index
    };
    
    if (xQueueSend(ping_cmd_queue, &cmd, pdMS_TO_TICKS(1000)) == pdTRUE) {
        return ESP_OK;
    }
    
    return ESP_ERR_TIMEOUT;
}

esp_err_t ping_manager_update_target(int target_index, uint32_t interval_ms, uint32_t timeout_ms, uint32_t count)
{
    if (!ping_manager_running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ping_cmd_t cmd = {
        .cmd_type = PING_CMD_UPDATE_TARGET,
        .target_index = target_index,
        .target = {
            .interval_ms = interval_ms,
            .timeout_ms = timeout_ms,
            .count = count
        }
    };
    
    if (xQueueSend(ping_cmd_queue, &cmd, pdMS_TO_TICKS(1000)) == pdTRUE) {
        return ESP_OK;
    }
    
    return ESP_ERR_TIMEOUT;
}

esp_err_t ping_manager_set_target_enabled(int target_index, bool enabled)
{
    if (!ping_manager_running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ping_cmd_t cmd = {
        .cmd_type = enabled ? PING_CMD_ENABLE_TARGET : PING_CMD_DISABLE_TARGET,
        .target_index = target_index
    };
    
    if (xQueueSend(ping_cmd_queue, &cmd, pdMS_TO_TICKS(1000)) == pdTRUE) {
        return ESP_OK;
    }
    
    return ESP_ERR_TIMEOUT;
}

esp_err_t ping_manager_get_target_stats(int target_index, ping_target_t* target)
{
    if (!ping_manager_running || !target || target_index < 0 || target_index >= PING_MAX_TARGETS) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(ping_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        if (strlen(ping_targets[target_index].ip_address) > 0) {
            memcpy(target, &ping_targets[target_index], sizeof(ping_target_t));
            xSemaphoreGive(ping_mutex);
            return ESP_OK;
        }
        xSemaphoreGive(ping_mutex);
    }
    
    return ESP_ERR_NOT_FOUND;
}

esp_err_t ping_manager_get_all_targets(ping_target_t targets[], int* count)
{
    if (!ping_manager_running || !targets || !count) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *count = 0;
    
    if (xSemaphoreTake(ping_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        for (int i = 0; i < PING_MAX_TARGETS; i++) {
            if (strlen(ping_targets[i].ip_address) > 0) {
                memcpy(&targets[*count], &ping_targets[i], sizeof(ping_target_t));
                (*count)++;
            }
        }
        xSemaphoreGive(ping_mutex);
        return ESP_OK;
    }
    
    return ESP_ERR_TIMEOUT;
}

esp_err_t ping_manager_ping_once(const char* target_ip, uint32_t count, uint32_t timeout_ms)
{
    if (!target_ip) {
        ESP_LOGE(TAG, "Invalid target IP");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "One-time ping to %s...", target_ip);
    
    ip_addr_t target_addr;
    if (!ipaddr_aton(target_ip, &target_addr)) {
        ESP_LOGE(TAG, "Invalid IP address: %s", target_ip);
        return ESP_ERR_INVALID_ARG;
    }
    
    ping_context_t context = {
        .target = NULL,
        .success = false,
        .response_time = 0
    };
    
    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.target_addr = target_addr;
    ping_config.count = count;
    ping_config.interval_ms = 1000;
    ping_config.timeout_ms = timeout_ms;
    ping_config.data_size = 64;
    
    esp_ping_callbacks_t cbs = {
        .on_ping_success = ping_success_cb,
        .on_ping_timeout = ping_timeout_cb,
        .on_ping_end = ping_end_cb,
        .cb_args = &context
    };
    
    esp_ping_handle_t ping;
    esp_err_t ret = esp_ping_new_session(&ping_config, &cbs, &ping);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ping session: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_ping_start(ping);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start ping: %s", esp_err_to_name(ret));
        esp_ping_delete_session(ping);
        return ret;
    }
    
    // Wait for ping completion
    uint32_t wait_time = (count * 1000) + timeout_ms + 1000;
    vTaskDelay(pdMS_TO_TICKS(wait_time));
    
    esp_ping_stop(ping);
    esp_ping_delete_session(ping);
    
    return ESP_OK;
}

esp_err_t ping_manager_ping_google(void)
{
    return ping_manager_ping_once("8.8.8.8", 3, 2000);
}

void ping_manager_deinit(void)
{
    if (!ping_manager_running) {
        return;
    }
    
    ESP_LOGI(TAG, "Stopping ping manager...");
    
    // Send stop command
    ping_cmd_t cmd = { .cmd_type = PING_CMD_STOP_ALL };
    xQueueSend(ping_cmd_queue, &cmd, pdMS_TO_TICKS(1000));
    
    // Wait for task to finish
    if (ping_task_handle) {
        vTaskDelay(pdMS_TO_TICKS(500)); // Give task time to cleanup
        ping_task_handle = NULL;
    }
    
    // Clean up resources
    if (ping_cmd_queue) {
        vQueueDelete(ping_cmd_queue);
        ping_cmd_queue = NULL;
    }
    
    if (ping_mutex) {
        vSemaphoreDelete(ping_mutex);
        ping_mutex = NULL;
    }
    
    // Clear targets
    memset(ping_targets, 0, sizeof(ping_targets));
    active_target_count = 0;
    ping_manager_running = false;
    result_callback = NULL;
    callback_user_data = NULL;
    
    ESP_LOGI(TAG, "Ping manager stopped");
}
