#ifndef JOYSTICK_H_
#define JOYSTICK_H_

typedef enum {
    JS_NONE,
    JS_UP,
    JS_DOWN,
    JS_LEFT,
    JS_RIGHT,
    JS_CENTER
} JoystickDirection;

// Initialize ADC/Joystick
void Joystick_init(void);

// Read which direction is pressed
JoystickDirection Joystick_getDirection(void);

// Cleanup resources
void Joystick_cleanup(void);

#endif
