/*

	Linux USBGuard Daemon

	Runs in the background and handles USB related signals

*/

#define _GNU_SOURCE
#include <stdio.h>    //printf(3)
#include <stdlib.h>   //exit(3)
#include <unistd.h>   //fork(3), chdir(3), sysconf(3)
#include <signal.h>   //signal(3)
#include <sys/stat.h> //umask(3)
#include <syslog.h>   //syslog(3), openlog(3), closelog(3)

// Necessary for Udev monitoring
#include <libudev.h>  // apt-get install libudev-dev

// Necessary for Desktop notifications
#include <libnotify/notify.h> // apt-get install libnotify-dev

#define SUBSYSTEM "usb"

int daemonize() {

	pid_t pid;

	pid = fork();

	if (pid < 0){
		exit(EXIT_FAILURE);
	}

	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	// On success: The child process becomes session leader
	if (setsid() < 0) {
		exit(EXIT_FAILURE);
	}

	/* Catch, ignore and handle signals */
	//TODO: Implement a working signal handler */
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	// Fork off for the second time
	pid = fork();

	// An error occurred
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	// Success: Let the parent terminate 
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	// Set new file permissions
	umask(0);

	// Change the working directory to the root directory 
	// or another appropriated directory
	chdir("/");

	// Close all open file descriptors 
	int x;
	for (x = sysconf(_SC_OPEN_MAX); x>=0; x--) {
		close (x);
	}

	// Open the log file 
	openlog ("USBGuard", LOG_PID, LOG_DAEMON);

	return 0;
}



static void device_notification(struct udev_device* dev)
{

	notify_init("Sample");
	NotifyNotification * n = NULL;
	char * message;

	const char* action = udev_device_get_action(dev);
	if (! action) {
		action = "exists";
	}

	const char* vendor = udev_device_get_sysattr_value(dev, "idVendor");
	if (! vendor) {
		vendor = "0000";
	}

	const char* product = udev_device_get_sysattr_value(dev, "idProduct");
	if (! product) {
		product = "0000";
	}


	if (0> asprintf(&message, 
		"Node: %s\n   Subsystem: %s\n   Devtype: %s\n   Action: %s\n",
		udev_device_get_devnode(dev), udev_device_get_subsystem(dev), udev_device_get_devtype(dev), udev_device_get_action(dev))) {
		syslog(LOG_WARNING, "[-] Format String failed");
		return;
	}

	n = notify_notification_new ("USBGuard", message, 0);

	if(!notify_notification_show(n, 0)) {
		syslog(LOG_WARNING, "[-] Could not display message");
		return;
	}
}

static void process_device(struct udev_device* dev)
{
	if (dev) {
		if (udev_device_get_devnode(dev))
			device_notification(dev);

		udev_device_unref(dev);
	}
}


/*
 * returns 0 if no match, 1 if match
 *
 * Taken from drivers/usb/core/driver.c, as it's not exported for our use :(
 
static int usb_match_device(struct usb_device *dev,
			    const struct usb_device_id *id)
{
	if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) && id->idVendor != le16_to_cpu(dev->descriptor.idVendor)) {
		return 0;
	}
		

	if ((id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) && id->idProduct != le16_to_cpu(dev->descriptor.idProduct)) {
		return 0;
	}

	/* No need to test id->bcdDevice_lo != 0, since 0 is never
	greater than any unsigned number. 
	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_LO) && (id->bcdDevice_lo > le16_to_cpu(dev->descriptor.bcdDevice))) {
		return 0;
	}

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_HI) && (id->bcdDevice_hi < le16_to_cpu(dev->descriptor.bcdDevice))){
		return 0;
	}

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_CLASS) && (id->bDeviceClass != dev->descriptor.bDeviceClass)) {
		return 0;
	}

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_SUBCLASS) && (id->bDeviceSubClass != dev->descriptor.bDeviceSubClass)) {
		return 0;
	}

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_PROTOCOL) && (id->bDeviceProtocol != dev->descriptor.bDeviceProtocol)) {
		return 0;
	}

	return 1;
} */



static void monitor_devices(struct udev* udev)
{
	struct udev_monitor* mon = udev_monitor_new_from_netlink(udev, "udev");

	udev_monitor_filter_add_match_subsystem_devtype(mon, SUBSYSTEM, NULL);
	udev_monitor_enable_receiving(mon);

	int fd = udev_monitor_get_fd(mon);

	while (1) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		// Wait for monitor to return
		int ret = select(fd+1, &fds, NULL, NULL, NULL);
		if (ret <= 0) {
			break;
		}
		

		if (FD_ISSET(fd, &fds)) {
			struct udev_device* dev = udev_monitor_receive_device(mon);
			process_device(dev);
		}
	}
}



int main(int * argc, int ** argv) {

	notify_init("Sample");
	NotifyNotification * n = NULL;
	char * message;

	struct udev *udev;
	/* Create the udev object */
	udev = udev_new();
	if (!udev) {
		printf("Can't create udev\n");
		exit(1);
	}

	// Daemonize and run in the backgroun
	daemonize();

	if (0> asprintf(&message, "Background service is running...\nPID: %d", getpid())) {
		return EXIT_FAILURE;
	}
	

	n = notify_notification_new ("USBGuard", message, 0);

	notify_notification_set_timeout(n, 5000); // 5 seconds

	if(!notify_notification_show(n, 0)) {
		syslog(LOG_WARNING, "[-] Could not display message");
		return EXIT_FAILURE;
	}
	

	syslog(LOG_NOTICE, "[+] USBGuard Started");

	// Begin monitoring for USB insertions/removals
	monitor_devices(udev);
	//monitor();

	// Cleanup
	syslog(LOG_NOTICE, "[-] USBGuard Stopped");
	closelog();
	return EXIT_SUCCESS;
}