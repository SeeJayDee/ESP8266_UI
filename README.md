ESP8266 WiFi UI

This is an Arduino sketch for the ESP8266 (with Arduino bootloader), for a WiFi-connected mains relay that has a web interface.

There is a simple serial console interface for configuring the WiFi connection, among other things.
WiFi settings are stored in "EEPROM" (actually flash, but whatever ;) )

The bill of materials:<br>
	> ESP-01 module (ESP8266) <br>
	> SSR-25 Solid-state relay, from eBay, probably not approved by your local electrical regulator<br>
	> FTDI FT232R breakout board<br>
	> 2x push button switches<br>
	> 2x 10k pull-up resistors<br>
	> Mains plug & socket<br>
	> Mains fuse, 10A<br>
	> some kind of non-conductive case for it all<br>
	> A somewhat gung-ho approach to personal safety<br>
	
