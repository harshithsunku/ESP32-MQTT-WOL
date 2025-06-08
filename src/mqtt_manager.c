#include "mqtt_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_chip_info.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

static const char *TAG = "MQTT_MANAGER";

// MQTT client handle
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;
static mqtt_message_callback_t user_message_callback = NULL;

// Function prototypes
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static char* create_device_info_json(void);
static char* create_ping_result_json(const char* ip_address, bool success, uint32_t response_time);

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected to broker");
            mqtt_connected = true;
            
            // Send initial hello message
            mqtt_manager_send_hello();
            
            // Send device info
            mqtt_manager_send_device_info();
            
            // Subscribe to command topics
            mqtt_manager_subscribe("esp32/commands", MQTT_QOS_1);
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected from broker");
            mqtt_connected = false;
            break;
            
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT subscribed to topic, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT unsubscribed from topic, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT message published, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT message received:");
            ESP_LOGI(TAG, "  Topic: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "  Data: %.*s", event->data_len, event->data);
            
            // Call user callback if provided
            if (user_message_callback) {
                // Null-terminate the strings for callback
                char topic[event->topic_len + 1];
                char data[event->data_len + 1];
                
                memcpy(topic, event->topic, event->topic_len);
                topic[event->topic_len] = '\0';
                
                memcpy(data, event->data, event->data_len);
                data[event->data_len] = '\0';
                
                user_message_callback(topic, data, event->data_len);
            }
            break;
            
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error occurred");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGE(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
                ESP_LOGE(TAG, "Last captured errno : %d (%s)", event->error_handle->esp_transport_sock_errno,
                         strerror(event->error_handle->esp_transport_sock_errno));
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGE(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                ESP_LOGE(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            break;
              default:
            ESP_LOGI(TAG, "MQTT event: %" PRId32, event_id);
            break;
    }
}

esp_err_t mqtt_manager_init(mqtt_message_callback_t message_callback)
{
    if (mqtt_client != NULL) {
        ESP_LOGW(TAG, "MQTT manager already initialized");
        return ESP_OK;
    }
    
    user_message_callback = message_callback;    // MQTT client configuration
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = MQTT_BROKER_URI,
            .verification.crt_bundle_attach = esp_crt_bundle_attach,
        },
        .credentials = {
            .username = MQTT_USERNAME,
            .authentication.password = MQTT_PASSWORD,
            .client_id = MQTT_CLIENT_ID,
        },
        .network = {
            .timeout_ms = 10000,
            .refresh_connection_after_ms = 20000,
        },
        .session = {
            .keepalive = 60,
            .disable_clean_session = false,
        },
    };
    
    ESP_LOGI(TAG, "Initializing MQTT client...");
    ESP_LOGI(TAG, "Broker: %s", MQTT_BROKER_URI);
    ESP_LOGI(TAG, "Username: %s", MQTT_USERNAME);
    ESP_LOGI(TAG, "Client ID: %s", MQTT_CLIENT_ID);
    
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }
    
    // Register event handler
    esp_err_t ret = esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register MQTT event handler");
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        return ret;
    }
    
    // Start the MQTT client
    ret = esp_mqtt_client_start(mqtt_client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client");
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        return ret;
    }
    
    ESP_LOGI(TAG, "MQTT manager initialized successfully");
    return ESP_OK;
}

void mqtt_manager_deinit(void)
{
    if (mqtt_client != NULL) {
        esp_mqtt_client_stop(mqtt_client);
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        mqtt_connected = false;
        ESP_LOGI(TAG, "MQTT manager deinitialized");
    }
}

bool mqtt_manager_is_connected(void)
{
    return mqtt_connected;
}

esp_err_t mqtt_manager_send_hello(void)
{
    if (!mqtt_connected) {
        ESP_LOGW(TAG, "MQTT not connected, cannot send hello message");
        return ESP_FAIL;
    }
    
    // Create hello message with device info
    cJSON *json = cJSON_CreateObject();
    cJSON *message = cJSON_CreateString("Hello from ESP32 IoT Device!");
    cJSON *timestamp = cJSON_CreateNumber(esp_timer_get_time() / 1000); // milliseconds
    
    // Add device MAC address
    uint8_t mac[6];
    char mac_str[18];
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
        snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        cJSON *device_mac = cJSON_CreateString(mac_str);
        cJSON_AddItemToObject(json, "device_mac", device_mac);
    }
    
    cJSON_AddItemToObject(json, "message", message);
    cJSON_AddItemToObject(json, "timestamp", timestamp);
    
    char *json_string = cJSON_Print(json);
    if (json_string == NULL) {
        ESP_LOGE(TAG, "Failed to create hello JSON");
        cJSON_Delete(json);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Sending hello message: %s", json_string);
    
    int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_HELLO, json_string, 0, MQTT_QOS_1, 0);
    
    free(json_string);
    cJSON_Delete(json);
    
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish hello message");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Hello message sent successfully, msg_id=%d", msg_id);
    return ESP_OK;
}

