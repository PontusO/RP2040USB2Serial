# RP2040USB2Serial
An Arduino based USB to serial adapter for the RP2040 explicitly made for flashing ESP devices on Challenger RP2040 boards with different ESP devices on them.

This firmware have been tested with and supports the following ESP devices:
 - ESP8285
 - ESP32-C3
 - ESP32-C6

# Building and downloading
When compiling this program the following build options must be met:
 - The correct board must be set otherwise it will most certainly not work.
 - You must use the Tiny USB stack otherwise you will not get the advanced options such as RTS/DTS and change of baudrate that i required for the USB2Serial adapter to work properly.

If for some reason things doesn't work you can enable DEBUG information by setting the DEBUG_LVL flag to either 1 or 2. 1 Will simply show the status of the RTS/DTR control signals and basic data flow.
Setting it to 2 will enable output of all data flowing from and to the ESP device itself.

If you are having problems with data not comming back from the ESP32 to esptool please try our own fork of esptool which has some alterations that makes it work more smoothly when the RP2040 is acting as a USB2Serial gateway. You can find it [here](https://github.com/PontusO/esptool).
