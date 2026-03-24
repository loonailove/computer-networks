#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "common.h"
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


int main(int argc,char** argv) {
	/* Don't modify this */
	init(HOST,PORT);

	struct l3_msg t;

	/* Receive the frame from the link */
	int len = link_recv(&t, sizeof(struct l3_msg));
	DIE(len < 0, "Receive message");

	/* We have to convert it to host order */
	uint32_t recv_sum = ntohl(t.hdr.sum);
	/* zeroing the field */
	t.hdr.sum = 0;
	int sum_ok = (simple_csum((void *) &t, sizeof(struct l3_msg)) == recv_sum);
	/* TODO 2: Change to crc32 */
	
	uint32_t compute_crc = crc32((void *) &t, sizeof(struct 13_msg));
	uint32_t ok_crc = (recv_sum == compute_crc);

	/* Since we are sending messages with a payload of 1500 - sizeof(header), most of the times the bytes from
	 * 30 - 1500 will be corrupted and thus when we are printing or string message "Hello world" we see no probems.
	 * This will be visible when we will be sending a file */

	printf("[RECV] len=%d; sum(%s)=0x%04hx; payload=\"%s\";\n", t.hdr.len, sum_ok ? "GOOD" : "BAD", recv_sum, t.payload);

	/* TODO 3.1: In a loop, recv a frame and check if the CRC is good */

	/* TODO 3.2: If the crc is bad, send a NACK frame */

	/* TODO 3.2: Otherwise, write the frame payload to a file recv.data */

	/* TODO 3.3: Adjust the corruption rate */

	while(1) {
		int len = link_recv(&t, sizeof(struct l3_msg));
		if (len < 0) break;

		uint32_t recv_sum = ntohl(t.hdr.sum);
		t.hdr.sum = 0;


	}

	return 0;
}
