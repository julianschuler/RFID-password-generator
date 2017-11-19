# Build Instructions
## Additional components required
### Hardware
- AVR programmer (in this tutorial an Arduino is used [as ISP](https://www.arduino.cc/en/Tutorial/ArduinoISP), using another programmer should be a similar procedure)
- soldering iron, solder
- for programming the ATtiny: jumper cables, 10µF capacitor (just for the Arduino as ISP)
- for the flat version: at least a hot air gun, ideally a hot air soldering station
- optional: flux (highly recommended for the flat version)

### Software
- [Arduino IDE](https://www.arduino.cc/en/main/software) (use 1.6.4+, older Versions may not work)
- [USBKeyboard library](https://github.com/julianschuler/USBKeyboard)
- avrdude (the easiest way to install it under Windows is installing [WinAVR](https://www.sourceforge.net/projects/winavr/))

## Assembly
### Normal version
Get yourself all parts listed in [`parts-list-normal-version.txt`](parts-list-normal-version.txt) and solder them to the pcb, starting with the SMDs. **Do not solder the ATtiny directly to the pcb, use the socket instead!** 

### Flat version
Solder all parts listed in [`parts-list-flat-version.txt`](parts-list-flat-version.txt) to the main pcb. For soldering the QFN parts it is recommended to add a tiny amount of solder on the pcb pads, apply a lot of flux around them and position the parts. Heat the parts one by one with hot air until the solder starts flowing, after that the parts should align themselves. **Be careful not to heat the parts to long, otherwise you can damage them!**


## Programming the ATtiny84A
If you have no AVR programmer, set up your Arduino [as ISP](https://www.arduino.cc/en/Tutorial/ArduinoISP) and connect it to the ATtiny as shown below.

[need to add connection diagrams here]

Copy the [USBKeyboard library](https://github.com/julianschuler/USBKeyboard) into your Arduino libraries folder, which can normally be found under Documents/Arduino/libraries

Open the sketch [`rfid-password-generator.ino`](../sourcecode/RFID-password-generator/RFID-password-generator.ino) with the Arduino IDE and open _File > Preferences_. Paste `http://drazzy.com/package_drazzy.com_index.json` into the _Additional Boards manager URLs_ field near the bottum and click _OK_. To install the ATtiny Core open _Tools > Board > Boards Manager_. Scroll to the bottom, select _ATTinyCore by Spence Konde_ and click on the _Install_ button.

Go to _Tools_ and select `Board: "ATtiny24/44/84"`, `B.O.D.: "B.O.D. Disabled"`, `LTO (1.6.11+ only): "Disabled"`, `Pin Mapping: "Counterclockwise (like ATTinyCore)"`, `Clock: "12 MHz (external)"`, `Chip "ATtiny84"`, `Port: "COMx"` (`COMx` ist your Arduino port, e. g. `COM3`), `Programmer: "Arduino as ISP"`.
If you want build the flat version, be sure to set `#define flatVersion` to 1 in the sixth line. Compile and flash the sketch by clicking on the upload button.

Press `Win+R`, type in `cmd` and hit Enter to run the command prompt. Paste `avrdude -c arduino -p t84 -p COMx -b 19200 -U lock:w:0xfc:m` in and change `COMx` to your Arduino port, e. g. `COM3`. By running the command, you lock your ATtiny, this way the flash and especially the EEPROM, where your passwords are stored, are unaccessable from the outside.
The ATtiny is still reprogrammable, but the EEPROM content gets lost by doing this.

Plug the ATtiny into the socket respectively disconnect the Arduino/AVR programmer from the programming header. Add an unused  RFID tag by holding it above the RFID password generator plugged into an USB port while pressing the button or after hitting Caps Lock four times, this tag will be your master tag. By repeating this process with the master tag, all saved RFID tags and their corresponding passwords will be deleted.

In the current configuration ten RFID tags (+ master tag) and therefore ten passwords with 46 colums each can be saved, it can be adjusted by changing `#define passwordLength` to the desired value. The number of RFID tags in relation on the password length can be calculated by `507/(password length + 4)`.