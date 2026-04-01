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

	FILE *f_out = fopen("recv.data", "wb");
	if (!f_out) {
		printf("Eroare la deschiderea fisierului!\n");
		return -1;
	}

	/* cream urmatoarele 2 pachete: ACK si NACK (pt a ne usura munca ulterior) */
	struct l3_msg ack;
	memcpy(ack.payload, "ACK", strlen("ACK"));
	ack.hdr.len = strlen("ACK");
	ack.hdr.sum = 0;
	ack.hdr.sum = htonl(crc32((uint8_t *)&ack, sizeof(struct l3_msg)));

	struct l3_msg nack;
	memcpy(nack.payload, "NACK", strlen("NACK"));
	nack.hdr.len = strlen("NACK");
	nack.hdr.sum = 0;
	nack.hdr.sum = htonl(crc32((uint8_t *)&nack, sizeof(struct l3_msg)));

	/* receiver este incontinuu pregatit sa primeasca pachete */
	/* se opreste doar cand primeste pachetul cu len = 0 si sum = 0 */
	while(1) {
		link_recv(&t, sizeof(struct l3_msg));

		/* end of tranmission */
		if (t.hdr.len == 0) {
			printf("Inchidere transmisie...\n");
			break;
		}

		/* altfel, verificam integritatea pachetului primit */

		uint32_t original_crc32 = t.hdr.sum;
		t.hdr.sum = 0;
		t.hdr.sum = htonl(crc32((uint8_t *)&t, sizeof(struct l3_msg)));

		if (t.hdr.sum == original_crc32) {
			printf("[RECV]: Packet received. Sending ACK.\n");

			/* trebuie sa scriem in fisier */
			fwrite(t.payload, 1, t.hdr.len, f_out);

			link_send(&ack, sizeof(struct l3_msg));
		} else {
			printf("[RECV]: Packet CORRUPTED. Sending NACK.\n");
			link_send(&nack, sizeof(struct l3_msg));
		}
	}

	fclose(f_out);

	return 0;
}
