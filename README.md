phloppy\_0
=========

phloppy\_0 is a [Commodore Amiga](https://en.wikipedia.org/wiki/Amiga) floppy
drive emulator. It emulates four drives (`DF0:`, `DF1:`, `DF2:`, `DF3:`) and
is controlled through WiFi by an Android or a PC application.

`DF0:` - `DF3:` interoperation
--------------------------

The device is connected between the Amiga motherboard and the internal floppy
drive (DF0:). When a floppy image is inserted in the virtual DF0: the physical
drive input signals `SEL0` and `MTR0` are inactive. When there is no floppy image
in the virtual DF0: the `SEL0` and `MTR0` signals are passed from the Amiga
motherboard through the emulator to the physical drive.

When there are `DF1:` - `DF3`: physical drives connected to the Amiga, inserting
a floppy image in any of the virtual `DF1:` - `DF3:` will result in malfunction
of the drive.

Supported floppy image formats
------------------------------
 
Two floppy image formats are supported:
- ADF ([Amiga Disk File](https://en.wikipedia.org/wiki/Amiga\_Disk\_File)), 880 kB
- raw MFM - 12668 bytes per track, 160 tracks per disk

Hardware
--------

Main hardware components:
- STM32F446 - Cortex-M4 microcontroller; core of the emulator
- SDRAM, 8 MB; used for keeping floppy image contents
- ESP12-E - ESP8266 based WiFi module; provides WiFi hotspot to allow connection
  of a controlling application
- XC9272XL CPLD & two 8-channel open-drain buffers - interface between the
  microcontroller and the Amiga motherboard

On-board connectors:
- micro USB - for flashing the microcontroller
- SWD - for flashing/debugging the microcontroller
- JTAG - for flashing the CPLD
- UART (3.3 V) - for flashing the ESP8266
- power input - from the Amiga motherboard
- power output - to the physical floppy drive
- SEL2, SEL3 - input for the appropriate floppy selection signals
- male 34-pin pinheader - for connecting through a ribbon cable with
  the Amiga motherboard; for A600 the cable should be around 155 mm long
- female 34-pin pinheader - for connecting directly to the physical floppy
  drive

LEDs:
- green - indicates SDRAM active
- yellow - indicates read operation in progress
- red - indicates communication with the controlling application
  in progress
- blue - indicates write buffer is not empty (turn on the controlling
  application to write the buffer to the persistent memory)

In idle mode the device consumes around 170 mA. When the SDRAM is in active
mode the current consumption is around 185 mA.

Software
--------

Software components:
- `android/` - Android control application sources
- `cpld/` - CPLD design files
- `output/` - APK, JED, Gerbers, microcontroller firmware
- `pc/` - PC control application sources
- `uc/` - microcontroller firmware sources
- `wifi/` - ESP8266 source files

The Android application requires SDK level 15 and works at least on
Android 4.0.3 and 8.0.0.

The WiFi hotspot parameters are stored in the STM32 microcontroller and
read by ESP8266 during initialization. The parameters can be changed in source
code or after compilation in the microcontroller firmware image file.

License
-------

The project is licensed under GPL-3.0 license. See `LICENSE` for more information.
Parts of the project are covered by other licenses (see appropriate files for details):
- ESP8266 SPI driver by David Ogilvy (MetalPhreak)
- CMSIS header files by ARM LIMITED
- STM32 header files by STMicroelectronics
