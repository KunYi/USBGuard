/*

	Header file for USBGuard on Linux

*/


#define _GNU_SOURCE
#include <stdio.h>    // printf(3)
#include <stdlib.h>   // exit(3)
#include <unistd.h>   // fork(3), chdir(3), sysconf(3)
#include <signal.h>   // signal(3)
#include <sys/stat.h> // umask(3)
#include <syslog.h>   // syslog(3), openlog(3), closelog(3)
#include <string.h>   // strncmp
#include <aio.h> 	  // async I/O 
#include <sys/types.h> // aiocb datatype
#include <fcntl.h> 	  // open() file descriptor
#include <string.h>	  // strsep(3)

// Necessary for Udev monitoring
#include <libudev.h>  // apt-get install libudev-dev

// Necessary for Desktop notifications
#include <libnotify/notify.h> // apt-get install libnotify-dev


#define SUBSYSTEM "usb"

typedef struct _deviceID {

	// Serial number of the device
	char * serialnumber;

	// Vendor ID of the device
	int vendorID;

	// Product ID of the device
	int productID;

	// Pointer to next 
	struct _deviceID * next;

} deviceID;


