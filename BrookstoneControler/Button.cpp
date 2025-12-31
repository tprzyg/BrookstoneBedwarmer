#include "Button.h"

Button::Button(byte pin, void (*action)(), unsigned long delay) {
  this->pin = pin;
  lastReading = LOW;
  debounceDelay = delay;
  action_routine = action;
  init();
}

void Button::init() {
  pinMode(pin, INPUT_PULLUP);
  update();
}

void IRAM_ATTR Button::update_ISR() {
  if (millis() - lastDebounceTime > debounceDelay) {
    uint8_t newReading = digitalRead(pin);
    if (newReading != lastReading) {
      lastDebounceTime = millis();
      state = HIGH;
      lastReading = HIGH;
      pressType = BUTTON_SHORT_PRESS;
    }
  }
}

void Button::update() {
  if (millis() - lastDebounceTime > debounceDelay) {
    uint8_t newReading = digitalRead(pin);
    if (newReading != lastReading) {
      lastDebounceTime = millis();
      state = newReading;
      lastReading = newReading;
      if (state == LOW) {
        pressType = BUTTON_NO_PRESS;
      }
    }
    else if (state == HIGH) {
      if (millis() - lastDebounceTime > 15 * debounceDelay) {
        pressType = BUTTON_VERY_LONG_PRESS;
      } else if (millis() - lastDebounceTime > 5 * debounceDelay) {
        pressType = BUTTON_LONG_PRESS;
      } else {
        pressType = BUTTON_SHORT_PRESS;
      }
    }
  }
}

uint8_t Button::getState() {
  update();
  return state;
}

bool Button::isPressed() {
  return (getState() == HIGH);
}

bool Button::isShortPress() {
  return (getState() == HIGH) && (pressType == BUTTON_SHORT_PRESS);
}

bool Button::isLongPress() {
  return (getState() == HIGH) && (pressType == BUTTON_LONG_PRESS);
}

bool Button::isVeryLongPress() {
  return (getState() == HIGH) && (pressType == BUTTON_VERY_LONG_PRESS);
}
bool Button::isReleased() {
  return (getState() == LOW);
}