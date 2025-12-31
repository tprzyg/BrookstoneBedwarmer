/*
 * Brookstone Bedwarmer replacement controller.
 * (C)2022 Tomasz Przygoda <tprzyg@yahoo.com>
 *
 * 4 input buttons (reused from the original controller board) for temp+, temp-, timer+ and timer-
 * 1 Power on/off button (also reused from the original board) with display ON it will set the ESP32 into deep sleep
 * The original segmented LCD display was replaced with 0.96" OLED (SSD1306)
 * Temperature is adjusted from 10% to 100% with 10% increments
 * Timer can be adjusted from 1min to 12hrs (originally was from 30min to 8hrs)
 * Logic was changed to allow for 1min timer-step value (originally 30min step value)
 * and also the display will show not the preset timer value, but rather the remaining hours:minutes
 * Display will dim after 15 seconds and go blank after 60 seconds of inactivity
 * When display is inactive, pressing any button (including power button) will reactivate the display
 * When display is active pressing adjustment buttons will make the desired adjustment
 * while pressing power button will turn off heat, reset timer and put the controller into deep sleep
 * When controller is in deep sleep mode, pressing power button will restart and re-initialize the controller,
 * but all other buttons are ignored
 * When timer runs out, the controller will turn off heat, reset timer and put the controller into deep sleep
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

#include "BrookstoneControler.h"

// Allocate the SSD1306 OLED Screen
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

bool wifiOn;
bool timerOn;
bool heaterOn;
bool displayOn;
bool refreshDisplay;
bool enterSleepMode;

unsigned int heatSetting;
unsigned int lastButtonPressed;

unsigned long currentTime;
unsigned long timerEndTime = 0;
unsigned long timerStartTime = 0;
unsigned long timerRemaining = 0;

unsigned long lastDisplayOn = 0;
unsigned long lastDisplayUpdateTime = 0;

unsigned long currentPressTime = 0;
unsigned long lastButtonPressTime = 0;
unsigned long lastButtonActionTime = 0;

unsigned long lastHeatOnTime = 0;
unsigned long lastHeatOffTime = 0;

typedef void (*dummyFunction)(void);

void updateHeatSetting(unsigned int myFactor) {
  currentPressTime = millis();
  refreshDisplay = true;
  unsigned int newHeatSetting = heatSetting + TEMP_STEP * myFactor;

  if ((newHeatSetting <= MAX_TEMP) and (newHeatSetting >= MIN_TEMP)) {
    heatSetting = newHeatSetting;
    lastButtonActionTime = currentPressTime;
  }
}

void updateTimerSetting(unsigned int myFactor) {
  currentPressTime = millis();
  refreshDisplay = true;
  unsigned long newTimerEndTime = timerEndTime + (TIMER_STEP * myFactor) * 60000;

  if (((newTimerEndTime - currentTime )<= MAX_TIMER * 60000) and ((newTimerEndTime - currentTime) >= MIN_TIMER * 60000)) {
    timerEndTime = newTimerEndTime;
    lastButtonActionTime = currentPressTime;
  }
}

void IRAM_ATTR temp_up_ISR() {
  currentPressTime = millis();
  if (currentPressTime - lastButtonPressTime > MIN_BUTTON_PRESS_TIME) {
    lastButtonPressTime = currentPressTime;
    if (displayOn) {
      lastButtonPressed = TEMP_PLUS_PIN;
    } else {
      displayOn = true;
      lastDisplayOn = currentPressTime;
    }
  }
}

void IRAM_ATTR temp_down_ISR() {
  currentPressTime = millis();
  if (currentPressTime - lastButtonPressTime > MIN_BUTTON_PRESS_TIME) {
    lastButtonPressTime = currentPressTime;
    if (displayOn) {
      lastButtonPressed = TEMP_MINUS_PIN;
    } else {
      displayOn = true;
      lastDisplayOn = currentPressTime;
    }
  }
}

void IRAM_ATTR timer_up_ISR() {
  currentPressTime = millis();
  if (currentPressTime - lastButtonPressTime > MIN_BUTTON_PRESS_TIME) {
    lastButtonPressTime = currentPressTime;
    if (displayOn) {
      lastButtonPressed = TIMER_PLUS_PIN;
    } else {
      displayOn = true;
      lastDisplayOn = currentPressTime;
    }
  }
}

void IRAM_ATTR timer_down_ISR() {
  currentPressTime = millis();
  if (currentPressTime - lastButtonPressTime > MIN_BUTTON_PRESS_TIME) {
    lastButtonPressTime = currentPressTime;
    if (displayOn) {
      lastButtonPressed = TIMER_MINUS_PIN;
    } else {
      displayOn = true;
      lastDisplayOn = currentPressTime;
    }
  }
}

void IRAM_ATTR power_button_ISR() {
  currentPressTime = millis();
  if (currentPressTime - lastButtonPressTime > MIN_BUTTON_PRESS_TIME) {
    refreshDisplay = true;
    lastButtonPressTime = currentPressTime;
    if (displayOn) {
      lastButtonPressed = POWER_ON_PIN;
      enterSleepMode = true;
    } else {
      displayOn = true;
      lastDisplayOn = currentPressTime;
    }
  }
}

void checkPowerOff() {
  currentTime = millis();
  if (enterSleepMode) {
    Serial.println("Preparing for DEEP_SLEEP mode...");
    if (digitalRead(HEATER_RELAY_PIN) == HIGH) {
      Serial.print("Turning Heater OFF @ ");
      Serial.println(currentTime);
      digitalWrite(HEATER_RELAY_PIN, LOW);
    }
    Serial.println("Turning timer OFF");
    heatSetting = 0;
    timerEndTime = 0;
    timerStartTime = 0;
    timerRemaining = 0;
    Serial.println("Turning display OFF");
    display.clearDisplay();
    display.display();
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    Serial.print("Setting up RTC_WKAEUP on ");
    Serial.print(POWER_ON_PIN);
    Serial.println(" to become a wakeup button");
    esp_sleep_enable_ext0_wakeup(POWER_ON_PIN, 0);
    Serial.println("Entering DEEP_SLEEP mode.");
    delay(1000);
    esp_deep_sleep_start();
  }
}

void updateDisplay() {
  currentTime = millis();
  if (displayOn) {
    if (currentTime - lastDisplayUpdateTime >= DISPLAY_REFRESH_RATE) {
      refreshDisplay = true;
    }
  }
  if (refreshDisplay) {
    lastDisplayUpdateTime = currentTime;
    int timerHours = timerRemaining / 60;
    int timerMinutes = timerRemaining - timerHours * 60;
    char heatStr[8];
    sprintf(heatStr, "%3d%%", heatSetting);
    char timeStr[8];
    sprintf(timeStr, "%2d:%02d", timerHours, timerMinutes);
    if (currentTime - lastDisplayOn > DISPLAY_DIMMING) {
      displayOn = true;
      // PUT DISPLAY DIMMING CODE HERE
    }
    if (currentTime - lastDisplayOn > DISPLAY_TIMEOUT) {
      displayOn = false;
      display.ssd1306_command(SSD1306_DISPLAYOFF);
    }
    if (displayOn) {
      // MAKE SURE DISPLAY IS ON
      display.ssd1306_command(SSD1306_DISPLAYON);
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setFont(&FreeSans9pt7b);
      display.setTextSize(1);
      display.setCursor(0, 24);
      display.println("Heat:");
      display.setFont(&FreeSansBold12pt7b);
      display.setTextSize(1);
      display.println(heatStr);
      display.setFont(&FreeSans9pt7b);
      display.setTextSize(1);
      display.setCursor(0, 80);
      display.println("Timer:");
      display.setFont(&FreeSansBold12pt7b);
      display.setTextSize(1);
      display.println(timeStr);
      display.display();
    }
    refreshDisplay = false;
    if (DEBUG_LEVEL > 0) {
      Serial.print("LastDisplayOn = ");
      Serial.print((currentTime-lastDisplayOn)/1000);
      Serial.print("s ago, Heat = ");
      Serial.print(heatStr);
      Serial.print(", Timer = ");
      Serial.println(timeStr);
    }
  }
}

void checkTimerOff() {
  currentTime = millis();
  long timerRemainingMillis = timerEndTime - currentTime;
  if (timerRemainingMillis > 60000) {
    timerRemaining = round(timerRemainingMillis / 60000);
  } else {
    timerRemaining = 0;
    timerOn = false;
  }
  if (!timerOn) {
    timerEndTime = 0;
    timerStartTime = 0;
    timerRemaining = 0;
    timerOn = false;
    heaterOn = false;
    enterSleepMode = true;
  }
}

void runHeater() {
  currentTime = millis();
  if (timerOn) {
    unsigned long dutyOnTime = round(HEATER_TIME_CYCLE * (heatSetting * 0.01 * (MAX_DUTY_CYCLE - MIN_DUTY_CYCLE) + MIN_DUTY_CYCLE));
    unsigned long dutyOffTime = HEATER_TIME_CYCLE - dutyOnTime;
    if ((heatSetting >= MIN_TEMP) and (heatSetting <= MAX_TEMP)) {
      if (!heaterOn) {
        if (currentTime - lastHeatOffTime >= dutyOffTime) {
          // TURN THE HEAT ON
          digitalWrite(HEATER_RELAY_PIN, HIGH);
          if (DEBUG_LEVEL > 0) {
            Serial.print("Heater ON after ");
            Serial.println(currentTime - lastHeatOffTime);
          }
          heaterOn = true;
          lastHeatOnTime = currentTime;
        }
      } else {
        if (currentTime - lastHeatOnTime >= dutyOnTime) {
          // TURN THE HEAT OFF
          digitalWrite(HEATER_RELAY_PIN, LOW);
          if (DEBUG_LEVEL > 0) {
            Serial.print("Heater OFF after ");
            Serial.println(currentTime - lastHeatOnTime);
          }
          heaterOn = false;
          lastHeatOffTime = currentTime;
        }
      }
    } else {
      Serial.println("Invalid heat setting... ignoring...");
      // MAKE SURE THAT THE HEAT IS OFF
      if (digitalRead(HEATER_RELAY_PIN) == HIGH) {
        digitalWrite(HEATER_RELAY_PIN, LOW);
      }
    }
  }
}

void checkButtons() {
  currentTime = millis();
  bool buttonAction = false;
  unsigned int actionFactor = 1;
  if (digitalRead(lastButtonPressed) == LOW) {
    if (currentTime - lastButtonPressTime > 10 * MIN_BUTTON_PRESS_TIME) {
      buttonAction = true;
      actionFactor = 10;
    } else if (currentTime - lastButtonPressTime > 5 * MIN_BUTTON_PRESS_TIME) {
      buttonAction = true;
      actionFactor = 5;
    } else if (currentTime - lastButtonActionTime > MIN_BUTTON_PRESS_TIME) {
      buttonAction = true;
    }
  }
  if (currentTime - lastButtonActionTime < round(2*MIN_BUTTON_PRESS_TIME / actionFactor)) {
    buttonAction = false;
  }
  if (buttonAction) {
    lastButtonActionTime = currentTime;
    switch (lastButtonPressed) {
      case TEMP_PLUS_PIN:
       updateHeatSetting(1);
       break;
      case TEMP_MINUS_PIN:
       updateHeatSetting(-1);
       break;
      case TIMER_PLUS_PIN:
       updateTimerSetting(1);
       break;
      case TIMER_MINUS_PIN:
       updateTimerSetting(-1);
       break;
    }
  }
}

void setup_pin(unsigned int PIN, dummyFunction ISR_routine) {
  pinMode(PIN, INPUT_PULLUP);
  attachInterrupt(PIN, ISR_routine, FALLING);
}

void setup() {
  // Setup Serial Monitor
  Serial.begin(115200);
  // Initialize the SSD1306 OLED Screen
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_I2C)) {
    Serial.println(F("SSD1306 allocation failed"));
    delay(10000);
    ESP.restart();
  }
  delay(2000);
  display.ssd1306_command(SSD1306_DISPLAYON);
  display.clearDisplay();
  display.setRotation(1);
  display.setTextColor(WHITE);
  display.setFont(&FreeSans9pt7b);
  display.setTextSize(1);
  display.clearDisplay();
  display.display();
  Serial.println("ESP32 Controller for a Brookstone Heated Mattress Pad.");
  timerOn = true;
  heaterOn = false;
  displayOn = true;
  refreshDisplay = true;
  enterSleepMode = false;
  heatSetting = DEFAULT_TEMP;
  lastDisplayOn = currentTime;
  timerStartTime = currentTime;
  timerRemaining = DEFAULT_TIMER;
  timerEndTime = timerStartTime + DEFAULT_TIMER * 60000;
  lastButtonPressTime = currentTime;
  setup_pin(POWER_ON_PIN, power_button_ISR);
  setup_pin(TEMP_PLUS_PIN, temp_up_ISR);
  setup_pin(TEMP_MINUS_PIN, temp_down_ISR);
  setup_pin(TIMER_PLUS_PIN, timer_up_ISR);
  setup_pin(TIMER_MINUS_PIN, timer_down_ISR);
  pinMode(HEATER_RELAY_PIN, OUTPUT);
  display.println("ON");
  display.display();
  updateDisplay();
}

void loop() {
  checkButtons();
  checkTimerOff();
  checkPowerOff();
  updateDisplay();
  runHeater();
}
