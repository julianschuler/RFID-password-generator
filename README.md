# RFID password generator
## Introduction
The RFID password generator is a simple way to generate cryptographic passwords and save them in hardware, only accessable with the right RFID tag.

Using it is as simple as plugging it into a USB port and hold a previously registered RFID tag above, the RFID password generator will type your password. Each RFID tag refers to one password, to add a RFID tag or change the password of a registered one, press the button while holding the RFID tag above it or hit Caps Lock four times before, in both cases a new random password will be generated.

## Variants
This project has two versions, a flat and a normal version. The main difference is the form (the flat version has the dimensions of a credit card and fits perfectly inside your wallet), due to its thickness of just 2.4mm the flat version doesn't include a button for adding and changing passwords, you have to hit Caps Lock four times to do this.

Keep in mind that the flat version is a lot more complicated to build compared to the normal version due to its tiny SMD parts. Nevertheless, you should have at least experience in SMD soldering and Arduino/ATtiny programming, this isn't a beginner project!

## Build Instructions
If you want to rebuild this project, see [Build Instructions](documentation/build-instructions.md).

## References 
This project includes a modified version of the [MFRC522 library](https://github.com/miguelbalboa/rfid) to function properly with the ATtiny84.

## Licenses
All files of this project except the pcb and schematic files are licensed under MIT License (see [`LICENSE.txt`](LICENSE.txt) for further information). These pcb and schematic files are licensed under [Creative Commons Attribution Share Alike 4.0](https://creativecommons.org/licenses/by-sa/4.0/).

## To-do
- add connection diagrams
- create PCB files for the flat version
- create a parts list for the normal version

##### Note: this isn't a finished project yet, not all listed features are already included!
