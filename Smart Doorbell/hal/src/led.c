#define _GNU_SOURCE
#include "hal/led.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
//directory of green and red leds
static const char *LED_GREEN_PATH = "/sys/class/leds/ACT/brightness";
static const char *LED_RED_PATH   = "/sys/class/leds/PWR/brightness";

static void write_led(const char *path, int value)
{
    FILE *f = fopen(path, "w");
    if (!f) {
        
        perror("write_led: fopen");
        return;
    }
    fprintf(f, "%d", value);
    fclose(f);
}

void hal_led_init(void)
{
    hal_led_all_off();
}

void hal_led_cleanup(void)
{
    hal_led_all_off();
}

//on or off echo 1 or 0
void hal_led_green_on(void)  { write_led(LED_GREEN_PATH, 1); }
void hal_led_green_off(void) { write_led(LED_GREEN_PATH, 0); }

void hal_led_red_on(void)    { write_led(LED_RED_PATH, 1); }
void hal_led_red_off(void)   { write_led(LED_RED_PATH, 0); }

void hal_led_all_off(void)
{
    write_led(LED_GREEN_PATH, 0);
    write_led(LED_RED_PATH, 0);
}

//flashing n time for when incorrect of correct 
void hal_led_flash_green_n_times(int n, long total_ms)
{
    if (n <= 0 || total_ms <= 0) return;
    long period_ms = total_ms / n;           
    long half_ms = period_ms / 2;            
    for (int i = 0; i < n; ++i) {
        hal_led_green_on();
        struct timespec req = { .tv_sec = half_ms / 1000, .tv_nsec = (half_ms % 1000) * 1000000 };
        nanosleep(&req, NULL);
        hal_led_green_off();
        req.tv_sec = (period_ms - half_ms) / 1000;
        req.tv_nsec = ((period_ms - half_ms) % 1000) * 1000000;
        nanosleep(&req, NULL);
    }
}

void hal_led_flash_red_n_times(int n, long total_ms)
{
    if (n <= 0 || total_ms <= 0) return;
    long period_ms = total_ms / n;
    long half_ms = period_ms / 2;
    for (int i = 0; i < n; ++i) {
        hal_led_red_on();
        struct timespec req = { .tv_sec = half_ms / 1000, .tv_nsec = (half_ms % 1000) * 1000000 };
        nanosleep(&req, NULL);
        hal_led_red_off();
        req.tv_sec = (period_ms - half_ms) / 1000;
        req.tv_nsec = ((period_ms - half_ms) % 1000) * 1000000;
        nanosleep(&req, NULL);
    }
}

