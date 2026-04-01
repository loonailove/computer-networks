#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "common.h"
#include "link_emulator/lib.h"
#include <arpa/inet.h>
#include "include/utils.h"

#define HOST "127.0.0.1"
#define PORT 10000


int main(int argc,char** argv) {
	init(HOST,PORT);

	/* Look in common.h for the definition of l3_msg */
	struct l3_msg t;

	FILE *f_in = fopen("file.bin", "rb");
	if (!f_in) {
		printf("Eroare la deschiderea fisierului!\n");
		return -1;
	}

	/* acum trebuie sa trimitem pachete cu informatii de cate 1400 de octeti maxim */

	/* cat timp mai avem octeti de citit ... */
	int bytes_read = 0;
	while((bytes_read = fread(t.payload, 1, MAX_PAYLOAD, f_in)) > 0) {
		t.hdr.len = bytes_read;

		t.hdr.sum = 0;
		t.hdr.sum = htonl(crc32((uint8_t *)&t, sizeof(struct l3_msg)));

		// trimitem pachetul
		// dar trebuie sa asteptam si ack/nack pana a-l trimite pe urmatorul
		int success = 0;
		while (!success) {
			link_send(&t, sizeof(struct l3_msg));

			/* asteptam raspunsul de la receiver */
			struct l3_msg reply;
			link_recv(&reply, sizeof(struct l3_msg));

			/* 1. verificam ca raspunsul sa nu se fi corupt pe drum */
			uint32_t original_crc32 = reply.hdr.sum;
			reply.hdr.sum = 0;
			reply.hdr.sum = htonl(crc32((uint8_t *)&reply, sizeof(struct l3_msg)));

			if (original_crc32 != reply.hdr.sum) {
				printf("[SEND] CORRUPTED ACK/NACK... retransmitting\n");
				continue;
			}

			/* 2. am primit raspunsul de la receiver */
			if (strncmp(reply.payload, "ACK", sizeof(reply.hdr.len)) == 0) {
				printf("[SEND]: ACK received!\n");
				success = 1;
			} else {
				printf("[SEND]: NACK received... retransmitting\n");
				continue;
			}
		}
	}

	/* trimitem un pachet cu len = 0 si sum = 0
	pt. a notifica receiver-ul ca s-a oprit transmisia!!!
	*/
	t.hdr.len = 0;
	t.hdr.sum = 0;
	t.hdr.sum = htonl(crc32((uint8_t *)&t, sizeof(struct l3_msg)));
	link_send(&t, sizeof(struct l3_msg));
	
	fclose(f_in);

	return 0;
}
