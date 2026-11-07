#include "led.h"
#include "esp_log.h"

static const char *TAG = "LED";
static led_strip_handle_t led_strip;

esp_err_t led_init(int gpio_num)
{
    ESP_LOGI(TAG, "Inicjalizacja diody LED na porcie GPIO%d", gpio_num);

    // Konfiguracja paska LED
    led_strip_config_t strip_config = {
        .strip_gpio_num = gpio_num, // Użycie konfigurowalnego portu
        .max_leds = LED_STRIP_LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    // Konfiguracja backendu RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10 MHz
        .flags.with_dma = false,
    };

    // Utworzenie urządzenia
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    
    // Upewnienie się, że dioda jest wyłączona na starcie
    led_off();
    return ESP_OK;
}

esp_err_t led_set_color(uint8_t red, uint8_t green, uint8_t blue)
{
    if (led_strip == NULL) {
        ESP_LOGE(TAG, "Sterownik LED nie został zainicjalizowany!");
        return ESP_ERR_INVALID_STATE;
    }
    // Ustawienie koloru dla pierwszej diody (indeks 0)
    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, red, green, blue));
    // Odświeżenie stanu diody
    return led_strip_refresh(led_strip);
}

esp_err_t led_off(void)
{
    return led_set_color(0, 0, 0);
}
