#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"
#include "include/utils.h"

/**
 * You can change these to communicate with another colleague.
 * There are several factors that could stop this from working over the
 * internet, but if you're on the same network it should work.
 * Just fill in their IP here and make sure that you use the same port.
 */
#define HOST "127.0.0.1"
#define PORT 10001

#include "common.h"

/* Our unique layer 2 ID */
static int ID = 123131;

/* Function which our protocol implementation will provide to the upper layer. */
int recv_frame(char *buf, int size)
{
	/* TODO 1.1: Call recv_byte() until we receive the frame start
	 * delimitator. This operation makes this function blocking until it
	 * receives a frame. */
	char c1, c2;
	c1 = recv_byte();
	c2 = recv_byte();

	while ((c1 != DLE && c2 != STX) || (c1 != DLE && c2 == STX) || (c1 == DLE && c2 != STX)) {
		c1 = c2;
		c2 = recv_byte();
	}
	/* TODO 2.1: The first two 2 * sizeof(int) bytes represent sender and receiver ID */
	// am gasit DLE si STX

	Frame frame;
	char *cast = (char *)&frame;

	cast[0] = c1;
	cast[1] = c2;

	for (int i = 2; i < sizeof(struct Frame); i++) {
		cast[i] = recv_byte();
	}

	if (frame.destination != ID && frame.destination != 0) {
		printf("[RECEIVER] Cadrul a fost ignorat (destinație greșită: %d)\n", frame.destination);
		return -1; // Nu e pentru noi
	}

	/* TODO 2.2: Check that the frame was sent to me */

	/* TODO 1.2: Read bytes and copy them to buff until we receive the end of the frame */

	// extragem payload-ul si il punem in buffer
	strncpy(buf, frame.payload, size);

	if (frame.frame_delim_end[0] == DLE && frame.frame_delim_end[1] == ETX) {
		return strlen(buf); // everything went well
	}

	/* If everything went well return the number of bytes received */
	return -1;
}

int main(int argc,char** argv){
	/* Don't modify this */
	init(HOST,PORT);

        // TODO remove these recives, whih are hardcoded to receive a "Hello"
	// message, and replace them with code that can receive any message.
	// char c;

	/* Wait for the start of a frame */
	// char c1,c2;
	// c1 = recv_byte();
	// c2 = recv_byte();

	/* Cat timp nu am primit DLE STX citim bytes
	while(((c1 != DLE) && (c2 != STX)) || (c1 == DLE && c2 != STX)
        || (c1 != DLE && c2 == STX)) {
		c1 = c2;
		c2 = recv_byte();
	}


	printf("%d ## %d\n",c1, c2);
	c = recv_byte();
	printf("%c\n", c);

	c = recv_byte();
	printf("%c\n", c);

	c = recv_byte();
	printf("%c\n", c);

	c = recv_byte();
	printf("%c\n", c);

	c = recv_byte();
	printf("%c\n", c);

	c = recv_byte();
	printf("%c\n", c);
	*/

	/* TODO 1.0: Allocate a buffer and call recv_frame */
	char *buffer = (char *)malloc(256 * sizeof(char));

	printf("[RECEIVER] Aștept date...\n");

	int bytes_received = recv_frame(buffer, 256);
	if (bytes_received >= 0) {
		printf("[RECEIVER] Am primit %d octeți. Mesajul este: %s\n", bytes_received, buffer);
	} else {
		printf("[RECEIVER] Eroare la primirea cadrului.\n");
	}

	bytes_received = recv_frame(buffer, 256);
	if (bytes_received >= 0) {
		printf("[RECEIVER] Am primit cadrul 2 (Timestamp): %s\n", buffer);
	}

	free(buffer);

	/* TODO 3: Measure latency in a while loop for any frame that contains
	 * a timestamp we receive, print frame_size and latency */

	printf("[RECEIVER] Finished transmission\n");
	return 0;
}
