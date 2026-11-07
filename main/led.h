#pragma once

#include "esp_err.h"
#include "led_strip.h"

// Domyślny numer pinu GPIO dla diody RGB
#define LED_GPIO_DEFAULT 48
// Maksymalna liczba diod w pasku (w tym przypadku 1)
#define LED_STRIP_LED_COUNT 1

/**
 * @brief Inicjalizuje sterownik diody LED.
 * 
 * @param gpio_num Numer pinu GPIO, do którego podłączona jest dioda.
 * @return esp_err_t ESP_OK w przypadku sukcesu.
 */
esp_err_t led_init(int gpio_num);

/**
 * @brief Ustawia kolor diody.
 * 
 * @param red Wartość składowej czerwonej (0-255).
 * @param green Wartość składowej zielonej (0-255).
 * @param blue Wartość składowej niebieskiej (0-255).
 * @return esp_err_t ESP_OK w przypadku sukcesu.
 */
esp_err_t led_set_color(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Wyłącza diodę (ustawia kolor na czarny).
 * 
 * @return esp_err_t ESP_OK w przypadku sukcesu.
 */
esp_err_t led_off(void);
