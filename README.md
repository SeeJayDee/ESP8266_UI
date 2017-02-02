ESP8266 WiFi UI

This is an Arduino sketch for the ESP8266 (with Arduino bootloader), for a WiFi-connected mains relay that has a web interface.

There is a simple serial console interface for configuring the WiFi connection, among other things.
WiFi settings are stored in "EEPROM" (actually flash, but whatever ;) )

The bill of materials:
	> ESP-01 module (ESP8266)
	> SSR-25 Solid-state relay, from eBay, probably not approved by your local electrical regulator
	> FTDI FT232R breakout board
	> 2x push button switches
	> 2x 10k pull-up resistors
	> Mains plug & socket
	> Mains fuse, 10A
	> some kind of non-conductive case for it all
	> A somewhat gung-ho approach to personal safety
	