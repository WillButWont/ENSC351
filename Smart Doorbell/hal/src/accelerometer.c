#include "hal/accelerometer.h"
#include "hal/spi_adc.h"
#include <stdlib.h>
#include <stdio.h>

// --- CONFIGURATION ---
// CHECK THIS: Ensure this matches your BeagleY-AI SPI setup
#define ADC_SPI_DEV     "/dev/spidev0.0" 
#define ADC_SPI_SPEED   1000000 // 1 MHz

// --- WIRING MAPPING ---
// Based on your provided mapping:
// Accel X -> Channel 2
// Accel Y -> Channel 1
// Accel Z -> Channel 0
#define ACC_X_CH  2
#define ACC_Y_CH  1
#define ACC_Z_CH  0

static int spi_fd = -1;

void Accel_init(void) {
    spi_fd = hal_spi_adc_open(ADC_SPI_DEV, ADC_SPI_SPEED);
    if (spi_fd < 0) {
        printf("Accel: Failed to open SPI device %s\n", ADC_SPI_DEV);
    }
}

void Accel_readXYZ(int* x, int* y, int* z) {
    if (spi_fd < 0) return;

    // Read values using the SPI ADC driver
    if (x) *x = hal_spi_adc_read_ch(spi_fd, ACC_X_CH, ADC_SPI_SPEED);
    if (y) *y = hal_spi_adc_read_ch(spi_fd, ACC_Y_CH, ADC_SPI_SPEED);
    if (z) *z = hal_spi_adc_read_ch(spi_fd, ACC_Z_CH, ADC_SPI_SPEED);
}

void Accel_cleanup(void) {
    if (spi_fd >= 0) {
        hal_spi_adc_close(spi_fd);
        spi_fd = -1;
    }
}