#define _GNU_SOURCE
#include "hal/joystick.h"
#include "hal/spi_adc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <gpiod.h>

// --- GPIO CONFIGURATION FOR BUTTON ---
#define JOYSTICK_CHIP "/dev/gpiochip1"
#define JOYSTICK_LINE 41  

static int spi_fd = -1;
static uint32_t spi_speed = 250000;

// libgpiod handles
static struct gpiod_chip *gpio_chip = NULL;
static struct gpiod_line_request *gpio_req = NULL;

static int x_center = 2048;
static int y_center = 2048;

// Helper to get ms 
static long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

// Helper: Configure GPIO using libgpiod v2
static int configure_joystick_button(void) {
    // 1. Open Chip
    gpio_chip = gpiod_chip_open(JOYSTICK_CHIP);
    if (!gpio_chip) {
        perror("hal_joystick: gpiod_chip_open");
        return -1;
    }

    // 2. Configure Settings (Input, Pull-up, Debounce)
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    if (!settings) {
        perror("hal_joystick: gpiod_line_settings_new");
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
        return -1;
    }
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    
    // Joystick buttons usually connect to Ground, so we need a Pull-Up
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);
    
    // 3. Configure Request
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "joystick_button");
    
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    unsigned int offset = JOYSTICK_LINE;
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);

    // 4. Request the line
    gpio_req = gpiod_chip_request_lines(gpio_chip, req_cfg, line_cfg);
    
    // Clean up config objects
    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);

    if (!gpio_req) {
        perror("hal_joystick: gpiod_chip_request_lines");
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
        return -1;
    }

    return 0;
}

int hal_joystick_init(const char *spi_device, uint32_t spi_speed_hz)
{
    // 1. Initialize SPI for X/Y Axis
    spi_fd = hal_spi_adc_open(spi_device, spi_speed_hz);
    if (spi_fd < 0) return -1;
    spi_speed = spi_speed_hz;

    // 2. Initialize GPIO for Button (SEL)
    if (configure_joystick_button() != 0) {
        fprintf(stderr, "Failed to initialize Joystick Button on %s line %d\n", JOYSTICK_CHIP, JOYSTICK_LINE); 
    }

    // 3. Auto-calibration: joystick is at rest/center 
    int samples = 20;
    long long start = now_ms();
    long long timeout = start + 1000; 
    long sum_x = 0, sum_y = 0;
    int cnt = 0;
    for (int i = 0; i < samples && now_ms() < timeout; ++i) {
        int xv = hal_spi_adc_read_ch(spi_fd, 6, spi_speed);
        int yv = hal_spi_adc_read_ch(spi_fd, 7, spi_speed);
        if (xv >= 0 && yv >= 0) {
            sum_x += xv;
            sum_y += yv;
            ++cnt;
        }
        struct timespec req = {0, 20000000}; 
        nanosleep(&req, NULL);
    }
    if (cnt > 0) {
        x_center = (int)(sum_x / cnt);
        y_center = (int)(sum_y / cnt);
    } else {
        x_center = 2048; y_center = 2048;
    }

    return 0;
}

void hal_joystick_cleanup(void) 
{
    // Cleanup GPIO
    if (gpio_req) {
        gpiod_line_request_release(gpio_req);
        gpio_req = NULL;
    }
    if (gpio_chip) {
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
    }

    // Cleanup SPI
    if (spi_fd >= 0) {
        hal_spi_adc_close(spi_fd);
        spi_fd = -1;
    }
}

// Read raw ADCs
int hal_joystick_read_raw(int *x_out, int *y_out)
{
    if (spi_fd < 0) return -1;
    int xv = hal_spi_adc_read_ch(spi_fd, 6, spi_speed);
    int yv = hal_spi_adc_read_ch(spi_fd, 7, spi_speed);
    if (xv < 0 || yv < 0) return -1;
    if (x_out) *x_out = xv;
    if (y_out) *y_out = yv;
    return 0;
}

// Map raw to direction
joystick_dir_t hal_joystick_read_direction(void)
{
    int xv, yv;
    if (hal_joystick_read_raw(&xv, &yv) != 0) return JOY_NONE;

    // Threshold
    const int threshold = 1000; 

    int dx = xv - x_center;
    int dy = yv - y_center;

    int absdx = dx < 0 ? -dx : dx;
    int absdy = dy < 0 ? -dy : dy;

    if (absdy > threshold && absdy >= absdx) {
        if (dy > 0) return JOY_UP;     
        else return JOY_DOWN;          
    } else if (absdx > threshold && absdx > absdy) {
        if (dx > 0) return JOY_RIGHT;  
        else return JOY_LEFT;           
    }

    return JOY_NONE;
}

// Wait until joystick stick returns to center
void hal_joystick_wait_until_released(void)
{
    for (;;) {
        joystick_dir_t d = hal_joystick_read_direction();
        if (d == JOY_NONE) return;
        struct timespec req = {0, 50000000};
        nanosleep(&req, NULL);
    }
}

// Check if button is pressed
bool hal_joystick_is_pressed(void)
{
    if (!gpio_req) return false;

    // Read value from libgpiod
    int val = gpiod_line_request_get_value(gpio_req, JOYSTICK_LINE);

    if (val < 0) {
        // Error reading line
        return false;
    }

    // Active LOW: 0 means pressed, 1 means released
    return (val == 0);
}