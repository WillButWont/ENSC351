#ifndef HAL_LED_H
#define HAL_LED_H

#include <stdbool.h>

void hal_led_init(void);
void hal_led_cleanup(void);

//leds on or off
void hal_led_green_on(void);
void hal_led_green_off(void);

void hal_led_red_on(void);
void hal_led_red_off(void);

void hal_led_all_off(void);

//flashing for correct of incorrect answer
void hal_led_flash_green_n_times(int n, long total_ms);
void hal_led_flash_red_n_times(int n, long total_ms);

#endif 

