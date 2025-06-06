#ifndef OOB_BUTTON_H
#define OOB_BUTTON_H
#include <Arduino.h>

#define BUTTON_NO_PRESS         0
#define BUTTON_SHORT_PRESS      1
#define BUTTON_LONG_PRESS       2
#define BUTTON_VERY_LONG_PRESS  3

class Button {
  
  private:
    uint8_t pin;
    uint8_t state;
    uint8_t pressType = BUTTON_NO_PRESS;
    uint8_t lastReading;
    unsigned long debounceDelay;
    unsigned long lastDebounceTime = 0;
    void (*action_routine)();
    void IRAM_ATTR update_ISR();
    void update();

  public:
    Button(uint8_t pin, void (*action)(), unsigned long delay = 200);
    void init();
    uint8_t getState();
    bool isPressed();
    bool isShortPress();
    bool isLongPress();
    bool isVeryLongPress();
    bool isReleased();
};
#endif /* OOB_BUTTON_H */
