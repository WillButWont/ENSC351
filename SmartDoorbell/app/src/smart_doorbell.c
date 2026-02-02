#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <math.h> 
#include <string.h> 

#include "hal/led.h"
#include "hal/joystick.h"
#include "hal/accelerometer.h" 
#include "hal/uart.h" 
#include "sound.h"
#include "camera.h"
#include "udp_client.h"

// --- CONFIG ---
#define ESP32_IP "192.168.4.1" 
#define PIN_LENGTH 4
static const joystick_dir_t SECRET_PIN[PIN_LENGTH] = { JOY_LEFT, JOY_LEFT, JOY_UP, JOY_DOWN };

#define TAMPER_THRESHOLD 1000

// --- RFID CONFIG ---
#define UART_DEVICE "/dev/ttyAMA0" 
#define RFID_SECRET_KEY "5A5992"

long long current_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000LL);
}

// Helper to handle unlocking logic (shared by PIN and RFID)
void perform_unlock(const char* method) {
    printf("[ACCESS] UNLOCKING DOOR via %s\n", method);
    sound_play_correct(); 
    
    char udp_msg[64];
    snprintf(udp_msg, sizeof(udp_msg), "Door Unlocked by %s", method);
    udp_send(udp_msg);
    
    // Visual feedback: Green LED on
    hal_led_red_off(); 
    hal_led_green_on();
    sleep(3); 
    hal_led_green_off(); 
    hal_led_red_on();
}

int main(void) {
    // 1. Initialize HAL and Modules
    hal_led_init();
    camera_init();
    udp_init();
    sound_init(); 
    Accel_init(); 

    if (hal_joystick_init("/dev/spidev0.0", 250000) != 0) {
        printf("Joystick Init Failed! (Continuing anyway...)\n");
    }

    // Initialize UART for RFID
    if (hal_uart_init(UART_DEVICE, 9600) != 0) {
        printf("UART Init Failed! RFID will not work. Check %s permissions/existence.\n", UART_DEVICE);
    }

    // 2. Variables
    joystick_dir_t input_buffer[PIN_LENGTH];
    int input_count = 0;
    long long last_motion_check = 0;
    bool button_was_pressed = false;

    // Buffer for RFID data
    char rfid_buffer[64];

    // Accelerometer baseline
    int last_x = 0, last_y = 0, last_z = 0;
    Accel_readXYZ(&last_x, &last_y, &last_z); 

    printf("=== BEAGLEY-AI SMART DOORBELL STARTED ===\n");
    hal_led_red_on(); // Default locked state

    // 3. Main Loop
    while (1) {
        // --- A. DOORBELL BUTTON ---
        bool button_is_pressed = hal_joystick_is_pressed();
        
        if (button_is_pressed && !button_was_pressed) {
            printf("[DOORBELL] Button Pressed! Ding Dong!\n");
            sound_play_doorbell(); 
            udp_send("Doorbell Button Pressed");
        }
        button_was_pressed = button_is_pressed;

        // --- B. PIN CODE LOGIC ---
        joystick_dir_t dir = hal_joystick_read_direction();
        
        if (dir != JOY_NONE && dir != JOY_CENTER) {
            printf("[INPUT] Direction: %d\n", dir);
            
            input_buffer[input_count++] = dir;
            
            // Visual feedback
            hal_led_red_off(); hal_led_green_on();
            usleep(100000); 
            hal_led_green_off(); hal_led_red_on();

            hal_joystick_wait_until_released();

            if (input_count >= PIN_LENGTH) {
                bool correct = true;
                for(int i=0; i<PIN_LENGTH; i++) {
                    if(input_buffer[i] != SECRET_PIN[i]) correct = false;
                }

                if (correct) {
                    perform_unlock("PIN");
                } else {
                    printf("[ACCESS] DENIED (Wrong PIN)\n");
                    sound_play_incorrect(); 
                    hal_led_flash_red_n_times(3, 500);
                }
                input_count = 0; 
            }
        }

        // --- C. RFID UART LOGIC ---
        int bytes_read = hal_uart_read(rfid_buffer, sizeof(rfid_buffer) - 1);
        if (bytes_read > 0) {
            rfid_buffer[bytes_read] = '\0'; // Null-terminate
            
            // Remove newline characters (\r or \n) sent by Flipper
            rfid_buffer[strcspn(rfid_buffer, "\r\n")] = 0;

            if (strlen(rfid_buffer) > 0) {

                if (strcmp(rfid_buffer, RFID_SECRET_KEY) == 0) {
                    perform_unlock("RFID");
                } else {
                    printf("[ACCESS] DENIED (Unknown Tag)\n");
                    sound_play_incorrect();
                    hal_led_flash_red_n_times(2, 200);
                }
            }
        }

        // --- D. TAMPER DETECTION ---
        int x, y, z;
        Accel_readXYZ(&x, &y, &z);
        int delta = abs(x - last_x) + abs(y - last_y) + abs(z - last_z);
        
        if (delta > TAMPER_THRESHOLD) {
            printf("[ALARM] TAMPER DETECTED! Delta: %d\n", delta);
            sound_play_alarm();
            udp_send("TAMPER DETECTED: Device Shaken!");
            
            for(int i=0; i<5; i++) {
                hal_led_red_on(); usleep(50000);
                hal_led_red_off(); usleep(50000);
            }
            hal_led_red_on();
            
            sleep(2); 
            Accel_readXYZ(&last_x, &last_y, &last_z);
        } else {
            last_x = x; last_y = y; last_z = z;
        }

        // --- E. MOTION LOGIC ---
        long long now = current_ms();
        if (now - last_motion_check > 200) {
            // Only check motion if user isn't busy entering a PIN
            if (input_count == 0) {
                if (camera_capture(ESP32_IP) == 0) {
                    if (camera_check_motion()) {
                        printf("[MOTION] Movement detected!\n");
                        udp_send("Motion Detected at Front Door");
                        sleep(5); 
                    }
                }
            }
            last_motion_check = now;
        }

        usleep(10000); 
    }

    // 4. Cleanup
    sound_cleanup();
    Accel_cleanup();
    hal_joystick_cleanup();
    hal_led_cleanup();
    hal_uart_cleanup(); 
    udp_cleanup();
    return 0;
}