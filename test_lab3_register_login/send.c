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

	printf("Available commands: REGISTER <user> <pass> | LOGIN <user> <pass> | CONNECT\n");

	while(1) {
		struct l3_msg t;
		/* We set the payload */
		/* trebuie sa citim un input de la tastatura
		sa il salvam intr-un buffer,
		il trimitem catre receptor si el trebuie sa "parseze" comanda */
		char command[64], user[64], pass[64];
		char input[256];

		if (fgets(input, sizeof(input), stdin)) {
			int num = sscanf(input, "%s %s %s", command, user, pass);

			if (strcmp(command, "REGISTER") == 0) {
				t.hdr.type = 1;
			} else if (strcmp(command, "LOGIN") == 0) {
				t.hdr.type = 2;
			} else if (strcmp(command, "CONNECT") == 0) {
				t.hdr.type = 3;
			}

			snprintf(t.payload, MAX_PAYLOAD, "%s %s", user, pass);
			
			t.hdr.len = strlen(t.payload) + 1;
		}

		/* Add the checksum */
		/* Note that we compute the checksum for both header and data. Thus
		* we set the checksum equal to 0 when computing it */
		t.hdr.sum = 0;

		/* Since sum is on 32 bits, we have to convert it to network order */
		t.hdr.sum = htonl(simple_csum((void *) &t, sizeof(struct l3_msg)));

		/* TODO 2.0: Call crc32 function */

		/* Send the message */

		/* TODO 3.1: Receive the confirmation */
		int success = 0;
        while (!success) {
            link_send(&t, sizeof(struct l3_msg));

            struct l3_msg res;
            if (link_recv(&res, sizeof(struct l3_msg)) > 0) {
				/* verificam checksum-ul */
				uint32_t res_sum = ntohl(res.hdr.sum);
				res.hdr.sum = 0;
				if (simple_csum((void *)&res, sizeof(struct l3_msg)) == res_sum) {
					if (res.hdr.type == 4) { // ACK
						struct l3_msg status;
						if (link_recv(&status, sizeof(struct l3_msg)) > 0) {
							// afisam doar daca rasp. este valid
							printf("%s", status.payload);
							success = 1;
						}
					} else if (res.hdr.type == 5) { // NACK
                        printf("[SEND]: Got NACK (frame was corrupted at receiver), retransmitting...\n");
                    }
				}
			}
		}

		/* TODO 3.3: Update this to read the content of a file and send it as
		* chunks of that file given a MTU of 1500 bytes */

	}
	return 0;
}
