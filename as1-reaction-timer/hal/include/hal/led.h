#ifndef LED_H
#define LED_H

typedef enum {
    LED_UP,
    LED_DOWN,
} LedDirection;

void LED_init(void);
void LED_turnOn(LedDirection dir);
void LED_allOff(void);
void LED_cleanup(void);

#endif
