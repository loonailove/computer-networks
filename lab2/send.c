#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"
#include "include/utils.h"

/**
 * You can change these to communicate with another colleague.
 * There are several factors that could stop this from working over the
 * internet, but if you're on the same network it should work.
 * Just fill in their IP here and make sure that you use the same port.
 */
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
	Frame *frame = (Frame *)malloc(sizeof(struct Frame));
	memset(frame, 0, sizeof(struct Frame));
	/* TODO 1.2: Copy the data from buffer to our frame structure */
	//strncpy(frame->payload, buf, size);
	memcpy(frame->payload, buf, size);
	/* TODO 2.1: Set the destination and source */
	frame->frame_delim_start[0] = DLE;
	frame->frame_delim_start[1] = STX;
	
	frame->source = ID;
	frame->destination = 0;

	frame->frame_delim_end[0] = DLE;
	frame->frame_delim_end[1] = ETX;
	/* TODO 1.3: We can cast the frame to a char *, and iterate through sizeof(struct Frame) bytes
	 calling send_bytes. */

	char *cast = (char *)frame;
	for (int i = 0; i < sizeof(struct Frame); i++) {
		send_byte(cast[i]);
	}

	/* if all went all right, return 0 */
	free(frame);

	return 0;
}

int main(int argc,char** argv){
	// Don't touch this
	init(HOST,PORT);

	// TODO remove these sends, whih are hardcoded to send a "Hello"
	// message, and replace them with code that can send any message.
	/* Send Hello */
	// send_byte(DLE);
	// send_byte(STX);
	// send_byte('H');
	// send_byte('e');
	// send_byte('l');
	// send_byte('l');
	// send_byte('o');
	// send_byte('!');
	// send_byte(DLE);
	// send_byte(ETX);

	/* TODO 1.0: Get some input in a buffer and call send_frame with it */
	char *buffer = (char *)malloc(256 * sizeof(char));
	scanf("%s", buffer);

	send_frame(buffer, strlen(buffer));
	/* TODO 3.1: Get a timestamp of the current time copy it in the the payload */
	/*
	time_t currentTime;
	time(&currentTime);

	// printf("Current time: %s\n", currentTime);

	char *timeStr = ctime(&currentTime);	// long int -> string (char *)
	send_frame(timeStr, strlen(timeStr));
	*/

	struct timeval t_send;
	gettimeofday(&t_send, NULL); // luam timpul exact acum

	send_frame((char *)&t_send, sizeof(struct timeval));

	/* TODO 3.0: Update the maximum size of the payload in Frame to 100 (in common.h), send the frame */

	/* TODO 3.0: Update the maximum size of the payload in Frame to 300, send the frame */

	free(buffer);

	return 0;
}
