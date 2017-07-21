#!/bin/bash

# use which to find location of libudev and libnotify
libudev="$(which libudev)"
libnotify="$(which libnotify)"
# execute lsusb to check for all usb ports on system. 
lsusb="$(lsusb)" 

if ! [ -z "$libudev" ]  && ! [ -z "$libnotify" ]; then 
	echo -e "\033[92mlibrary dependencies are met\033[0m"
elif [ -z "$libudev"] && ! [ -z "$libnotify" ]; then
	echo -e "\033[93mlibudev is not installed\033[0m"
elif ! [ -z "$libudev"] && [ -z "$libnotify" ]; then
	echo -e "\033[93mlibnotify is not installed\033[0m."
else
	echo -e "\033[31mneither libnotify nor libudev is installed\033[0m"
fi


if [ -z "$lsusb" ]; then
	echo -e "\033[31mThere are no usb ports on this device\033[0m"
fi	


