#ifndef ACCELEROMETER_H_
#define ACCELEROMETER_H_

// Initialize accelerometer logic (no hardware init needed, relies on ADC)
void Accel_init(void);

// Cleanup (currently empty)
void Accel_cleanup(void);

// Read raw accelerometer values (0-4095)
void Accel_readXYZ(int* x, int* y, int* z);

#endif