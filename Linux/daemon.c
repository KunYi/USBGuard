/*

	Linux USBGuard Daemon

	Runs in the background and handles USB related signals

*/

#include "daemon.h"

// The whitelist
deviceID * whitelist = NULL;


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
	if (dev) {
		if (udev_device_get_devnode(dev)) {

			// Display a notification
			device_notification(dev, check_whitelist(dev));
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

	printf("\n\n                 Whitelist              \n"
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
	if ((pFile = fopen("usb-whitelist.cfg","rw+")) == NULL) {
		return 1;
	}

	// Read lines until end of file
	while(fgets(line, sizeof(line), pFile)) {

		// Read in formatted string from line
		if(sscanf (line,"%s %d %d",serialnumber, &vendor, &product)) {


			
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


	// Create the whitelist 
	parse_config();

	
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


	// Cleanup
	syslog(LOG_NOTICE, "[-] USBGuard Stopped");
	closelog();
	return EXIT_SUCCESS;
}