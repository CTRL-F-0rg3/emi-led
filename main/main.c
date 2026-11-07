#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"

#define TAG "EMI_BLE"
#define DEVICE_NAME "EMI-LED"

#define SERVICE_UUID 0xFFE0
#define CHAR_UUID    0xFFE1

static uint16_t char_handle = 0;
static uint16_t service_handle = 0;
static uint16_t conn_id = 0;
static esp_gatt_if_t gatts_if_global = 0;

// Funkcja pomocnicza do budowania pakietów rozgłoszeniowych (Advertising)
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x20,
    .max_interval = 0x40,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// GAP Event Handler (Zarządzanie połączeniami)
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (event == ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT) {
        esp_ble_gap_start_advertising(&adv_params);
    }
}

// GATTS Event Handler (Obsługa serwera i odbieranie danych)
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
            gatts_if_global = gatts_if;
            esp_ble_gap_set_device_name(DEVICE_NAME);
            esp_ble_gap_config_adv_data(&adv_data);
            
            esp_gatt_srvc_id_t srvc_id = {
                .is_primary = true,
                .id.inst_id = 0,
                .id.uuid.len = ESP_UUID_LEN_16,
                .id.uuid.uuid.uuid16 = SERVICE_UUID,
            };
            esp_ble_gatts_create_service(gatts_if, &srvc_id, 4);
            break;

        case ESP_GATTS_CREATE_EVT:
            service_handle = param->create.service_handle;
            esp_ble_gatts_start_service(service_handle);
            
            esp_bt_uuid_t char_uuid = {
                .len = ESP_UUID_LEN_16,
                .uuid.uuid16 = CHAR_UUID,
            };
            esp_ble_gatts_add_char(service_handle, &char_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, 
                                   ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE, NULL, NULL);
            break;

        case ESP_GATTS_ADD_CHAR_EVT:
            char_handle = param->add_char.attr_handle;
            break;

        case ESP_GATTS_CONNECT_EVT:
            conn_id = param->connect.conn_id;
            ESP_LOGI(TAG, "Urządzenie podłączone!");
            esp_ble_gap_stop_advertising();
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "Urządzenie rozłączone. Wznawiam rozgłaszanie...");
            esp_ble_gap_start_advertising(&adv_params);
            break;

        case ESP_GATTS_WRITE_EVT:
            if (param->write.handle == char_handle && param->write.len >= 3) {
                uint8_t *data = param->write.value;
                ESP_LOGI(TAG, "Odebrano RGB: R=%d, G=%d, B=%d", data[0], data[1], data[2]);
                led_set_color(data[0], data[1], data[2]);
            }
            break;

        default:
            break;
    }
}

void app_main(void) {
    // 1. NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. LED
    ESP_ERROR_CHECK(led_init(LED_GPIO_DEFAULT));

    // 3. Bluetooth
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    // 4. Rejestracja zdarzeń
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(0));

    ESP_LOGI(TAG, "System gotowy. Rozgłaszam sieć jako %s", DEVICE_NAME);
}
