# Brookstone Bedwarmer / Heated Mattress Pad Controller.

![alt text](https://github.com/tprzyg/[BrookstoneBedwarmer]/blob/[main]/resources/Bedwarmer_bb.jpg?raw=true)

This was a project to replace a flaky controller for a Brookstone Bedwarmer / Heated Mattress Pad, purchased at Bed Bath and Beyond. The heated mattress pad worked well in the beginning in the US, but after moving to Japan it became very flaky (most likely not liking 100V/50Hz instead of 110V/60Hz).

I used ESP32 board with 0.96" OLED SSD1306 type display, a set of two solid state relays (for controlling the A/C power), a small 5V PSU board and a 2A fuse with a housing. Below are links to the products on Japanese Amazon site (I'm sure that some similar/comparable products exist on your local Amazon or at some local electronics parts store).
- ESP32 DevKit boards: https://amzn.asia/d/9b7Ar7T
- 5V power modules: https://amzn.asia/d/gfirg1F
- Glass tube fuses and inline housings: https://amzn.asia/d/3iPEoGX
- Solid State Relays: https://amzn.asia/d/8DtwuSe
- 0.96" OLED Displays: https://amzn.asia/d/c7P6Bzr

This project could work with some modifications using ESP8266 board as well (I just happened to have extra ESP32 and no ESP8266 on hand).

Wiring the OLED screen is using I2C specific pins on ESP32 as SDA (GPIO_21) and SCL (GPIO_22) - these will be different when using ESP8266!
Also, the OLED screen uses 3.3V, so it is wired to the 3.3V output on ESP32 rather than using separate 3.3V regulator board.
I reused housing and most of the board of the original Brookstone controller (but desoldered most of the components - leaving only buttons/switches on the board).
I masked the display window with black electrical tape (as the 0.96" OLED is slightly smaller than the original segmented LCD - about2mm (about 0.08 in) on each side of the screen is masked). The OLED is (unprofessionally) mounted to the housing with the same black electrical tape. I also used copious amounts of electrical tape to cover the original board, to prevent any prospects of something touching PCB and shorting.
I checked carefully which wire brings "live" A/C and which is neutral and connected the bedwarmer heating element with a fuse on that line, to make sure that if anything electrical malfunction happens there is no "live" current in the bedwarmer / mattress pad.
I also slightly changed the behavior of the controller. The original controller default timer setting was 10hrs with adjustments by 30min up/down and the timer restarted every timer the timer+ or timer- buttons were pressed. My implementation uses a default 3 hrs. timer and with timer+/timer- you can increase or decrease the runtime in 10min steps, without restarting the timer.
The display also shows the remaining time on the timer, rather than the timer setting.
