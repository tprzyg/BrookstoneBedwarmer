#define MIN_TEMP 1
#define MAX_TEMP 10
#define TEMP_STEP 1
#define DEFAULT_TEMP 2
#define MIN_DUTY_CYCLE 0.1
#define MAX_DUTY_CYCLE 0.8
#define HEATER_TIME_CYCLE 1000

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
#define POWER_ON_PIN GPIO_NUM_26
#define HEATER_RELAY_PIN 23

// Settings for the 0.96" SSD1306 OLED SCREEN
#define SCREEN_I2C 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define DISPLAY_DIMMING 15000
#define DISPLAY_TIMEOUT 60000
#define DISPLAY_REFRESH_RATE 10000
