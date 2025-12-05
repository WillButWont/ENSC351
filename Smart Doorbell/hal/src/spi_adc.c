#define _GNU_SOURCE
#include "hal/spi_adc.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

int hal_spi_adc_open(const char *device, uint32_t speed_hz)
{
    int fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("hal_spi_adc_open: open");
        return -1;
    }

    uint8_t mode = 0;
    uint8_t bits = 8;
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) {
        perror("SPI: set mode");
        close(fd);
        return -1;
    }
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
        perror("SPI: set bits");
        close(fd);
        return -1;
    }
    if (speed_hz > 0) {
        if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) == -1) {
            perror("SPI: set speed");
            close(fd);
            return -1;
        }
    }
    return fd;
}

void hal_spi_adc_close(int fd)
{
    if (fd >= 0) close(fd);
}


//channel 0 is x and channel 1 is y
int hal_spi_adc_read_ch(int fd, int ch, uint32_t speed_hz)
{
    if (fd < 0 || ch < 0 || ch > 7) return -1;

    uint8_t tx[3];
    tx[0] = (uint8_t)(0x06 | ((ch & 0x04) >> 2));
    tx[1] = (uint8_t)((ch & 0x03) << 6);
    tx[2] = 0x00;

    uint8_t rx[3] = {0,0,0};

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .delay_usecs = 0,
        .speed_hz = speed_hz,
        .bits_per_word = 8,
        .cs_change = 0
    };

    int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1) {
        perror("hal_spi_adc_read_ch: SPI_IOC_MESSAGE");
        return -1;
    }

    int val = ((rx[1] & 0x0F) << 8) | rx[2]; 
    return val;
}

