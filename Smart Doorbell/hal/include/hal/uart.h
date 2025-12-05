#ifndef HAL_UART_H
#define HAL_UART_H

#include <stdbool.h>

// Initialize UART on the specified device at the given baud rate
// Returns 0 on success, -1 on failure
int hal_uart_init(const char* device, int baud_rate);

// Read data from UART into buffer (non-blocking). 
// Returns number of bytes read (0 if no data, -1 if error).
int hal_uart_read(char* buffer, int max_len);

// Write data to UART (blocking)
void hal_uart_write(const char* buffer);

// Cleanup UART resources
void hal_uart_cleanup(void);

#endif