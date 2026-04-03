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

	printf("Available commands: OPEN <account> | DEPOSIT <account> <ammount> | BALANCE <account>\n");

	/* Look in common.h for the definition of l3_msg */
	struct l3_msg t;
	struct l3_msg reply;

	/* citim de la tastatura comanda
	o punem in pachetul repr. de structura l3_msg
	*/

	char buffer[256];
	char command[256], arg1[256], arg2[256];

	while(fgets(buffer, sizeof(buffer), stdin) > 0) {
		memset(&t, 0, sizeof(struct l3_msg));
   		buffer[strcspn(buffer, "\n")] = 0;

		int match = sscanf(buffer, "%s %s %s", command, arg1, arg2);
		int valid_command = 0;

		if (strncmp(command, "OPEN", strlen("OPEN")) == 0 ||
			strncmp(command, "DEPOSIT", strlen("DEPOSIT")) == 0 ||
			strncmp(command, "BALANCE", strlen("BALANCE")) == 0
		) {
			valid_command = 1;
		}

		if ((match == 2 || match == 3) && valid_command == 1) {
			// OPEN | BALANCA
			memcpy(t.payload, buffer, strlen(buffer));
			t.hdr.len = strlen(t.payload);

			t.hdr.sum = 0;
			t.hdr.sum = crc32((void *)&t, t.hdr.len);

			link_send((void *)&t, sizeof(struct l3_msg));

			/* am trimis pachetul
			vedem daca am primit ack sau nack
			*/

			link_recv((void *)&reply, sizeof(struct l3_msg));
			
			if (strncmp(reply.payload, "ACK", strlen("ACK")) == 0) {
				printf("[SEND] Packet was sent succesfully!\n");
				continue;
			} else if (strncmp(reply.payload, "NACK", strlen("NACK")) == 0) {
				printf("[SEND] Got NACK (frame was corrupted at receiver)\n");
				continue;
			}

		// } else if (res == 3) {
		// 	// DEPOSIT
		// 	memcpy(&t.payload, buffer, MAX_PAYLOAD);

		} else if (valid_command == 0) {
			printf("[SEND] Comanda nevalida! Vezi: Available commands.\n");
		} else {
			printf("[SEND] Argumentele nu se potrivesc. Vezi: Available commands.\n");
		}
	}

	// ultimul pachet (pt. ca recv sa se opreasca din a primi pachete)
	memset(&t.payload, 0, MAX_PAYLOAD);
	t.hdr.len = 0;
	t.hdr.sum = 0;
	link_send((void *)&t, sizeof(struct l3_msg));

	return 0;
}
