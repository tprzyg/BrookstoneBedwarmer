# BrookstoneBedwarmer
This was a project to replace a flaky controller for a Brookstone Bedwarmer / Heated Mattress Pad.

I used ESP32 board with 0.96" OLED SSD1306 type display, set of two solid state relays (for contorlling the A/C power) and a small 5V PSU board and a 2A fuse with a housing. Below are links to the products on Japanese Amazon site (I'm sure similar/comparable products exist on Amazon in any locality they serve or any electronics parts store).
- ESP32 DevKit boards: https://amzn.asia/d/9b7Ar7T
- 5V power modules: https://amzn.asia/d/gfirg1F
- Glass tube fuses and inline housings: https://amzn.asia/d/3iPEoGX
- Solid State Relays: https://amzn.asia/d/8DtwuSe
- 0.96" OLD Displays: https://amzn.asia/d/c7P6Bzr

This project could work with very minor modifications using ESP8266 board as well (I just happened to have extra ESP32 and no ESP8266 on hand).

Wiring the OLED screen is I2C device, and using specific pins on ESP32 as SDA (GPIO_21) and SCL (GPIO_22) - these is different on ESP8266!
Also the OLED screen uses 3.3V, so it's wired to the 3.3V output on ESP32 rather than using separate 3.3V regulator board.
I reused housing and most of the board of the original Brookstone controller (but desoldered most of the components - leaving only switches on the board).
I masked the display window with black electrical tape (as the 0.96" OLED is slightly smaller than the original segmented LCD - about 2mm on each side of the screen is masked). The OLED is (unprofessinally) mounted to the hosuing with just black electrical tape. I aldo used copious amounts of electrical tape to cover the original board, to prevent any prospects of something touching PCB and shorting.
I Checked carefully which wire brins "live" and which is neutral and connected the bedwarmer with a fuse on that line, to make sure that if anything electrical malfunction happens there's no "live" current in the bedwarmer / mattress pad.
