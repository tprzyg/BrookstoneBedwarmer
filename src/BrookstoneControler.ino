/*
 * Brookstone Bedwarmer replacement controller.
 * (C)2022 Tomasz Przygpda <tprzyg@yahoo.com>
 *
 * 4 input buttons (reused from the original controller board) for Temp_Up, Temp_Down, Timer_Up and Timer_Down
 * 1 Power_On/Power_Off button (also reused from the original board) with OFF setting the ESP32 to deep sleep
 * Display was replaced with 0.96" OLED (SSD1306) and replaced messaging
 * Temperature is adjusted from 1 - 10 (10% to 100%)
 * Timer can be adjusted from 3 to 75 (in 10min steps it equals to a range of 30min to 12:30min)
 * Logic was slightly changed to allow for 10min timer-step value (originally 30min step value)
 * and also the display will show not the preset timer value, but rather the remaining hours:minutes
 * Display will dim after 15 seconds and go blank after 60 seconds of inactivity
 * When display is inactive, pressing any button (including power button) will reactivate the display
 * When display is active pressing adjustment buttons will make the desired adjustment
 * and pressing power button with turn off heat, reset timer and put the controller into deep sleep
 * When controller is in deep sleep mode, pressing power button will restart and re-initialize the controller
 * When timer runs out, the controller will turn off heat and put the controller into deep sleep
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

#define MIN_TEMP 1
#define MAX_TEMP 10
#define TEMP_STEP 1
#define DEFAULT_TEMP 2
#define MIN_DUTY_CYCLE 0.1
#define MAX_DUTY_CYCLE 0.8
#define HEATER_TIME_UNIT 1000

#define MIN_TIMER 10
#define MAX_TIMER 720
#define TIMER_STEP 10
#define DEFAULT_TIMER 180

#define DEBOUNCING_TIME 250

// Define PINs for buttons, 
#define TEMP_PLUS_PIN GPIO_NUM_13
#define TEMP_MINUS_PIN GPIO_NUM_12
#define TIMER_PLUS_PIN GPIO_NUM_14
#define TIMER_MINUS_PIN GPIO_NUM_27
#define POWER_PIN GPIO_NUM_26
#define HEATER_RELAY_PIN 23

// Settings for the 0.96" SSD1306 OLED SCREEN
#define SCREEN_I2C 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define DISPLAY_DIMMING 15000
#define DISPLAY_TIMEOUT 60000
// Initialize Screen
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


int heaterOn;
int displayOn;
int displayUpdate;
int enterSleepMode;

int heatSetting;
int timerRemaining;
int lastButtonPressed;

unsigned long currentTime;
unsigned long lastDisplayOn;
unsigned long lastPressTime;
unsigned long timerStartTime;
unsigned long lastUpdateTime;
unsigned long lastHeatOnTime;
unsigned long lastHeatOffTime;
unsigned long currentPressTime;

void IRAM_ATTR temp_up() {
  currentPressTime = millis();
  if (currentPressTime - lastPressTime > DEBOUNCING_TIME) {
    displayUpdate = true;
    lastPressTime = currentPressTime;
    lastButtonPressed = TEMP_PLUS_PIN;
    if (displayOn == true) {
      if (heatSetting < MAX_TEMP - HEAT_STEP) {
        heatSetting = heatSetting + HEAT_STEP;
      }
    } else {
      displayOn = true;
    }
    lastDisplayOn = currentPressTime;
  }
}

void IRAM_ATTR temp_down() {
  currentPressTime = millis();
  if (currentPressTime - lastPressTime > DEBOUNCING_TIME) {
    displayUpdate = true;
    lastPressTime = currentPressTime;
    lastButtonPressed = TEMP_MINUS_PIN;
    if (displayOn == true) {
      if (heatSetting > MIN_TEMP + HEAT_STEP) {
        heatSetting = heatSetting - HEAT_STEP;
      }
    } else {
      displayOn = true;
    }
    lastDisplayOn = currentPressTime;
  }
}

void IRAM_ATTR timer_up() {
  currentPressTime = millis();
  if (currentPressTime - lastPressTime > DEBOUNCING_TIME) {
    displayUpdate = true;
    lastPressTime = currentPressTime;
    lastButtonPressed = TIMER_PLUS_PIN;
    if (displayOn == true) {
      if ((timerRemaining > 0) and (timerRemaining < MAX_TIMER - TIMER_STEP)) {
        timerRemaining = timerRemainig + TIMER_STEP;
      }
    } else {
      displayOn = true;
    }
    lastDisplayOn = currentPressTime;
  }
}

void IRAM_ATTR timer_down() {
  currentPressTime = millis();
  if (currentPressTime - lastPressTime > DEBOUNCING_TIME) {
    displayUpdate = true;
    lastPressTime = currentPressTime;
    lastButtonPressed = TIMER_MINUS_PIN;
    if (displayOn == true) {
      if (timerRemainig > MIN_TIMER + TIMER_STEP) {
        timerRemaining = timerRemaining - TIMER_STEP;
      }
    } else {
      displayOn = true;
    }
    lastDisplayOn = currentPressTime;
  }
}

void IRAM_ATTR power_button() {
  currentPressTime = millis();
  if (currentPressTime - lastPressTime > DEBOUNCING_TIME) {
    displayUpdate = true;
    lastPressTime = currentPressTime;
    lastButtonPressed = POWER_ON_PIN;
    if (displayOn == true) {
      enterSleepMode = true;
    } else {
      displayOn = true;
      lastDisplayOn = currentPressTime;
    }
  }
}

void updateDisplay(int myHeat, int myTime) {
  unsigned long currentTime = millis();
  int myHours = myTime / 60;
  int myMinutes = myTime - myHours * 60;
  char heatStr[8];
  sprintf(heatStr, "%3d%%", myHeat * 10);
  char timeStr[8];
  sprintf(timeStr, "%2d:%02d", myHours, myMinutes);
  if (currentTime - lastDisplayOn > DISPLAY_TIMEOUT) {
    displayOn = false;
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }
  if (displayOn == true) {
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

void runHeater(int myHeat) {
  currentTime = millis();
  unsigned long dutyOnTime = round(HEATER_TIME_UNIT * (myHeat * 0.1 * (MAX_DUTY_CYCLE - MIN_DUTY_CYCLE) + MIN_DUTY_CYCLE));
  unsigned long dutyOffTime = HEATER_TIME_UNIT - dutyOnTime;
  if (timerRemaining > 0) and ((heatSetting >= MIN_TEMP and (heatSetting <= MAX_TEMP)) {
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
 	  if (digitalRead(HEATER_RELAY_PIN) == HIGH) {
      Serial.print("Turning Heater OFF @ ");
      Serial.println(currentTime);
      digitalWrite(HEATER_RELAY_PIN, LOW);
    }
  }
}

void deepSleepMode() {
  unsigned long currentTime = millis();
  heatSetting = 0;
  timerRemaining = 0;
  if (digitalRead(HEATER_RELAY_PIN) == HIGH) {
    Serial.print("Turning Heater OFF @ ");
    Serial.println(currentTime);
    digitalWrite(HEATER_RELAY_PIN, LOW);
  }
  Serial.println("Turning timer OFF");
  Serial.println("Turning display OFF");
  display.clearDisplay();
  display.display();
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  Serial.print("Setting up RTC on ");
  Serial.print(WAKEUP_PIN);
  Serial.println(" to become a wakeup button");
  esp_sleep_enable_ext0_wakeup(WAKEUP_PIN, 0);
  delay(1000);
  Serial.println("Entering SEEP_SLEEP mode");
  esp_deep_sleep_start();
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
  Serial.println("Bedwarmer Controller Mockup");
  displayOn = true;
  heaterOn = false;
  displayUpdate = true;
  enterSleepMode = false;
  currentTime = millis();
  lastPressTime = currentTime;
  timerStartTime = currentTime;
  lastDisplayOn = currentTime;
  heatSetting = DEFAULT_TEMP;
  timerRemaining = DEFAULT_TIMER;
  pinMode(TEMP_UP_PIN, INPUT_PULLUP);
  pinMode(TEMP_DOWN_PIN, INPUT_PULLUP);
  pinMode(TIMER_UP_PIN, INPUT_PULLUP);
  pinMode(TIMER_DOWN_PIN, INPUT_PULLUP);
  pinMode(POWER_ON_PIN, INPUT_PULLUP);
  pinMode(HEATER_RELAY_PIN, OUTPUT);
  digitalWrite(HEATER_RELAY_PIN, LOW);
  attachInterrupt(TEMP_UP_PIN, temp_up, FALLING);
  attachInterrupt(TEMP_DOWN_PIN, temp_down, FALLING);
  attachInterrupt(TIMER_UP_PIN, timer_up, FALLING);
  attachInterrupt(TIMER_DOWN_PIN, timer_down, FALLING);
  attachInterrupt(POWER_ON_PIN, power_button, FALLING);
}

void loop() {
  unsigned long currentTime = millis();
  if (enterSleepMode == true) {
    deepSleepMode();
  }
  if (timerRemamining > 0) {
  
  }
  runHeater(heatSetting);
  if (displayOn == true) {
    if (currentTime - lastUpdateTime >= 10000) {
      displayUpdate = true;
      lastUpdateTime = currentTime;
    }
  }
  if (displayUpdate == true) {
    updateDisplay(heatSetting, timerRemaining);
    if (remainingMinutes <= 0) {
      enterSleepMode = true;
    }
    displayUpdate = false;
  }
  delay(100);
}
