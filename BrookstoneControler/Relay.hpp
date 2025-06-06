#include "Relay.h"

Relay::Relay(byte pin) {
  this->pin = pin;
  init();
}

void Relay::init() {
  pinMode(pin, OUTPUT);
  off();
}

void Relay::on() {
  digitalWrite(pin, HIGH);
  status = getStatus();
}

void Relay::off() {
  digitalWrite(pin, LOW);
  status = getStatus();
}

void Relay::toggle() {
  status = getStatus();
  digitalWrite(pin, !status);
  status = getStatus();
}

uint8_t Relay::getStatus() {
  status = digitalRead(pin);
  return status;
}

bool Relay::isOn() {
  return (getState() == HIGH);
}

bool Relay::isOff() {
  return (getState() == LOW);
}