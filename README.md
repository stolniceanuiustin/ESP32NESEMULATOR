## NES Emulator on ESP32 - A project of mine 
I developed a functional emulator for the Nintendo Entertainment System (NES), allowing original games (ROMs) to run on an ESP32s3.   
This is a port of my original NESEmulator, which was designed for x86.  
Currently, it runs Super Mario Bros at a "cinematic", but not real time yet, 24 FPS. 
The CPU emulation is cycle stepped but not cycle accurate - i removed the checks for page crossings, as that took way too much CPU time, while the PPU renders everything on a scanline at once.    
C is used for all the emulation of the system and C++ is used for less performance critical stuff.  
The PS2 Controller "Driver" emulated SPI as i couldn't get the SPI library to work on my ESP32. In reality, the PS2 controller just communicates via type 3 SPI.

## Implemented MAPPERS:
Unfortunetly, the only mapper implemented for now is MAPPER0, and it is hardcoded, as i tried to eliminate as much C++ overhead as possbile.  
After i get this to run at full speed, i will implement other popular mappers.    
So working games include, but are not limited to Super Mario Bros. 1 and Donkey Kong, Pac Man!   



## Prerequisites:
*ESP-IDF or Arduino IDE (I am using PlatformIO IDE)  
*TFT-SPI LIBRARY  
*A compatible ESP32 development board(I use ESP32s3)    
*A display(I use a cheap chinese TFT display). The resolution must be at least 256x240  
*A NES ROM(For obvious reasons, i can't provide one, but i am sure you can find one!)  
*LittleFS to upload the ROMs to your development board  


## How to use
1. Clone this repository  
2. Place your ROMS(only the games that you also own in real life!!!!) in the data folder.
3. Run pio run --target uploadfs
4. Configure your ESP32 for input and output. The way i configured it is:  
   ST7789 Driver and TFT_eSPI library for display.   
   PS2 Controller with a custom PS2 Controller Driver
5. Upload the project to ESP and enjoy :)
