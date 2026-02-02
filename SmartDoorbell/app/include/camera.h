#ifndef CAMERA_H
#define CAMERA_H
#include <stdbool.h>

void camera_init(void);
// Download image from ESP32
int camera_capture(const char* ip_address);
// Check if downloaded image has motion
bool camera_check_motion(void);
void camera_cleanup(void);

#endif