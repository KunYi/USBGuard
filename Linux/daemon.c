/*

	Linux USBGuard Daemon

	Runs in the background and handles USB related signals

*/

#include "daemon.h"


// The whitelist
deviceID * whitelist = NULL;


int disconnect_device(struct udev_device* dev) {


	/* http://man7.org/linux/man-pages/man7/aio.7.html
	struct aiocb {
		 The order of these fields is implementation-dependent 

		int             aio_fildes;      File descriptor 
		off_t           aio_offset;      File offset 
		volatile void  *aio_buf;         Location of buffer 
		size_t          aio_nbytes;      Length of transfer 
		int             aio_reqprio;     Request priority 
		struct sigevent aio_sigevent;    Notification method 
		int             aio_lio_opcode;  Operation to be performed;
		                                 lio_listio() only 

	 Various implementation-internal fields not shown 
	}; */


	// Open file descriptor to the bus unbind interface
	int file = open("/sys/bus/usb/drivers/usb/unbind", O_WRONLY, 0);


	if(file == -1) {
		return 1;
	}

	// Grab the path to the device
	const char* path = udev_device_get_devpath(dev);
	
	char* token;
	char* str = strdup(path);
	char* location;
	

	if(!path) {
		close(file);
		return 1;
	}

	// Grab only the relevant bus location from the path
	while((token = strsep(&str, "/")) != NULL) {
		location = strdup(token);
	}

	// create the control block structure
	// and buffer to write
	struct aiocb cb;
	char write_buffer[strlen(location)+1];
	
	memset(&cb, 0, sizeof(struct aiocb));
	memcpy(write_buffer, location, strlen(location)+1);
	cb.aio_nbytes = strlen(location)+1;
	cb.aio_fildes = file;
	cb.aio_offset = 0;
	cb.aio_buf = write_buffer;

	//printf("%lu\n", strlen(write_buffer));
	//printf("%s\n", write_buffer);

	// Async-write this location to the unbind interface
	if(aio_write(&cb) == -1) {
		close(file);
		return 1;
	}

	/* Wait until completion */
	while (aio_error (&cb) == EINPROGRESS);

	// Check for errors
	int err = aio_error(&cb);
	int ret = aio_return(&cb);

	if (err != 0) {
		close (file);
		return 1;
	}

	close(file);
	return 0;

}


static void device_notification(struct udev_device* dev, int status)
{

	notify_init("Sample");
	NotifyNotification * n = NULL;
	char * message;

	// Status
	char * s_msg = "Rejected";
	if (status == 0) { asprintf(&s_msg, "Whitelisted"); }



	const char* action = udev_device_get_action(dev);
	if (! action) {
		action = "exists";
	}


	// Grab device information
	const char* vendor = udev_device_get_sysattr_value(dev, "idVendor");
	const char* product = udev_device_get_sysattr_value(dev, "idProduct");
	const char* serial = udev_device_get_sysattr_value(dev, "serial");


	if(!vendor || !product || !serial) {
		return;
	}


	if (0> asprintf(&message, 
		"Status: %s\n   Serial #: %s\n   idVendor-idProduct: %s-%s\n   Action: %s\n",
		s_msg, serial, vendor, product, action)) {
		syslog(LOG_WARNING, "[-] Format String failed");
		return;
	}

	
	n = notify_notification_new ("USBGuard", message, 0);
	

	if(!notify_notification_show(n, 0)) {
		syslog(LOG_WARNING, "[-] Could not display message");
		return;
	}
}


int strtodec(const char * num) {

	int dec = 0, i, j, len;

	len = strlen(num);
	for(i=0; i<len; i++){
		dec = dec * 10 + ( num[i] - '0' );
	}

	return dec;
}
 
/*
	Returns 0 if whitelisted, 1 otherwise
*/
static int check_whitelist(struct udev_device * dev) {

	deviceID *iter = whitelist;

	// Grab device information
	const char* vendor = udev_device_get_sysattr_value(dev, "idVendor");
	const char* product = udev_device_get_sysattr_value(dev, "idProduct");
	const char* serial = udev_device_get_sysattr_value(dev, "serial");

	if(!vendor || !product || !serial) {
		return 1;
	}

	// Loop through the whitelist
	while (iter) {

		/* Debug print statements
		printf("%s %s\n", serial, iter->serialnumber);
		printf("%d %d\n", strtodec(product), iter->productID);
		printf("%d %d\n", strtodec(vendor), iter->vendorID);*/

		if ((strncmp(serial, iter->serialnumber, strlen(serial))==0) &&
			(strtodec(product) == iter->productID) &&
			(strtodec(vendor) == iter->vendorID)) {
			return 0;
		}

		// Move to next item
		iter = iter->next;
	}

	return 1;
} 


