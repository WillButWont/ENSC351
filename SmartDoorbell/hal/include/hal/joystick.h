#ifndef HAL_JOYSTICK_H
#define HAL_JOYSTICK_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    JOY_NONE = 0,
    JOY_UP,
    JOY_DOWN,
    JOY_LEFT,
    JOY_RIGHT,
    JOY_CENTER
} joystick_dir_t;

// Initialize joystick (SPI for directions, libgpiod for button)
// Returns 0 on success, -1 on failure
int hal_joystick_init(const char *spi_device, uint32_t spi_speed_hz);

void hal_joystick_cleanup(void);

// Read direction of joystick stick
joystick_dir_t hal_joystick_read_direction(void);

// Wait until joystick stick is released to center
void hal_joystick_wait_until_released(void);

// Read raw ADC values (helper)
int hal_joystick_read_raw(int *x_out, int *y_out);

// Check if the joystick button (SEL) is pressed
// Returns true (1) if pressed, false (0) otherwise
bool hal_joystick_is_pressed(void);

#endif