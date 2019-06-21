# RFID password generator
#### Note: This project is just for reference and won't be continued, a similar approach without the RFID interface can be found in the more recent [USB password manager project](https://github.com/julianschuler/USB-password-manager). The reason for the discontination of this project is the minuscule added security in comparison to the extra effort of using a RFID interface for authentification. Furthermore a password manager is an easier and more secure way of generating and storing cryptographically strong passwords, see the [security evaluations section](https://github.com/julianschuler/USB-password-manager#Security-evalutaions) of the USB password manager project.

## Introduction
The RFID password generator is a simple way to generate cryptographic passwords and save them in hardware, only accessable with the right RFID tag.

Using it is as simple as plugging it into a USB port and hold a previously registered RFID tag above, the RFID password generator will type your password. Each RFID tag refers to one password, to add a RFID tag or change the password of a registered one, press the button while holding the RFID tag above it or hit Caps Lock four times before, in both cases a new random password will be generated.

## Variants
This project has two versions, a flat and a normal version. The main difference is the form (the flat version has the dimensions of a credit card and fits perfectly inside your wallet), due to its thickness of just 2.4mm the flat version doesn't include a button for adding and changing passwords, you have to hit Caps Lock four times to do this.

Keep in mind that the flat version is a lot more complicated to build compared to the normal version due to its tiny SMD parts. Nevertheless, you should have at least experience in SMD soldering and Arduino/ATtiny programming, this isn't a beginner project!

## Build Instructions
If you want to rebuild this project, see [Build Instructions](documentation/build-instructions.md).

## Licenses
All files of this project except the pcb and schematic files are licensed under MIT License (see [`LICENSE.txt`](LICENSE.txt) for further information). These pcb and schematic files are licensed under [Creative Commons Attribution Share Alike 4.0](https://creativecommons.org/licenses/by-sa/4.0/).

## To do
- add connection diagrams
- create PCB files for the flat version
- create a parts list for the normal version