static void process_device(struct udev_device* dev)
{
	// Number of failed disconnect async requests before continuing
	int count = 0;
	int result = 1;

	if (dev) {
		if (udev_device_get_devnode(dev)) {


			result = check_whitelist(dev);

			// Display a notification
			device_notification(dev, result);

			if(result) {

				// Disconnect the non-whitelisted device
				count = 0;

				while(disconnect_device(dev) != 0) {
					count+=1;
					if (count > 5) {
						return;
					}
				}	

			}	
			
		}

		udev_device_unref(dev);
	}
}



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

	syslog(LOG_NOTICE, "[-] Stopped monitoring");
}

deviceID * create_device(char * serialnumber, int vendor, int product) {

	// Allocate memory
	deviceID * dpointer = malloc(sizeof(deviceID));

	// Set values
	if(dpointer) {
		dpointer->serialnumber = malloc(50);
		dpointer->vendorID = vendor;
		dpointer->productID = product;
		dpointer->next = NULL;

		// Copy the serial number over in memory
		memcpy(dpointer->serialnumber, serialnumber, 50);
	} 

	return dpointer;
}


void print_whitelist (deviceID * list) {

	// Pointer to head of the linked-list
	deviceID *iter = whitelist;

	printf("\n\n               Whitelist               \n"
		   "------------------------------------------\n"
		   "   Serial Number  | VendorID |  ProductID \n"
		   "------------------------------------------\n");


	// Loop through the list
	while (iter) {

		// Print the device information
		printf(" %s     %d        %d\n", iter->serialnumber, iter->vendorID, iter->productID);   

		// Move to next item
		iter = iter->next;
	}

}


int parse_config(){

	// Arguments
	char serialnumber[50];
	int vendor, product;

	// File and line variables
	char line[256];
	FILE * pFile;

	
	// Open the file
	if ((pFile = fopen("/etc/usbguard/usb-whitelist.cfg","rw+")) == NULL) {
		return 1;
	}

	// Read lines until end of file
	while(fgets(line, sizeof(line), pFile)) {



		// Read in formatted string from line
		if((*line != '\n') && (strstr(line, "#") == NULL) && sscanf (line,"%s %d %d",serialnumber, &vendor, &product)) {


			
			// Add a new node to the linked list (whitelist)
			deviceID * new_device = create_device (serialnumber, vendor, product);

			if(new_device){

				// Make the new entry the head 
				new_device->next = whitelist;

				whitelist = new_device;
			}   


		} 
		
	}

	fclose(pFile);
	print_whitelist(whitelist);

}


void clear_whitelist(){

	// Iterate through the linked list freeing memory blocks
	while(whitelist) {
		// Get head of the list
		deviceID *block = whitelist;

		// Move head to next
		whitelist = whitelist->next;

		// Delete the head
		if(block->serialnumber) {
			free(block->serialnumber);
		}
		free(block);
	}

}


void handle_signal(int sig) {

	if (sig == SIGINT) {
		syslog(LOG_NOTICE, "[*] Shutting down USBGuard");
		closelog();
		syslog(LOG_NOTICE, "[*] USBGuard Stopped");
		exit(0);
	} else if (sig == SIGHUP) {
		syslog(LOG_NOTICE, "[*] Reloading configuration file...");
		clear_whitelist();
		parse_config();
	} else {
		syslog(LOG_NOTICE, "[-] Unhandled signal...");
	}
}



int main(int * argc, int ** argv) {


	printf("\n                USBGuard                \n"
			"     Personal Computer USB Management    \n");

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


	// Create the whitelist 
	parse_config();


	// Open the log file 
	openlog ("USBGuard", LOG_PID, LOG_DAEMON);


	if(geteuid() != 0) {
		printf("\n[-] The daemon needs to run with root permissions"
			   " to disconnect devices.\n");
		syslog(LOG_NOTICE, "[-] Not run with appropriate permissions");
		closelog();
		exit(1);
	}


	// Daemonize and run in the backgroun
	printf("\n[*] Starting daemon...look for the Ubuntu notification\n");
	//daemon(0,0);

	/* Daemon will handle two signals */
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);

	if (0> asprintf(&message, "[+] Background service is running...\nPID: %d", getpid())) {
		return EXIT_FAILURE;
	}
	

	n = notify_notification_new ("USBGuard", message, 0);

	notify_notification_set_timeout(n, 5000); // 5 seconds

	if(!notify_notification_show(n, 0)) {
		syslog(LOG_WARNING, "[-] Could not display message");
		//return EXIT_FAILURE;
	}
	


	syslog(LOG_NOTICE, "[+] USBGuard Started");
	syslog(LOG_NOTICE, "%s", message);

	// Begin monitoring for USB insertions/removals
	monitor_devices(udev);


	// Cleanup
	syslog(LOG_NOTICE, "[-] USBGuard Stopped");
	closelog();
	return EXIT_SUCCESS;
}