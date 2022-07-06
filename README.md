# xbox360-rpi-flasher
XBox 360 NAND reader/writer for Raspberry Pi 4 (might also work for Pi3b or others, but this is untested)

Warning: this has not been properly tested yet. mav2010 has successfully RGH3.0'd a Corona 16MB model following the steps below. Proceed with caution.

If you want the safe way, get a Raspberry Pi Pico and use Picoflasher. 


Wiring Instructions for Pi 4, following the color coding for the Pico from Weekendmodder (https://weekendmodder.com/picoflasher):
Blue: GPIO23
Green: GPIO24
Black: MOSI (GPIO 10)
Orange: MISO (GPIO 9)
Red: SCLK (GPIO 11)
Brown: GPIO 26
Yellow: Ground


Quick tutorial:

0. follow the link above to understand how to solder the wires to the XBOX360 and connect them to the Pi4 as described. Connect the XBOX to the power supply and give it standby power. Don't turn it on!

1. Compile and run to check general connectivity. It will not read or write anything really, but just establish a general connection and read out hte manufacturer code. It should give an output like this (example from a Corona 16MB Slim):

$ sudo ./xbox-flasher 
Initializing XBOX360 SPI...
Entering flashmode...
Reading flash config...
Flash config1: 0x00000000
Flash config2: 0x00043000

If that works, go on and take the next steps.

2. read NAND: uncomment the 3 nand_to_file() commands near the end of the main.c file and recompile. This dumps the NAND 3 times now, check afterwards with cksum that all dumps are identical and also check that JRunner can read them! If yes, the connection is stable enough to attempt a write.

3. write the ECC image with JRunner. It will provide you with a glitch.ecc file in it's output directory. Transfer this to the Pi 4 into the same folder as the compiled binary.

4. Comment out the 3 nand_to_file() statements again and uncomment the file_to_nand() and nand_to_file() statements. Also adjust the filename to glitch.ecc and update the number of blocks to be read to 0x050 (search in main.c for "glitch.ecc", you will find my comment on where to change). Since glitch.ecc is smaller, we also only want to dump the freshly written part of the NAND, otherwise the checksums will not match. Compile, run, check checksums of input and dumped file.

5. If all is good, you can turn your XBOX on now and Xell should launch. When you obtained your CPU key, use it with JRunner and build the full image. It will give an updflash.bin file in it's output directory. Transfer this to the Pi 4.

6. Adjust the filename to "updflash.bin" and change the number of blocks to be read back to 0x400. Compile, run, compare. If all matches, you should be able to boot and you are done. :-)

Hint: The number of blocks to be read only affects the read procedure, for write the program automatically sets the right number of blocks depending on the file size.
