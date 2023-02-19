#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"

/* We don't touch this */
#define HOST "127.0.0.1"
#define PORT 10000

/* Here we have the Frame structure */
#include "common.h"

/* Our unqiue layer 2 ID */
static int ID = 123131;

/* Function which our protocol implementation will provide to the upper layer. */
int send_frame(char *buf, int size)
{

	/* TODO 1.1: Create a new frame. */

	/* TODO 1.2: Copy the data from buffer to our frame structure */

	/* TODO 2.1: Set the destination and source */

	/* TODO 1.3: We can cast the frame to a char *, and iterate through sizeof(struct Frame) bytes
	 calling send_bytes. */

	/* if all went all right, return 0 */
	return 0;
}

int main(int argc,char** argv){
	// Don't touch this
	init(HOST,PORT);

	/* Send Hello */
	send_byte(DLE);
	send_byte(STX);
	send_byte('H');
	send_byte('e');
	send_byte('l');
	send_byte('l');
	send_byte('o');
	send_byte('!');
	send_byte(DLE);
	send_byte(ETX);

	/* TODO 1.0: Get some input in a buffer and call send_frame with it */

	/* TODO 3.1: Get a timestamp of the current time copy it in the the payload */

	/* TODO 3.0: Upload the maximum size of the payload in Frame to 100, send the frame */

	/* TODO 3.0: Upload the maximum size of the payload in Frame to 300, send the frame */

	return 0;
}
