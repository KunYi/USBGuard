default: daemon.c 
	gcc -o usbguard-daemon daemon.c -ludev -lrt `pkg-config --cflags --libs libnotify`

install:
	# Install dependencies
	sudo ./install_dependencies.sh

	# Create our system locations
	sudo mkdir -p /etc/usbguard && sudo mkdir -p /usr/lib/usbguard

	# Compile our daemon
	gcc -o /tmp/usbguard-daemon daemon.c -ludev -lrt `pkg-config --cflags --libs libnotify`

	# Copy the config files and binary to the appropriate locations
	sudo mv /tmp/usbguard-daemon /usr/lib/usbguard/ && sudo cp usb-whitelist.cfg /etc/usbguard/ && sudo cp usbguard-daemon.service /lib/systemd/system/

	# Create the symlink to our .service file for systemd
	sudo ln -sf /lib/systemd/system/usbguard-daemon.service /etc/systemd/system/multi-user.target.wants

	# Start the service this one time, systemd will handle subsequent starts
	#sudo /usr/lib/usbguard/usbguard-daemon

