#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_mac.h"
#include "esp_efuse.h"
#include "esp_partition.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#include "esp_efuse_table.h"
#include "esp_event.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "ping/ping_sock.h"

static const char *TAG = "DEVICE_INFO";

// WiFi configuration
#define WIFI_SSID      "HighSpeedFiberHome"
#define WIFI_PASS      "123456789000"
#define MAXIMUM_RETRY  5

// FreeRTOS event group to signal when we are connected
static EventGroupHandle_t s_wifi_event_group;

// The event group allows multiple bits for each event, but we only care about two events:
// - we are connected to the AP with an IP
// - we failed to connect after the maximum amount of retries
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
         .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

static void test_on_ping_success(esp_ping_handle_t hdl, void *args)
{
    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time, recv_len;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    ESP_LOGI(TAG, "%" PRIu32 " bytes from " IPSTR " icmp_seq=%d ttl=%d time=%" PRIu32 " ms",
           recv_len, IP2STR(&target_addr.u_addr.ip4), seqno, ttl, elapsed_time);
}

static void test_on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
    uint16_t seqno;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    ESP_LOGI(TAG, "From " IPSTR " icmp_seq=%d timeout", IP2STR(&target_addr.u_addr.ip4), seqno);
}

static void test_on_ping_end(esp_ping_handle_t hdl, void *args)
{
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
    ESP_LOGI(TAG, "\n--- " IPSTR " ping statistics ---", IP2STR(&target_addr.u_addr.ip4));
    ESP_LOGI(TAG, "%" PRIu32 " packets transmitted, %" PRIu32 " received, time %" PRIu32 "ms", transmitted, received, total_time_ms);
    if (transmitted > 0) {
        ESP_LOGI(TAG, "packet loss: %.1f%%", 100.0 * (transmitted - received) / transmitted);
    }
}

void ping_google(void)
{
    ESP_LOGI(TAG, "Starting ping to google.com...");
    
    ip_addr_t target_addr;
    // Google's public DNS server IP
    ipaddr_aton("8.8.8.8", &target_addr);
    
    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.target_addr = target_addr;
    ping_config.count = 5;
    ping_config.interval_ms = 1000;
    ping_config.timeout_ms = 3000;
    ping_config.data_size = 64;
    
    // Set callback functions
    esp_ping_callbacks_t cbs = {
        .on_ping_success = test_on_ping_success,
        .on_ping_timeout = test_on_ping_timeout,
        .on_ping_end = test_on_ping_end,
        .cb_args = NULL
    };
    
    esp_ping_handle_t ping;
    esp_ping_new_session(&ping_config, &cbs, &ping);
    esp_ping_start(ping);
    
    // Wait for ping to complete
    vTaskDelay(pdMS_TO_TICKS(10000));
    
    esp_ping_stop(ping);
    esp_ping_delete_session(ping);
}

void print_device_info(void)
{
    ESP_LOGI(TAG, "=== ESP32 Device Information ===");
    
    // Chip information
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    ESP_LOGI(TAG, "Chip Information:");
    ESP_LOGI(TAG, "  Model: %s", CONFIG_IDF_TARGET);
    ESP_LOGI(TAG, "  Revision: %d", chip_info.revision);
    ESP_LOGI(TAG, "  Cores: %d", chip_info.cores);
    ESP_LOGI(TAG, "  Features: %s%s%s%s",
             (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
             (chip_info.features & CHIP_FEATURE_BT) ? "BT/" : "",
             (chip_info.features & CHIP_FEATURE_BLE) ? "BLE/" : "",
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "Embedded Flash" : "External Flash");
      // Flash information
    uint32_t flash_size;
    esp_flash_get_size(NULL, &flash_size);
    ESP_LOGI(TAG, "  Flash Size: %" PRIu32 " MB", flash_size / (1024 * 1024));
    
    // MAC addresses
    uint8_t mac[6];
    ESP_LOGI(TAG, "MAC Addresses:");
    
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
        ESP_LOGI(TAG, "  WiFi STA: %02x:%02x:%02x:%02x:%02x:%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    
    if (esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP) == ESP_OK) {
        ESP_LOGI(TAG, "  WiFi AP:  %02x:%02x:%02x:%02x:%02x:%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    
    if (esp_read_mac(mac, ESP_MAC_BT) == ESP_OK) {
        ESP_LOGI(TAG, "  Bluetooth: %02x:%02x:%02x:%02x:%02x:%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
      // Memory information
    ESP_LOGI(TAG, "Memory Information:");
    ESP_LOGI(TAG, "  Free heap: %" PRIu32 " bytes", (uint32_t)esp_get_free_heap_size());
    ESP_LOGI(TAG, "  Minimum free heap: %" PRIu32 " bytes", (uint32_t)esp_get_minimum_free_heap_size());
    ESP_LOGI(TAG, "  Internal RAM free: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    ESP_LOGI(TAG, "  External RAM free: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
      // Partition information
    ESP_LOGI(TAG, "Partition Information:");
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
    if (partition != NULL) {
        ESP_LOGI(TAG, "  Running partition: %s", partition->label);
        ESP_LOGI(TAG, "  Partition address: 0x%08" PRIx32, (uint32_t)partition->address);
        ESP_LOGI(TAG, "  Partition size: %" PRIu32 " bytes", (uint32_t)partition->size);
    }
    
    // Boot partition
    const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
    if (boot_partition != NULL) {
        ESP_LOGI(TAG, "  Boot partition: %s", boot_partition->label);
    }
      // System information
    ESP_LOGI(TAG, "System Information:");
    ESP_LOGI(TAG, "  IDF Version: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "  Up time: %" PRId64 " us", esp_timer_get_time());
    ESP_LOGI(TAG, "  Reset reason: %d", esp_reset_reason());
    
    // CPU frequency
    ESP_LOGI(TAG, "  CPU Frequency: %d MHz", CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ);
    
    // Temperature (if available)
    #ifdef CONFIG_ESP32_PHY_CALIBRATION_AND_DATA_STORAGE
    // Note: Temperature sensor is not always available or accurate
    ESP_LOGI(TAG, "  Note: Temperature sensor reading not implemented in this example");
    #endif    // Security information
    ESP_LOGI(TAG, "Security Information:");
    
    // Check if secure boot is enabled
    #ifdef CONFIG_SECURE_BOOT
    bool secure_boot_enabled = esp_secure_boot_enabled();
    ESP_LOGI(TAG, "  Security boot: %s", secure_boot_enabled ? "Enabled" : "Disabled");
    #else
    ESP_LOGI(TAG, "  Security boot: Disabled (not configured)");
    #endif
    
    // Check flash encryption status using efuse API
    #ifdef CONFIG_SECURE_FLASH_ENC_ENABLED
    ESP_LOGI(TAG, "  Flash encryption: Enabled");
    #else
    ESP_LOGI(TAG, "  Flash encryption: Disabled");
    #endif
    
    ESP_LOGI(TAG, "=== End Device Information ===\n");
}

void app_main(void)
{
    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Print device information first
    print_device_info();
    
    // Initialize and connect to WiFi
    ESP_LOGI(TAG, "Starting WiFi connection...");
    wifi_init_sta();
    
    // Test ping to Google
    ping_google();
    
    // Main application loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000)); // Wait 30 seconds before next ping
        ping_google();
    }
}