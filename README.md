  ESP8266 WiFi Clamp Meter Stove Alarm
==========================================

1. The Problem
--------------

  This project started as a way to warn autistic occupants of a home
that they left the stove on for an extended period of time.  The first
version
[Bluetooth Clamp Meter](https://github.com/rickbronson/Bluetooth-Clamp-Meter-Stove-Alarm) used
a Bluetooth module, this one uses a ESP8266 WiFi module.

2. Design
--------------

  The clamp meter was bought [Clamp Meter](https://www.aliexpress.com/item/MT87-3-1-2-Digits-LCD-Digital-Clamp-Meter-AC-DC-Voltmeter-AC-Ammeter-Ohmmeter-Diode/32437352606.html?spm=2114.13010608.0.0.xfcRNw)

  The ESP8266 module was bought [ESP8266](https://www.aliexpress.com/item/The-new-ESP8266-ESP-12E-serial-WIFI-wireless-module-wireless-transceiver-Complete-circuit-impedance-matching-better/32320927821.html?spm=2114.13010308.0.0.mH8AbN)

  I tried to figure out what chip the MT87 uses but was unsuccessful.
It's probably close to a Intersil ICL7107 or clone.  So some reverse
engineering was in order.  It was determined that the analog input to
the chip was at D in the picture below and that the switched 3 volts
from the MT87 battery was at the bottom of R45 at A in the picture
below.  Bring B low to turn on the piezo buzzer.  C is the input for
volts and resistance measurement.  D is for current measurement.  E is
+3 volts and F is GND.  The boards are wired as such:

| Key on diagram  | ESP8266      | MT87  |
| ------------- | ------------- | ------------- |
|A              | ADC  |Pin 1 LM358 (Current measurement) |
|B              | GPIO2 | Pin 2 LM358 (set low to beep)    |

  Here is a diagram of how everything is connected.

![alt text](https://github.com/rickbronson/WiFi-Clamp-Meter-Stove-Alarm/blob/master/docs/hardware/stove-alarm5.png)

Here is the clamp meter before any modifications.

![alt text](https://github.com/rickbronson/WiFi-Clamp-Meter-Stove-Alarm/blob/master/docs/hardware/before.png)

  Here is the end product with a connector on the bottom of the meter
for a 5 volt power supply to plug in.  I added in a jack for power
with a 3.3 volt regulator soldered in at the jack to feed 3.3v to the
ESP8266 and the meter.  I had to clip off the header pins from the
bottom of the WiFi board to get it down in height so it would fit
inside the meter.

![alt text](https://github.com/rickbronson/WiFi-Clamp-Meter-Stove-Alarm/blob/master/docs/hardware/finished.png)

2. Hardware Design
--------------
 
  A button is tied from GPIO0 to ground to
	reset the WiFi module back to AP+STA (both Access Point and Station
	modes) so that you can then go to 192.168.4.1 and set it up to
	attach to your local network (ie. your router).  You can see the
	whole drilled in the case to the left of the ESP8266 module that the
	wires go through to the push button.

3. Software Design
--------------

  I started with the very simple and well written
	http://git.spritesserver.nl/esphttpd.git/ and changed it ever so
	slightly.  Clicking on the LED ON button changes the meaning of the
	LED such that the LED stays on normally but goes off to show
	movement.  When the AP button (see above) is pressed for more than 5
	seconds then the WiFi goes to AP+STA (both Access Point and Station
	modes).  On your computer or phone, scan for WiFi hotspots and
	something like "ESP_02F89B" should show up, connect to it then open
	a browser to 192.168.4.1 and set it up.  Click on the end of "If you
	haven't connected this device to your WLAN network now, you can do
	so." and then select your wireless router, enter your password.  It
	will tell you the IP address that it got, write that down.  Now
	connect back up to your home router and go to that IP address with a
	browser, you should see the same page that you saw at 192.168.4.1.

4. Parts
--------------
  I had many of the parts but here are some that I ordered:

[FTDI-USB](http://www.aliexpress.com/item/1pcs-FT232RL-FTDI-USB-3-3V-5-5V-to-TTL-Serial-Adapter-Module-for-Arduino-Mini/2019421866.html)

[LM1117](http://www.aliexpress.com/item/Free-Shipping-10PCS-LOT-Original-AMS1117-3-3V-AMS1117-LM1117-Voltage-Regulator-We-only-provide-good/32452410702.html)

5. Tools
--------------

  Debian Linux was used to build using
  https://github.com/pfalcon/esp-open-sdk.  Here is a rough guide to
  build everything.

```
sudo apt-get install binutils expat libtool-bin make unrar autoconf \
     automake libtool gcc g++ gperf flex bison texinfo gawk ncurses-dev \
     libexpat-dev python python-serial sed git
mkdir -p ~/boards/esp8266
cd ~/boards/esp8266
git clone https://github.com/rickbronson/WiFi-Clamp-Meter-Stove-Alarm.git
git clone https://github.com/pfalcon/esp-open-sdk
cd esp-open-sdk; make STANDALONE=n
sed -i -e "s/#if 0/#if 1/" esp_iot_sdk_v1.4.0/include/c_types.h
cd ../WiFi-Clamp-Meter-Stove-Alarm; make
```

  You will need to change SDK_BASE in
WiFi-Clamp-Meter-Stove-Alarm/esphttpdconfig.mk since the version
went from esp_iot_sdk_v1.3.0 to esp_iot_sdk_v1.4.0.

  I had to change esp-open-sdk/esptool/esptool.py because of serial
comm trouble:

```
!     ESP_RAM_BLOCK = 0x180
!     ESP_FLASH_BLOCK = 0x40 
```
