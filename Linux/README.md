# USBGuard-Linux

### Overview

The methodology involves using the libudev library to setup a USB event listener. When a USB device is inserted the background daemon automatically checks the whitelist (stored in memory as a linked list) for the new device. If the device isn't on the whitelist it is force disconnected from the system using /sys/bus/usb/drivers/usb/unbind

  
### Installation

To install all the dependencies use the included script:

`sudo ./install_dependencies.sh`

USBGuard uses libudev-dev to poll usb devices, and libnotify-dev to send notifications to the Ubuntu desktop. After installing the required dependencies simply use the makefile to build the daemon:

`make`

Once you've compiled the daemon, make sure to edit your configuration file to whitelist a few devices to test it out. Then run:

`sudo ./usbguard-daemon`


### TODO:

* Create the usb-whitelist.cfg file in /etc/usbguard/ during installation
* Create signal handlers to restart the service after updating the cfg file
* Add the functionality to kill power to the usb port if the hub is a smarthub
