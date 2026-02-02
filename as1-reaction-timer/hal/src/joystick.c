#include "hal/joystick.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <math.h>

static int spi_fd = -1;
static const char *spi_device = "/dev/spidev0.0";
static uint32_t speed_hz = 250000;

static int read_ch(int ch)
{
    uint8_t tx[3] = {
        (uint8_t)(0x06 | ((ch & 0x04) >> 2)),
        (uint8_t)((ch & 0x03) << 6),
        0x00
    };
    uint8_t rx[3] = {0};

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = speed_hz,
        .bits_per_word = 8,
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 1)
        return -1;

    return ((rx[1] & 0x0F) << 8) | rx[2]; // 12-bit result (10-bit effective)
}

void Joystick_init(void)
{
    spi_fd = open(spi_device, O_RDWR);
    if (spi_fd < 0) {
        perror("Failed to open SPI");
        return;
    }
    printf("Joystick SPI initialized.\n");
}

JoystickDirection Joystick_getDirection(void)
{
    if (spi_fd < 0)
        return JS_NONE;

    int x = read_ch(0);  // VRx
    int y = read_ch(1);  // VRy

    if (x < 0 || y < 0) return JS_NONE;

    // Calibrate thresholds (temporary)
    const int pos = 2048;
    const int tol = 300;

    if (x > pos + tol) return JS_RIGHT;
    if (x < pos - tol) return JS_LEFT;
    if (y > pos + tol) return JS_UP;
    if (y < pos - tol) return JS_DOWN;

    return JS_NONE;
}


void Joystick_cleanup(void)
{
    if (spi_fd >= 0) close(spi_fd);
    spi_fd = -1;
}
