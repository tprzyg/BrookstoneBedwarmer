#ifndef OOB_RELAY_H
#define OOB_RELAY_H
#include <Arduino.h>

class Relay {
private:
  uint8_t pin;
  uint8_t status;

public:
  Relay(uint8_t pin);
  void init();
  void on();
  void off();
  void toggle();
  uint8_t getStatus();
  bool isOn();
  bool isOff();
};

#endif /* OOB_RELAY_H */
