#include <stdlib.h>
#include <stdio.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/neutrino.h>

char data[255];
int server_coid;

typedef union {
	struct _pulse pulse;
	char msg[255];
} my_message_t;

int io_read(resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb) {
	// fprintf(stderr, "io read\n");
	//static char data[] = "hello";
	int nb;
	if (data == NULL){
		return 0;
		// fprintf(stderr, "io read data = null\n");
	}

	nb = strlen(data);

	//test to see if we have already sent the whole message.
	if (ocb->offset == nb){
		return 0;
		// fprintf(stderr, "io ocb offset message sent\n");
	}

	//We will return which ever is smaller the size of our data or the size of the buffer
	nb = min(nb, msg->i.nbytes);

	//Set the number of bytes we will return
	_IO_SET_READ_NBYTES(ctp, nb);

	//Copy data into reply buffer.
	SETIOV(ctp->iov, data, nb);

	//update offset into our data used to determine start position for next read.
	ocb->offset += nb;

	//If we are going to send any bytes update the access time for this resource.
	if (nb > 0)
		ocb->attr->flags |= IOFUNC_ATTR_ATIME;

	return (_RESMGR_NPARTS(1));
}

int io_write(resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb) {
	int nb = 0;

	// fprintf(stderr, "io write \n");

	if (msg->i.nbytes == ctp->info.msglen - (ctp->offset + sizeof(*msg))) {
		/* have all the data */
		char *buf;
		char *alert_msg;
		int i, small_integer;
		buf = (char*) (msg + 1);

		if (strstr(buf, "alert") != NULL) {
			for (i = 0; i < 2; i++) {
				alert_msg = strsep(&buf, " ");
			}
			small_integer = atoi(alert_msg);
			if (small_integer >= 1 && small_integer <= 99) {
				//FIXME :: replace getprio() with SchedGet()
				MsgSendPulse(server_coid, SchedGet(0, 0, NULL),
						_PULSE_CODE_MINAVAIL, small_integer);
				// fprintf(stderr, "correct Integer\n");
			} else {
				fprintf(stderr, "Integer is not between 1 and 99.\n");
			}
		} else {
			strcpy(data, buf);
		}

		nb = msg->i.nbytes;
	}
	_IO_SET_WRITE_NBYTES(ctp, nb);

	if (msg->i.nbytes > 0)
		ocb->attr->flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_CTIME;

	return (_RESMGR_NPARTS(0));
}

int io_open(resmgr_context_t *ctp, io_open_t *msg, RESMGR_HANDLE_T *handle,
		void *extra) {
	// fprintf(stderr, "io open\n");
	if ((server_coid = name_open("mydevice", 0)) == -1) {
		perror("name_open failed.");
		return EXIT_FAILURE;
	} else {
		// fprintf(stderr, "name open succeeded \n");
	}
	return (iofunc_open_default(ctp, msg, handle, extra));
}
int main(int argc, char *argv[]) {
	dispatch_t *dpp;
	resmgr_io_funcs_t io_funcs;
	resmgr_connect_funcs_t connect_funcs;
	iofunc_attr_t ioattr;
	dispatch_context_t *ctp;
	int id;

	// fprintf(stderr, "io read\n");

	if ((dpp = dispatch_create()) == NULL) {
		fprintf(stderr, "%s:  Unable to allocate dispatch context.\n", argv[0]);
		return (EXIT_FAILURE);
	} else {
		// fprintf(stderr, "allocated dispatch context\n");
	}
	iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs, _RESMGR_IO_NFUNCS,
			&io_funcs);
	connect_funcs.open = io_open;
	// fprintf(stderr, "io open return = %d\n", connect_funcs.open);
	io_funcs.read = io_read;
	// fprintf(stderr, "io read return = %d\n", io_funcs.read);
	io_funcs.write = io_write;
	// fprintf(stderr, "io write return = %d\n", io_funcs.write);

	iofunc_attr_init(&ioattr, S_IFCHR | 0666, NULL, NULL);

	if ((id = resmgr_attach(dpp, NULL, "/dev/local/mydevice", _FTYPE_ANY, 0,
			&connect_funcs, &io_funcs, &ioattr)) == -1) {
		fprintf(stderr, "%s:  Unable to attach name.\n", argv[0]);
		return (EXIT_FAILURE);
	} else {
		// fprintf(stderr, "device attached name\n");
	}

	ctp = dispatch_context_alloc(dpp);
	while (1) {
		ctp = dispatch_block(ctp);
		dispatch_handler(ctp);
	}
	return EXIT_SUCCESS;
}
