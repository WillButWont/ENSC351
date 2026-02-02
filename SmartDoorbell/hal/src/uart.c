#include "hal/uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

static int uart_fd = -1;

int hal_uart_init(const char* device, int baud_rate) {
    // Open in Read/Write, No Controlling TTY, Non-Blocking mode
    uart_fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart_fd == -1) {
        perror("[HAL UART] Unable to open UART");
        return -1;
    }

    struct termios options;
    tcgetattr(uart_fd, &options);

    // Set Baud Rate (supports common rates)
    speed_t baud;
    switch(baud_rate) {
        case 9600:   baud = B9600; break;
        case 19200:  baud = B19200; break;
        case 38400:  baud = B38400; break;
        case 57600:  baud = B57600; break;
        case 115200: baud = B115200; break;
        default:     baud = B9600; printf("[HAL UART] Warning: Defaulting to 9600\n"); break;
    }
    
    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);

    // --- Configure 8N1 (8 bits, No parity, 1 stop bit) ---
    options.c_cflag &= ~PARENB; // No parity
    options.c_cflag &= ~CSTOPB; // 1 stop bit
    options.c_cflag &= ~CSIZE;  // Mask character size bits
    options.c_cflag |= CS8;     // 8 data bits

    // --- Control Options ---
    options.c_cflag |= (CLOCAL | CREAD); // Enable receiver, ignore modem control lines

    // --- Input Options ---
    // Disable software flow control (XON/XOFF) and other processing
    options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
    
    // --- Output Options ---
    // Raw output
    options.c_oflag &= ~OPOST;

    // --- Local Options ---
    // Canonical mode off (raw input), no echo
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // Apply settings
    tcsetattr(uart_fd, TCSANOW, &options);
    
    // Ensure non-blocking is set explicitly
    fcntl(uart_fd, F_SETFL, FNDELAY);

    printf("[HAL UART] Initialized %s at %d baud\n", device, baud_rate);
    return 0;
}

int hal_uart_read(char* buffer, int max_len) {
    if (uart_fd == -1) return -1;
    
    int bytes_read = read(uart_fd, buffer, max_len);
    if (bytes_read < 0) {
        // EAGAIN means no data available right now (normal for non-blocking)
        if (errno == EAGAIN) return 0; 
        perror("[HAL UART] Read error");
        return -1;
    }
    return bytes_read;
}

void hal_uart_write(const char* buffer) {
    if (uart_fd == -1) return;
    int len = strlen(buffer);
    int written = write(uart_fd, buffer, len);
    if (written < 0) {
        perror("[HAL UART] Write error");
    }
}

void hal_uart_cleanup(void) {
    if (uart_fd != -1) {
        close(uart_fd);
        uart_fd = -1;
    }
}