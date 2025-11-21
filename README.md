## NES Emulator on ESP32 - A project of mine 
I developed a functional emulator for the Nintendo Entertainment System (NES), allowing original games (ROMs) to run on an ESP32s3.   
This is a port of my original NESEmulator, which was designed for x86.  
Currently, it runs at 11 FPS.  
The CPU emulation is cycle accurate, while the PPU renders everything on a scanline at once.  

## Implemented MAPPERS:
Unfortunetly, the only mapper implemented for now is MAPPER0, and it is hardcoded, as i tried to eliminate as much C++ overhead as possbile.  
After i get this to run at full speed, i will implement other popular mappers.  

## Prerequisites:
*ESP-IDF or Arduino IDE (I am using PlatformIO IDE)  
*TFT-SPI LIBRARY  
*A compatible ESP32 development board(I use ESP32s3)  
*A display(I use a cheap chinese TFT display). The resolution must be at least 256x240

##
 