esp_err_t mqtt_manager_send_status(const char* status)
{
    if (!mqtt_connected) {
        ESP_LOGW(TAG, "MQTT not connected, cannot send status");
        return ESP_FAIL;
    }
    
    if (status == NULL) {
        ESP_LOGE(TAG, "Status message is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    cJSON *json = cJSON_CreateObject();
    cJSON *status_msg = cJSON_CreateString(status);
    cJSON *timestamp = cJSON_CreateNumber(esp_timer_get_time() / 1000);
    
    cJSON_AddItemToObject(json, "status", status_msg);
    cJSON_AddItemToObject(json, "timestamp", timestamp);
    
    char *json_string = cJSON_Print(json);
    if (json_string == NULL) {
        ESP_LOGE(TAG, "Failed to create status JSON");
        cJSON_Delete(json);
        return ESP_FAIL;
    }
    
    int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_STATUS, json_string, 0, MQTT_QOS_1, 0);
    
    free(json_string);
    cJSON_Delete(json);
    
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish status message");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

static char* create_device_info_json(void)
{
    cJSON *json = cJSON_CreateObject();
    
    // Chip information
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    cJSON *chip = cJSON_CreateObject();
    cJSON_AddStringToObject(chip, "model", CONFIG_IDF_TARGET);
    cJSON_AddNumberToObject(chip, "revision", chip_info.revision);
    cJSON_AddNumberToObject(chip, "cores", chip_info.cores);
    
    // Features
    cJSON *features = cJSON_CreateArray();
    if (chip_info.features & CHIP_FEATURE_WIFI_BGN) {
        cJSON_AddItemToArray(features, cJSON_CreateString("WiFi"));
    }
    if (chip_info.features & CHIP_FEATURE_BT) {
        cJSON_AddItemToArray(features, cJSON_CreateString("Bluetooth Classic"));
    }
    if (chip_info.features & CHIP_FEATURE_BLE) {
        cJSON_AddItemToArray(features, cJSON_CreateString("Bluetooth LE"));
    }
    cJSON_AddItemToObject(chip, "features", features);
    
    // Memory info
    cJSON *memory = cJSON_CreateObject();
    cJSON_AddNumberToObject(memory, "free_heap", esp_get_free_heap_size());
    cJSON_AddNumberToObject(memory, "min_free_heap", esp_get_minimum_free_heap_size());
    
    // MAC addresses
    cJSON *mac_addresses = cJSON_CreateObject();
    uint8_t mac[6];
    char mac_str[18];
    
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
        snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        cJSON_AddStringToObject(mac_addresses, "wifi_sta", mac_str);
    }
    
    // Add all components to main JSON
    cJSON_AddItemToObject(json, "chip", chip);
    cJSON_AddItemToObject(json, "memory", memory);
    cJSON_AddItemToObject(json, "mac_addresses", mac_addresses);
    cJSON_AddStringToObject(json, "idf_version", esp_get_idf_version());
    cJSON_AddNumberToObject(json, "timestamp", esp_timer_get_time() / 1000);
    
    return cJSON_Print(json);
}

esp_err_t mqtt_manager_send_device_info(void)
{
    if (!mqtt_connected) {
        ESP_LOGW(TAG, "MQTT not connected, cannot send device info");
        return ESP_FAIL;
    }
    
    char *json_string = create_device_info_json();
    if (json_string == NULL) {
        ESP_LOGE(TAG, "Failed to create device info JSON");
        return ESP_FAIL;
    }
    
    int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_DEVICE_INFO, json_string, 0, MQTT_QOS_1, 0);
    
    free(json_string);
    
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish device info");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Device info sent successfully");
    return ESP_OK;
}

static char* create_ping_result_json(const char* ip_address, bool success, uint32_t response_time)
{
    cJSON *json = cJSON_CreateObject();
    
    cJSON_AddStringToObject(json, "target_ip", ip_address);
    cJSON_AddBoolToObject(json, "success", success);
    cJSON_AddNumberToObject(json, "response_time_ms", response_time);
    cJSON_AddNumberToObject(json, "timestamp", esp_timer_get_time() / 1000);
    
    return cJSON_Print(json);
}

esp_err_t mqtt_manager_send_ping_result(const char* ip_address, bool success, uint32_t response_time)
{
    if (!mqtt_connected) {
        return ESP_FAIL; // Silently fail if not connected to avoid spam
    }
    
    if (ip_address == NULL) {
        ESP_LOGE(TAG, "IP address is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    char *json_string = create_ping_result_json(ip_address, success, response_time);
    if (json_string == NULL) {
        ESP_LOGE(TAG, "Failed to create ping result JSON");
        return ESP_FAIL;
    }
    
    int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_PING_RESULTS, json_string, 0, MQTT_QOS_0, 0);
    
    free(json_string);
    
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish ping result");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t mqtt_manager_publish(const char* topic, const char* data, mqtt_qos_t qos, bool retain)
{
    if (!mqtt_connected) {
        ESP_LOGW(TAG, "MQTT not connected, cannot publish message");
        return ESP_FAIL;
    }
    
    if (topic == NULL || data == NULL) {
        ESP_LOGE(TAG, "Topic or data is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, data, 0, qos, retain ? 1 : 0);
    
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish message to topic: %s", topic);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Message published to %s, msg_id=%d", topic, msg_id);
    return ESP_OK;
}

esp_err_t mqtt_manager_subscribe(const char* topic, mqtt_qos_t qos)
{
    if (!mqtt_connected) {
        ESP_LOGW(TAG, "MQTT not connected, cannot subscribe");
        return ESP_FAIL;
    }
    
    if (topic == NULL) {
        ESP_LOGE(TAG, "Topic is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    int msg_id = esp_mqtt_client_subscribe(mqtt_client, topic, qos);
    
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to subscribe to topic: %s", topic);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Subscribed to %s, msg_id=%d", topic, msg_id);
    return ESP_OK;
}
