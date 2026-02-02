#include "hal/led.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define GREEN_PATH "/sys/class/leds/ACT/brightness"
#define RED_PATH   "/sys/class/leds/PWR/brightness"

static void write_led(const char *path, int value)
{
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "%d", value);
        fclose(f);
    }
}

void LED_init(void)
{
    LED_allOff();
}

void LED_turnOn(LedDirection dir)
{
    if (dir == LED_UP)
        write_led(GREEN_PATH, 1);
    else if (dir == LED_DOWN)
        write_led(RED_PATH, 1);
}

void LED_allOff(void)
{
    write_led(GREEN_PATH, 0);
    write_led(RED_PATH, 0);
}

void LED_cleanup(void)
{
    LED_allOff();
}
