/*
 * Brookstone Bedwarmer replacement controller.
 * (C)2022 Tomasz Przygpda <tprzyg@yahoo.com>
 *
 * 4 input buttons (reused from the original controller board) for temp+, temp-, timer+ and timer-
 * 1 Power on/off button (also reused from the original board) with display OFF will set the ESP32 to deep sleep
 * The original segmented LCD display was replaced with 0.96" OLED (SSD1306)
 * Temperature is adjusted from 10% to 100% with 10% increments
 * Timer can be adjusted from 10min to 12hrs (in 10min steps)
 * Logic was slightly changed to allow for 10min timer-step value (originally 30min step value)
 * and also the display will show not the preset timer value, but rather the remaining hours:minutes
 * Display will dim after 15 seconds and go blank after 60 seconds of inactivity
 * When display is inactive, pressing any button (including power button) will reactivate the display
 * When display is active pressing adjustment buttons will make the desired adjustment
 * while pressing power button will turn off heat, reset timer and put the controller into deep sleep
 * When controller is in deep sleep mode, pressing power button will restart and re-initialize the controller, but all other buttons are ignored
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

// Initialize Screen
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

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

unsigned long lastHeatOnTime = 0;
unsigned long lastHeatOffTime = 0;

typedef void (*ISR_fuction)(void);

void IRAM_ATTR temp_up_ISR() {
  currentPressTime = millis();
  if (currentPressTime - lastButtonPressTime > DEBOUNCING_TIME) {
    refreshDisplay = true;
    lastButtonPressed = TEMP_PLUS_PIN;
    lastButtonPressTime = currentPressTime;
    if (displayOn == true) {
      if (heatSetting < MAX_TEMP - TEMP_STEP) {
        heatSetting = heatSetting + TEMP_STEP;
      }
    } else {
      displayOn = true;
    }
    lastDisplayOn = currentPressTime;
  }
}

void IRAM_ATTR temp_down_ISR() {
  currentPressTime = millis();
  if (currentPressTime - lastButtonPressTime > DEBOUNCING_TIME) {
    refreshDisplay = true;
    lastButtonPressTime = currentPressTime;
    lastButtonPressed = TEMP_MINUS_PIN;
    lastButtonPressTime = currentPressTime;
    if (displayOn == true) {
      if (heatSetting > MIN_TEMP + TEMP_STEP) {
        heatSetting = heatSetting - TEMP_STEP;
      }
    } else {
      displayOn = true;
    }
    lastDisplayOn = currentPressTime;
  }
}

void IRAM_ATTR timer_up_ISR() {
  currentPressTime = millis();
  if (currentPressTime - lastButtonPressTime > DEBOUNCING_TIME) {
    refreshDisplay = true;
    lastButtonPressTime = currentPressTime;
    lastButtonPressed = TIMER_PLUS_PIN;
    lastButtonPressTime = currentPressTime;
    if (displayOn == true) {
      if (timerEndTime - currentTime < (MAX_TIMER - TIMER_STEP) * 60000) {
        timerEndTime = timerEndTime + TIMER_STEP * 60000;
      }
    } else {
      displayOn = true;
    }
    lastDisplayOn = currentPressTime;
  }
}

void IRAM_ATTR timer_down_ISR() {
  currentPressTime = millis();
  if (currentPressTime - lastButtonPressTime > DEBOUNCING_TIME) {
    refreshDisplay = true;
    lastButtonPressTime = currentPressTime;
    lastButtonPressed = TIMER_MINUS_PIN;
    lastButtonPressTime = currentPressTime;
    if (displayOn == true) {
      if (timerEndTime - currentTime > (TIMER_STEP + 1) * 60000) {
        timerEndTime = timerEndTime - TIMER_STEP * 60000;
      }
    } else {
      displayOn = true;
    }
    lastDisplayOn = currentPressTime;
  }
}

void IRAM_ATTR power_button_ISR() {
  currentPressTime = millis();
  if (currentPressTime - lastButtonPressTime > DEBOUNCING_TIME) {
    refreshDisplay = true;
    lastButtonPressTime = currentPressTime;
    lastButtonPressed = POWER_ON_PIN;
    if (displayOn == true) {
      enterSleepMode = true;
    } else {
      displayOn = true;
      lastDisplayOn = currentPressTime;
    }
  }
}

void checkPowerOff() {
  currentTime = millis();
  if (enterSleepMode == true) {
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
  if (displayOn == true) {
    if (currentTime - lastDisplayUpdateTime >= DISPLAY_REFRESH_RATE) {
      refreshDisplay = true;
      lastDisplayUpdateTime = currentTime;
    }
  }
  if (refreshDisplay == true) {
    int timerHours = timerRemaining / 60;
    int timerMinutes = timerRemaining - timerHours * 60;
    char heatStr[8];
    sprintf(heatStr, "%3d%%", heatSetting * 10);
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
    if (displayOn == true) {
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
    // DEBUG
    Serial.print("currentTime = ");
    Serial.print(currentTime);
    Serial.print(" LastDisplayOn = ");
    Serial.print(lastDisplayOn);
    Serial.print(" Heat = ");
    Serial.print(heatStr);
    Serial.print(" Timer = ");
    Serial.println(timeStr);
  }
}

void checkTimerOff() {
  currentTime = millis();
  long timerRemainingMillis = timerEndTime - currentTime;
  timerRemaining = round(timerRemainingMillis / 60000);
  if (timerRemaining <= 0) {
    timerRemaining = 0;
    timerOn = false;
  }
  if (timerOn == false) {
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
  if (timerOn == true) {
    unsigned long dutyOnTime = round(HEATER_TIME_CYCLE * (heatSetting * 0.1 * (MAX_DUTY_CYCLE - MIN_DUTY_CYCLE) + MIN_DUTY_CYCLE));
    unsigned long dutyOffTime = HEATER_TIME_CYCLE - dutyOnTime;
    if ((heatSetting >= MIN_TEMP) and (heatSetting <= MAX_TEMP)) {
      if (heaterOn == false) {
        if (currentTime - lastHeatOffTime >= dutyOffTime) {
          // TURN THE HEAT ON
          digitalWrite(HEATER_RELAY_PIN, HIGH);
          Serial.print("Heater ON @ ");
          Serial.println(currentTime);
          heaterOn = true;
          lastHeatOnTime = currentTime;
        }
        //    } else if (heaterOn == true) {
      } else {
        if (currentTime - lastHeatOnTime >= dutyOnTime) {
          // TURN THE HEAT OFF
          digitalWrite(HEATER_RELAY_PIN, LOW);
          Serial.print("Heater OFF @ ");
          Serial.println(currentTime);
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

void setup_pin(unsigned int PIN, ISR_fuction ISR_routine) {
  pinMode(PIN, INPUT_PULLUP);
  attachInterrupt(PIN, ISR_routine, FALLING);
}

void setup() {
  // Setup Serial Monitor
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_I2C)) {
    Serial.println(F("SSD1306 allocation failed"));
    delay(10000);
    ESP.restart();
  }
  delay(2000);
  display.clearDisplay();
  display.setRotation(3);
  display.ssd1306_command(SSD1306_DISPLAYON);
  Serial.println("ESP32 Controller for a Brookstone Heated Mattress Pad.");
  timerOn = true;
  heaterOn = false;
  pinMode(HEATER_RELAY_PIN, OUTPUT);
  digitalWrite(HEATER_RELAY_PIN, LOW);
  displayOn = true;
  refreshDisplay = true;
  // heaterOn = false;
  enterSleepMode = false;
  heatSetting = DEFAULT_TEMP;
  lastDisplayOn = currentTime;
  timerStartTime = currentTime;
  timerRemaining = DEFAULT_TIMER;
  timerEndTime = timerStartTime + DEFAULT_TIMER * 60000;
  lastButtonPressTime = currentTime;
  // pinMode(POWER_ON_PIN, INPUT_PULLUP);
  // pinMode(TEMP_PLUS_PIN, INPUT_PULLUP);
  // pinMode(TEMP_MINUS_PIN, INPUT_PULLUP);
  // pinMode(TIMER_PLUS_PIN, INPUT_PULLUP);
  // pinMode(TIMER_MINUS_PIN, INPUT_PULLUP);
  // attachInterrupt(POWER_ON_PIN, power_button_ISR, FALLING);
  // attachInterrupt(TEMP_PLUS_PIN, temp_up_ISR, FALLING);
  // attachInterrupt(TEMP_MINUS_PIN, temp_down_ISR, FALLING);
  // attachInterrupt(TIMER_PLUS_PIN, timer_up_ISR, FALLING);
  // attachInterrupt(TIMER_MINUS_PIN, timer_down_ISR, FALLING);
  setup_pin(POWER_ON_PIN, power_button_ISR);
  setup_pin(TEMP_PLUS_PIN, temp_up_ISR);
  setup_pin(TEMP_MINUS_PIN, temp_down_ISR);
  setup_pin(TIMER_PLUS_PIN, timer_up_ISR);
  setup_pin(TIMER_MINUS_PIN, timer_down_ISR);
  updateDisplay();
}

void loop() {
  checkTimerOff();
  checkPowerOff();
  updateDisplay();
  runHeater();
  delay(100);
}
