#ifndef HAL_SPI_ADC_H
#define HAL_SPI_ADC_H

#include <stdint.h>

//open SPI device
int hal_spi_adc_open(const char *device, uint32_t speed_hz);


void hal_spi_adc_close(int fd);

//read adc channels
int hal_spi_adc_read_ch(int fd, int ch, uint32_t speed_hz);

#endif 
