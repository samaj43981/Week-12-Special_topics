#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_now.h"

static const char* TAG = "ESP_NOW_SENDER";

// MAC Address ของ Receiver (ต้องเปลี่ยนตามของจริง)
static uint8_t receiver_mac[6] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC};

// ข้อมูลที่จะส่ง
typedef struct {
    char message[200];
    int counter;
    float sensor_value;
} esp_now_data_t;

// Callback เมื่อส่งข้อมูลเสร็จ
void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI(TAG, "✅ Data sent successfully");
    } else {
        ESP_LOGE(TAG, "❌ Failed to send data");
    }
}

// ฟังก์ชันเริ่มต้น WiFi
void wifi_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi initialized");
}

// ฟังก์ชันเริ่มต้น ESP-NOW
void espnow_init(void) {
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(on_data_sent));
    
    // เพิ่ม Peer (Receiver)
    esp_now_peer_info_t peer_info = {};
    memcpy(peer_info.peer_addr, receiver_mac, 6);
    peer_info.channel = 0;
    peer_info.encrypt = false;
    
    ESP_ERROR_CHECK(esp_now_add_peer(&peer_info));
    ESP_LOGI(TAG, "ESP-NOW initialized and peer added");
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    
    wifi_init();
    espnow_init();
    
    esp_now_data_t send_data;
    int counter = 0;
    
    ESP_LOGI(TAG, "🚀 ESP-NOW Sender started");
    
    while (1) {
        // เตรียมข้อมูลที่จะส่ง
        sprintf(send_data.message, "Hello from Sender! Time: %d", counter);
        send_data.counter = counter++;
        send_data.sensor_value = 25.5 + (float)(counter % 10);
        
        // ส่งข้อมูล
        esp_err_t result = esp_now_send(receiver_mac, (uint8_t*)&send_data, sizeof(send_data));
        if (result == ESP_OK) {
            ESP_LOGI(TAG, "📤 Sending: %s (Counter: %d)", send_data.message, send_data.counter);
        } else {
            ESP_LOGE(TAG, "❌ Error sending data");
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // ส่งทุก 2 วินาที
    }
}
