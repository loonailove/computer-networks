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

	/* Since we are sending messages with a payload of 1500 - sizeof(header), most of the times the bytes from
	 * 30 - 1500 will be corrupted and thus when we are printing or string message "Hello world" we see no probems.
	 * This will be visible when we will be sending a file */

	/* TODO 3.1: In a loop, recv a frame and check if the CRC is good */
	int fd = open("recv.data", O_WRONLY, O_CREAT, O_TRUNC, 0644);
	if (fd < 0) {
		printf("Eroare la deschiderea fisierului!\n");
		return -1;
        }

	while(1) {
        	struct l3_msg t;

        	/* Receive the frame from the link */
        	int len = link_recv((void *) &t, sizeof(struct l3_msg));
		if (len == 0) {
			printf("[RECV] Toate pachetele au fost primite. Inchidem conexiunea.");
			break;
		}

        	int recv_sum = ntohl(t.hdr.sum);

        	// t.hdr.sum = 0;
        	/* We have to convert it to host order */
        	/* network order to host order */
        	// t.hdr.sum = simple_csum((void *) &t, sizeof(struct l3_msg));
        	// int sum_ok = (t.hdr.sum == recv_sum);

        	/* TODO 2: Change to crc32 */
        	t.hdr.sum = 0;
        	t.hdr.sum = crc32((void *) &t, sizeof(struct l3_msg));
        	int sum_ok = (t.hdr.sum == recv_sum);

		// printf("[RECV] len=%d; sum(%s)=0x%04hx; payload=\"%s\";\n", t.hdr.len, sum_ok ? "GOOD" : "BAD", recv_sum, t.payload);

		struct l3_msg ack_packet;
		memset(&ack_packet, 0, sizeof(struct l3_msg));

		if (sum_ok == 1) {
			// GOOD
			printf("[RECV] OK... Packet received.\n");
			strncpy(ack_packet.payload, "ACK", strlen("ACK") + 1);

			ack_packet.hdr.len = strlen(ack_packet.payload) + 1;
			ack_packet.hdr.sum = 0;
			ack_packet.hdr.sum = htonl(crc32((void *) &ack_packet, sizeof(struct l3_msg)));

			link_send(&ack_packet, sizeof(struct l3_msg));

			int bytes_written = write(fd, t.payload, t.hdr.len);
			if (bytes_written < 0) {
				printf("Eroare la scrierea in fisier!\n");
				return -1;
			}
		} else {
			// BAD
			printf("[RECV] CORRUPTED...\n");
			strncpy(ack_packet.payload, "NACK", strlen("NACK") + 1);

			ack_packet.hdr.len = strlen(ack_packet.payload) + 1;
			ack_packet.hdr.sum = 0;
			ack_packet.hdr.sum = htonl(crc32((void *) &ack_packet, sizeof(struct l3_msg)));

			link_send(&ack_packet, sizeof(struct l3_msg));
		}
	}

	close(fd);

	/* TODO 3.2: If the crc is bad, send a NACK frame */

	/* TODO 3.2: Otherwise, write the frame payload to a file recv.data */

	/* TODO 3.3: Adjust the corruption rate */



	return 0;
}
