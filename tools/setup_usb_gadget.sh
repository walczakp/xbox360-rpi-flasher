#!/bin/bash
# Setup Raspberry Pi 4 as USB Serial Gadget for PiFlasher
# Unique VID/PID for JRunner detection
# Run as root on the Pi

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

echo "[*] Configuring USB Gadget (PiFlasher)..."

modprobe libcomposite

GADGET_DIR=/sys/kernel/config/usb_gadget/piflasher

# Cleanup
if [ -d "$GADGET_DIR" ]; then
    echo "[*] Cleaning up previous configuration..."
    echo "" > $GADGET_DIR/UDC 2>/dev/null
    rm $GADGET_DIR/configs/c.1/acm.usb0 2>/dev/null
    rmdir $GADGET_DIR/functions/acm.usb0 2>/dev/null
    rmdir $GADGET_DIR/configs/c.1/strings/0x409 2>/dev/null
    rmdir $GADGET_DIR/configs/c.1 2>/dev/null
    rmdir $GADGET_DIR/strings/0x409 2>/dev/null
    rmdir $GADGET_DIR 2>/dev/null
fi

# Setup with UNIQUE PiFlasher IDs
mkdir -p $GADGET_DIR
cd $GADGET_DIR

# ** UNIQUE PiFlasher VID/PID **
echo 0x1209 > idVendor    # pid.codes open-source VID
echo 0x4360 > idProduct   # PiFlasher PID ("4" for Pi4, "360" for Xbox 360)
echo 0x0100 > bcdDevice
echo 0x0200 > bcdUSB

# Device strings
mkdir -p strings/0x409
echo "RPi4-PiFlasher" > strings/0x409/serialnumber
echo "PiFlasher" > strings/0x409/manufacturer
echo "PiFlasher - RPi4 Xbox 360 Flasher" > strings/0x409/product

# Config
mkdir -p configs/c.1/strings/0x409
echo "Config 1" > configs/c.1/strings/0x409/configuration
echo 100 > configs/c.1/MaxPower

# Function (Serial ACM - CDC)
mkdir -p functions/acm.usb0
ln -s functions/acm.usb0 configs/c.1/

# Enable
if [ "$(ls -A /sys/class/udc 2>/dev/null)" ]; then
    ls /sys/class/udc | head -n 1 > UDC
    echo "[+] Gadget enabled successfully!"
    echo "[*] VID: 0x1209  PID: 0x4360 (PiFlasher)"
    echo "[*] Serial device: /dev/ttyGS0"
    echo "[*] Start server: sudo ./bin/pi4flasher server"
else
    echo "[-] No UDC driver found. Ensure 'dtoverlay=dwc2' is in config.txt and reboot."
fi
