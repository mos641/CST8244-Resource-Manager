#include <stdlib.h>
#include <stdio.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/neutrino.h>

#define MY_PULSE_CODE   _PULSE_CODE_MINAVAIL

typedef union {
	struct _pulse pulse;
	char msg[255];
} my_message_t;

/*
 *
 Configure myController as a server; register the device within the namespace
 call name_attach() and register the device name: “mydevice”
 exit FAILURE if name_attach() failed
 Upon startup, myController is to read the status of the device:

 call fopen() to open the device: /dev/local/mydevice
 scan the status and value from the file
 if the status is “status” then printf %s, value
 then if the value is “closed”
 then name_detach and exit with SUCCESS
 close the device by its fd

 while True:
 call MsgReceivePulse(attach->chid, &msg, sizeof(msg), NULL)

 if received a pulse message (i.e. rcvid == 0)
 then if received MY_PULSE_CODE

 then print the small integer from the pulse: printf("Small Integer: %d\n", msg.pulse.value.sival_int);
 call fopen() to open the device: /dev/local/mydevice
 scan the status and value from the file

 if the status is “status”
 then print Status: %s, value

 if value is “closed”
 then name_detach() and exit with SUCCESS
 close the fd

 else //rcvid != 0
 display appropriate error message and exit with FAILURE
 call name_detach()
 exit with SUCCESS
 *
 */
int main(int argc, char *argv[]) {
	name_attach_t *attach;
	int rcvid;
	FILE *mydevice;
	my_message_t msg;

	//fprintf(stderr, "0\n");

	if ((attach = name_attach(NULL, "mydevice", 0)) == NULL) {
		return EXIT_FAILURE;
	}

	//fprintf(stderr, "1\n");

	mydevice = fopen("/dev/local/mydevice", "r");

	char status[10];
	char value[255];
	//fprintf(stderr, "1.5\n");
	fscanf(mydevice, "%s %s", status, value);
	//fprintf(stderr, "2\n");

	fclose(mydevice);

	//printf("state = \"%s\", value = \"%s\"\n", status, value);

	if (strcmp(status, "status") == 0) {
		printf("Status %s\n", value);
	}

	//fprintf(stderr, "3\n");

	if (strcmp(value, "closed") == 0) {
		name_detach(attach, 0);
		fprintf(stderr, "closing\n");
		return EXIT_SUCCESS;
	}

	//fprintf(stderr, "4\n");

	while (1) {

		//fprintf(stderr, "receiving pulse");

		rcvid = MsgReceivePulse(attach->chid, &msg, sizeof(msg), NULL);

		//fprintf(stderr, "pulse received");

		if (rcvid == 0) {
			//fprintf(stderr, "pulse received");

			if (MY_PULSE_CODE == msg.pulse.code) {

				printf("Small Integer: %d\n",
						msg.pulse.value.sival_int);

				mydevice = fopen("/dev/local/mydevice", "r");

				fscanf(mydevice, "%s %s", status, value);

				if (strcmp(status, "status") == 0) {

					printf("Status %s\n", value);
				}

				if (strcmp(value, "closed") == 0) {

					fclose(mydevice);

					name_detach(attach, 0);

					return EXIT_SUCCESS;
				}
			} else {
				// if an error is received
				fprintf(stderr, "error received");

				name_detach(attach, 0);
				return EXIT_FAILURE;
			}

		}
	}

	fclose(mydevice);

	name_detach(attach, 0);

	return EXIT_SUCCESS;

}
